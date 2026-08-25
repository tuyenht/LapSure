#include "lap/chassis_profile.h"
#include "lap/profile.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void Expect(bool condition, const char* behavior) {
    if (!condition) {
        std::cerr << "FAIL " << behavior << '\n';
        ++failures;
    } else {
        std::cout << "PASS " << behavior << '\n';
    }
}

void WriteText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}
} // namespace

int main() {
    std::error_code ec;
    const auto root = std::filesystem::temp_directory_path() / L"lapsure-profile-provenance-security";
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / L"profiles" / L"chassis", ec);
    if (ec) {
        std::cerr << "FAIL could not create provenance test root\n";
        return 1;
    }

    WriteText(root / L"profiles" / L"attacker.json",
        "{\n"
        "  \"model\": \"Dell Precision 5560\",\n"
        "  \"serviceTag\": \"ATTACK1\",\n"
        "  \"cpuContains\": \"Attacker CPU\",\n"
        "  \"gpuContains\": \"Attacker GPU\"\n"
        "}\n");

    const auto factory = lap::LoadFactoryProfile((root / L"profiles").wstring(),
                                                  L"Dell Precision 5560", L"ATTACK1");
    Expect(!factory.loaded && !factory.exact && !factory.trustedProvenance,
           "unsigned mutable static factory profile cannot become factory truth");

    WriteText(root / L"profiles" / L"chassis" / L"attacker.profile",
        "profileId=attacker_precision_5560\n"
        "modelContains=Precision 5560\n"
        "displayName=Attacker-controlled Precision 5560\n"
        "validationStatus=physical-verified\n"
        "reference=attacker-controlled portable file\n"
        "port=optional_only|Optional attacker port|Left|USB-C|data|false\n");

    const auto chassis = lap::LoadChassisProfile(root.wstring(), L"Dell Precision 5560");
    Expect(!chassis.profileId.empty(), "mutable chassis fixture is discoverable for the attack-path test");
    Expect(chassis.validationStatus != L"physical-verified",
           "mutable chassis profile cannot self-assert physical verification");

    std::filesystem::remove_all(root, ec);
    return failures == 0 ? 0 : 1;
}

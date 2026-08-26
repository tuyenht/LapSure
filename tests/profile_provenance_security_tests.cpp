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

    const auto rawFactory = lap::LoadFactoryProfile((root / L"profiles").wstring(),
                                                     L"Dell Precision 5560", L"ATTACK1");
    Expect(rawFactory.profile.serviceTag == L"ATTACK1",
           "mutable static factory profile remains readable as advisory metadata");
    Expect(!rawFactory.loaded && !rawFactory.exact && !rawFactory.trustedProvenance,
           "unsigned mutable static factory profile cannot become factory truth");

    const auto decisionFactory = lap::LoadDecisionFactoryProfile((root / L"profiles").wstring(),
                                                                  L"Dell Precision 5560", L"ATTACK1");
    Expect(!decisionFactory.loaded && !decisionFactory.exact && !decisionFactory.trustedProvenance &&
               decisionFactory.profile.serviceTag.empty() && decisionFactory.profile.cpuContains.empty(),
           "decision factory boundary discards unsigned advisory expectations");

    WriteText(root / L"profiles" / L"chassis" / L"attacker.profile",
        "profileId=attacker_precision_5560\n"
        "modelContains=Precision 5560\n"
        "displayName=Attacker-controlled Precision 5560\n"
        "validationStatus=physical-verified\n"
        "reference=attacker-controlled portable file\n"
        "port=left_tb1|Attacker demoted TB4|Left|USB-C|data|false\n"
        "port=optional_only|Optional attacker port|Left|USB-C|data|false\n");

    const auto rawChassis = lap::LoadChassisProfile(root.wstring(), L"Dell Precision 5560");
    Expect(!rawChassis.profileId.empty() && rawChassis.validationStatus == L"physical-verified",
           "attack-path fixture proves raw mutable chassis metadata can self-assert verification");

    const auto decisionChassis = lap::LoadDecisionChassisProfile(root.wstring(), L"Dell Precision 5560");
    Expect(decisionChassis.validationStatus == L"static-unverified",
           "decision chassis boundary strips mutable physical-verification authority");

    unsigned requiredPorts = 0;
    bool protectedLeftTb1Required = false;
    bool attackerExtraRemainsOptional = false;
    for (const auto& port : decisionChassis.ports) {
        if (port.required) ++requiredPorts;
        if (port.id == L"left_tb1") protectedLeftTb1Required = port.required;
        if (port.id == L"optional_only") attackerExtraRemainsOptional = !port.required;
    }

    Expect(requiredPorts >= 4,
           "mutable Precision profile cannot shrink protected required-port denominator");
    Expect(protectedLeftTb1Required,
           "mutable Precision profile cannot demote a protected required port");
    Expect(attackerExtraRemainsOptional,
           "portable-only extra port remains advisory optional guidance");

    std::filesystem::remove_all(root, ec);
    return failures == 0 ? 0 : 1;
}

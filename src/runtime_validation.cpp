#include "lap/runtime_validation.h"
#include "lap/trust.h"
#include <windows.h>
#include <filesystem>

namespace lap {
namespace {
ValidationCheck C(const wchar_t*id,const wchar_t*name,ValidationStatus st,const std::wstring&detail,const std::wstring&evidence=L""){
 ValidationCheck c{};c.id=id;c.name=name;c.status=st;c.detail=detail;c.evidence=evidence;return c;
}
bool Exists(const std::filesystem::path&p){std::error_code ec;return std::filesystem::exists(p,ec);}
std::wstring Arch(){
#if defined(_M_X64) || defined(__x86_64__)
 return L"x64";
#elif defined(_M_ARM64)
 return L"arm64";
#else
 return L"x86/other";
#endif
}
}
const wchar_t* ValidationStatusText(ValidationStatus s){
 switch(s){case ValidationStatus::Pass:return L"PASS";case ValidationStatus::Warning:return L"WARNING";case ValidationStatus::Fail:return L"FAIL";default:return L"NOT RUN";}
}
void RunRuntimeValidation(AuditReport&r,const Capabilities&caps,const std::wstring&appDir){
 auto&v=r.hardware.stress.runtimeValidation;v.checks.clear();v.architecture=Arch();
#ifdef _MSC_VER
 v.compilerLabel=L"MSVC "+std::to_wstring(_MSC_VER);
#else
 v.compilerLabel=L"Non-MSVC compiler";
#endif
#ifdef NDEBUG
 v.buildLabel=L"Release";
#else
 v.buildLabel=L"Debug";
#endif
 v.checks.push_back(C(L"arch",L"64-bit target",v.architecture==L"x64"?ValidationStatus::Pass:ValidationStatus::Warning,L"Architecture="+v.architecture));
#ifdef _MSC_VER
 v.checks.push_back(C(L"compiler",L"MSVC compiler",ValidationStatus::Pass,v.compilerLabel));
#else
 v.checks.push_back(C(L"compiler",L"MSVC compiler",ValidationStatus::Warning,v.compilerLabel,L"Production acceptance target is MSVC x64."));
#endif
 v.checks.push_back(C(L"powershell",L"PowerShell capability",caps.powershell?ValidationStatus::Pass:ValidationStatus::Warning,caps.powershell?L"Available":L"Unavailable",L"Some extended providers become NOT TESTED if unavailable."));
 v.checks.push_back(C(L"profiles",L"Profiles directory",Exists(std::filesystem::path(appDir)/L"profiles")?ValidationStatus::Pass:ValidationStatus::Fail,Exists(std::filesystem::path(appDir)/L"profiles")?L"Present":L"Missing"));
 v.checks.push_back(C(L"trust_manifest",L"Trusted engine manifest",Exists(std::filesystem::path(appDir)/L"tools"/L"engine_manifest.txt")?ValidationStatus::Pass:ValidationStatus::Fail,L"External engines must remain hash-gated."));
 auto lhm=VerifyEngine(appDir,L"tools\\sensors\\lhm_bridge.exe",L"lhm_bridge");
 v.checks.push_back(C(L"cpu_sensor",L"CPU package sensor provider",lhm.hashMatches?ValidationStatus::Pass:ValidationStatus::Warning,lhm.hashMatches?L"Trusted provider ready":L"Optional trusted provider not configured",lhm.reason));
 auto mv=VerifyEngine(appDir,L"tools\\gpu\\memtest_vulkan.exe",L"memtest_vulkan");
 v.checks.push_back(C(L"vram_engine",L"VRAM integrity engine",mv.hashMatches?ValidationStatus::Pass:ValidationStatus::Warning,mv.hashMatches?L"Trusted memtest_vulkan ready":L"Trusted memtest_vulkan not configured",mv.reason));
 v.passed=v.warning=v.failed=v.notRun=0;
 for(auto&x:v.checks){switch(x.status){case ValidationStatus::Pass:v.passed++;break;case ValidationStatus::Warning:v.warning++;break;case ValidationStatus::Fail:v.failed++;break;default:v.notRun++;break;}}
 v.overall=v.failed?L"FAIL":v.warning?L"PASS WITH WARNINGS":L"PASS";
}
}
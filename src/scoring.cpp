#include "lap/scoring.h"
#include <algorithm>
namespace lap {
const wchar_t* ConfidenceText(Confidence c){switch(c){case Confidence::High:return L"HIGH";case Confidence::Medium:return L"MEDIUM";default:return L"LOW";}}
const wchar_t* VerdictText(TestVerdict v){switch(v){case TestVerdict::Pass:return L"PASS";case TestVerdict::Warning:return L"WARNING";case TestVerdict::Fail:return L"FAIL";case TestVerdict::Cancelled:return L"CANCELLED";default:return L"NOT TESTED";}}
TelemetrySummary SummarizeTelemetry(const std::vector<TelemetrySample>&v){
    TelemetrySummary s{};s.sampleCount=(unsigned)v.size();if(v.empty())return s;
    double cpu=0,gt=0,gp=0,gu=0,ct=0,cp=0;unsigned cn=0,tn=0,pn=0,un=0,ctn=0,cpn=0;
    for(auto&x:v){
        if(x.cpuUtilPercent>=0){cpu+=x.cpuUtilPercent;cn++;s.maxCpuUtil=std::max(s.maxCpuUtil,x.cpuUtilPercent);}
        if(x.gpuTempC>=0){gt+=x.gpuTempC;tn++;s.maxGpuTempC=std::max(s.maxGpuTempC,x.gpuTempC);}
        if(x.gpuPowerW>=0){gp+=x.gpuPowerW;pn++;s.maxGpuPowerW=std::max(s.maxGpuPowerW,x.gpuPowerW);}
        if(x.gpuUtilPercent>=0){gu+=x.gpuUtilPercent;un++;s.maxGpuUtil=std::max(s.maxGpuUtil,x.gpuUtilPercent);}
        if(x.cpuPackageTempC>=0){ct+=x.cpuPackageTempC;ctn++;s.maxCpuPackageTempC=std::max(s.maxCpuPackageTempC,x.cpuPackageTempC);s.cpuThermalConfidence=x.cpuThermalConfidence;}
        if(x.cpuPackagePowerW>=0){cp+=x.cpuPackagePowerW;cpn++;s.maxCpuPackagePowerW=std::max(s.maxCpuPackagePowerW,x.cpuPackagePowerW);}
        if(x.cpuThermalThrottle>0)s.cpuThrottleObserved=true;
    }
    s.avgCpuUtil=cn?cpu/cn:-1;s.avgGpuTempC=tn?gt/tn:-1;s.avgGpuPowerW=pn?gp/pn:-1;s.avgGpuUtil=un?gu/un:-1;s.avgCpuPackageTempC=ctn?ct/ctn:-1;s.avgCpuPackagePowerW=cpn?cp/cpn:-1;return s;
}
void AssessStressStage(StressStageResult&stage){
    stage.telemetrySummary=SummarizeTelemetry(stage.telemetry);
    if(stage.name.find(L"GPU")!=std::wstring::npos&&stage.telemetrySummary.maxGpuTempC>=0){
        stage.evidence+=L"; GPU max temperature="+std::to_wstring((int)stage.telemetrySummary.maxGpuTempC)+L"C";
        if(stage.telemetrySummary.maxGpuTempC>=90 && stage.verdict==TestVerdict::Pass)stage.verdict=TestVerdict::Warning;
    }
}
AuditDecision BuildAuditDecision(const AuditReport&r){
    AuditDecision d{};bool any=false,hardStageFail=false,allDeep=true;double maxGpu=-1;
    for(auto&s:r.hardware.stress.stages){
        any=true;if(s.verdict==TestVerdict::Fail)hardStageFail=true;
        if(s.verdict==TestVerdict::NotTested||s.verdict==TestVerdict::Cancelled)allDeep=false;
        maxGpu=std::max(maxGpu,s.telemetrySummary.maxGpuTempC);
    }
    for(auto&f:r.findings){
        if(f.severity==Severity::Critical&&f.state==State::Fail)d.criticalFails++;
        if(f.severity==Severity::Critical&&f.state==State::NotTested)d.criticalNotTested++;
        if(f.state==State::Warning)d.warnings++;
    }
    if(hardStageFail||d.criticalFails){
        d.overall=L"REJECT";d.stability=L"FAIL";d.confidence=Confidence::High;d.reasons.push_back(L"Critical hardware/stability failure detected.");
    }else if(!any||d.criticalNotTested||!allDeep){
        d.overall=L"INCOMPLETE";d.stability=any?L"PARTIAL":L"NOT TESTED";d.confidence=Confidence::Medium;d.reasons.push_back(L"Critical/deep tests are incomplete.");
    }else{
        d.overall=d.warnings?L"BUY WITH NOTES":L"BUY";d.stability=L"PASS";d.confidence=Confidence::High;
    }
    d.factory=r.genericMode?L"GENERIC / UNKNOWN":L"EXACT PROFILE";
    d.coverage=(!allDeep||d.criticalNotTested)?L"PARTIAL":L"HIGH";
    double maxCpu=-1;bool cpuThrottle=false;
    for(auto&s:r.hardware.stress.stages){maxCpu=std::max(maxCpu,s.telemetrySummary.maxCpuPackageTempC);cpuThrottle=cpuThrottle||s.telemetrySummary.cpuThrottleObserved;}
    if(maxCpu>=0){
        if(cpuThrottle||maxCpu>=100){d.thermal=L"CPU THERMAL / THROTTLE REVIEW";d.reasons.push_back(L"Trusted CPU telemetry reported severe temperature or throttling.");}
        else if(maxCpu>=95){d.thermal=L"CPU HOT / REVIEW";d.reasons.push_back(L"Trusted CPU package telemetry reached >=95C.");}
        else d.thermal=L"CPU TELEMETRY OK";
    }else if(maxGpu>=90){d.thermal=L"GPU HOT / REVIEW; CPU THERMAL UNKNOWN";d.reasons.push_back(L"GPU reached >=90C during sampled stress telemetry.");}
    else if(maxGpu>=0)d.thermal=L"GPU TELEMETRY OK; CPU THERMAL UNKNOWN";
    else d.thermal=L"UNKNOWN (NO RELIABLE THERMAL PROVIDER)";
    d.performance=r.hardware.stress.cpuBenchmark.verdict;
    if(r.hardware.stress.cpuBenchmark.verdict==L"BELOW BASELINE")d.reasons.push_back(L"CPU microbenchmark is below the validated local baseline for this benchmark version.");
    if(maxCpu<0)d.reasons.push_back(L"CPU package temperature/power/throttling remains unknown because no trusted provider returned data.");
    d.reasons.push_back(L"RAM online testing is partial coverage and does not equal a preboot memory certification.");
    if(r.hardware.stress.portPower.overall==L"FAIL"){
        d.overall=L"REJECT";d.reasons.push_back(L"One or more physical port stimulus tests failed.");
    }else if(r.hardware.stress.portPower.overall==L"INCOMPLETE"){
        if(d.overall==L"BUY")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";
        d.reasons.push_back(L"Physical port / power verification is incomplete.");
    }
    if(r.hardware.stress.functional.failed>0){
        d.overall=L"REJECT";d.confidence=Confidence::High;
        d.reasons.push_back(L"One or more functional hardware tests failed.");
    }else if(r.hardware.stress.functional.manualRequired>0||r.hardware.stress.functional.notTested>0){
        if(d.overall==L"BUY")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";
        d.reasons.push_back(L"Functional Test Center still contains manual or untested items.");
    }
    return d;
}
}

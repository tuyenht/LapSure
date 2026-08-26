#include "lap/scoring.h"
#include "lap/chassis_profile.h"
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
std::vector<CoverageDomain> BuildCoverageContract(const AuditReport&r){
    std::vector<CoverageDomain> out;
    auto add=[&](const wchar_t*id,const wchar_t*name,bool complete,const wchar_t*sources,const wchar_t*missing,bool required=true){out.push_back({id,name,complete?L"COMPLETE":L"PARTIAL",required,sources,complete?L"":missing});};
    const auto&claim=r.sellerClaim;const bool claimComplete=claim.provided&&!claim.model.empty()&&!claim.cpuContains.empty()&&claim.ramBytes>0&&claim.storageBytes>0;
    add(L"seller_claim",L"Cấu hình người bán cam kết",claimComplete,L"Biểu mẫu người bán + đối chiếu bằng chứng LapSure",L"Thiếu model, CPU, RAM hoặc dung lượng ổ do người bán cam kết",false);
    add(L"identity",L"Thông tin nhận diện máy",!r.model.empty()&&!r.serviceTag.empty()&&!r.hardware.cpuName.empty(),L"BIOS registry + CIM",L"Thiếu tên máy, Service Tag hoặc thông tin bộ xử lý");
    add(L"memory",L"Bộ nhớ RAM",r.hardware.installedRamBytes>0&&!r.hardware.memoryModules.empty(),L"GlobalMemoryStatusEx + CIM",L"Thiếu dung lượng RAM hoặc thông tin thanh RAM vật lý");
    const bool storage=!r.hardware.storage.empty()&&std::all_of(r.hardware.storage.begin(),r.hardware.storage.end(),[](const auto&d){return !d.model.empty()&&d.capacityBytes>0&&d.reliabilityReadable;});
    add(L"storage",L"Ổ lưu trữ và sức khỏe ổ",storage,L"CIM + Windows Storage Reliability; SMART là dữ liệu bổ sung",L"Có ổ đĩa thiếu nhận diện, dung lượng hoặc bằng chứng sức khỏe");
    const bool battery=!r.hardware.battery.present||(r.hardware.battery.capacityReadable&&r.hardware.battery.healthPercent>=0);
    add(L"battery",L"Pin",battery,L"CIM + báo cáo pin Windows",L"Thiếu dung lượng hoặc sức khỏe pin",r.hardware.battery.present);
    add(L"graphics",L"Bộ xử lý đồ họa",!r.hardware.gpus.empty(),L"CIM; dữ liệu hãng là phần bổ sung",L"Chưa nhận diện được bộ xử lý đồ họa");
    add(L"display",L"Màn hình",!r.hardware.displays.empty(),L"EDID gốc + cấu hình hiển thị",L"Chưa có bằng chứng EDID/màn hình hợp lệ");
    const bool stability=!r.hardware.stress.stages.empty()&&std::all_of(r.hardware.stress.stages.begin(),r.hardware.stress.stages.end(),[](const auto&s){return s.verdict!=TestVerdict::NotTested&&s.verdict!=TestVerdict::Cancelled;});
    add(L"stability",L"Độ ổn định CPU, RAM và GPU",stability,L"Các bài tải LapSure + sự kiện phát sinh",L"Còn bài kiểm tra tải bắt buộc chưa hoàn tất");
    bool thermal=false;for(const auto&s:r.hardware.stress.stages)thermal=thermal||s.telemetrySummary.maxCpuPackageTempC>=0||s.telemetrySummary.maxGpuTempC>=0;
    add(L"thermals",L"Nhiệt độ và giảm hiệu năng do nóng",thermal,L"Cảm biến tin cậy trong khi chạy tải",L"Chưa có mẫu nhiệt độ/giảm xung đủ tin cậy");
    unsigned physicalCompleted=0;for(const auto&x:r.hardware.stress.functional.items)if(x.id.rfind(L"physical_",0)==0&&x.status!=FunctionalStatus::NotTested&&x.status!=FunctionalStatus::ManualRequired)physicalCompleted++;
    const bool functional=!r.hardware.stress.functional.items.empty()&&r.hardware.stress.functional.notTested==0&&r.hardware.stress.functional.manualRequired==0&&physicalCompleted>=6;
    add(L"functional",L"Các thiết bị, ngoại hình và chức năng",functional,L"Kiểm tra tự động + hướng dẫn người dùng",physicalCompleted<6?L"Chưa hoàn tất kiểm tra ngoại hình/an toàn vật lý":L"Còn chức năng cần người dùng xác nhận hoặc chưa kiểm tra");
    add(L"ports_power",L"Cổng kết nối và nguồn sạc",r.hardware.stress.portPower.overall==L"PASS",L"So sánh thiết bị trước/sau + cắm thử thực tế",L"Còn cổng hoặc nguồn sạc bắt buộc chưa kiểm tra");
    add(L"runtime",L"Tính toàn vẹn chương trình và báo cáo",r.hardware.stress.runtimeValidation.overall==L"PASS",L"Cổng xác thực nội bộ LapSure",L"Xác thực khi chạy chưa đạt");
    return out;
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
    const auto requiredPortsRemaining=RequiredPortsRemaining(r.hardware.stress.chassisProfile);
    if(requiredPortsRemaining>0){
        if(d.overall==L"BUY"||d.overall==L"BUY WITH NOTES")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";d.confidence=Confidence::Medium;
        d.reasons.push_back(std::to_wstring(requiredPortsRemaining)+L" required physical port(s) remain untested.");
    }
    if(!r.hardware.stress.chassisProfile.profileId.empty()&&r.hardware.stress.chassisProfile.validationStatus!=L"physical-verified"){
        if(d.overall==L"BUY"||d.overall==L"BUY WITH NOTES")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";d.reasons.push_back(L"Chassis profile is not physical-verified.");
    }
    if(r.hardware.stress.functional.failed>0){
        d.overall=L"REJECT";d.confidence=Confidence::High;
        d.reasons.push_back(L"One or more functional hardware tests failed.");
    }else if(r.hardware.stress.functional.manualRequired>0||r.hardware.stress.functional.notTested>0){
        if(d.overall==L"BUY")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";
        d.reasons.push_back(L"Functional Test Center still contains manual or untested items.");
    }
    if(r.hardware.stress.runtimeValidation.failed>0){
        if(d.overall!=L"REJECT")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";
        if(d.overall!=L"REJECT")d.confidence=Confidence::Low;
        d.reasons.push_back(L"Runtime validation failed; this build cannot issue an acceptance verdict.");
    }else if(r.hardware.stress.runtimeValidation.notRun>0||r.hardware.stress.runtimeValidation.overall==L"NOT RUN"){
        if(d.overall==L"BUY"||d.overall==L"BUY WITH NOTES")d.overall=L"INCOMPLETE";
        d.coverage=L"PARTIAL";
        d.reasons.push_back(L"Runtime validation has not completed.");
    }
    const auto contract=BuildCoverageContract(r);const auto missing=std::count_if(contract.begin(),contract.end(),[](const auto&x){return x.required&&x.status!=L"COMPLETE";});
    if(missing>0){if(d.overall==L"BUY"||d.overall==L"BUY WITH NOTES")d.overall=L"INCOMPLETE";d.coverage=L"PARTIAL";d.reasons.push_back(std::to_wstring(missing)+L" required coverage domain(s) remain incomplete; see the coverage contract.");}
    return d;
}
}

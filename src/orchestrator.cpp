#include "lap/orchestrator.h"
#include "lap/chassis_profile.h"
#include <algorithm>
namespace lap {
const wchar_t* StageStateText(TestStageState s){switch(s){case TestStageState::Locked:return L"LOCKED";case TestStageState::Ready:return L"READY";case TestStageState::Running:return L"RUNNING";case TestStageState::Passed:return L"PASS";case TestStageState::Warning:return L"WARNING";case TestStageState::Failed:return L"FAIL";case TestStageState::Incomplete:return L"INCOMPLETE";}return L"UNKNOWN";}
void BuildOrchestrator(AuditReport&r,bool running,bool ready){
 auto&o=r.hardware.stress.orchestrator;o.stages.clear();
 auto add=[&](const wchar_t*id,const wchar_t*t,const wchar_t*sub,TestStageState st,unsigned c,unsigned total,bool op,const wchar_t*a){TestStageView v{};v.id=id;v.title=t;v.subtitle=sub;v.state=st;v.completed=c;v.total=total;v.requiresOperator=op;v.actionLabel=a;o.stages.push_back(std::move(v));};
 add(L"automatic",L"1. Automatic hardware audit",L"Inventory, factory comparison, health, telemetry and stress evidence",running?TestStageState::Running:ready?TestStageState::Passed:TestStageState::Ready,ready?1:0,1,false,L"RUN FULL AUDIT");
 auto&f=r.hardware.stress.functional;auto fs=!ready?TestStageState::Locked:f.failed?TestStageState::Failed:(f.manualRequired||f.notTested)?TestStageState::Incomplete:f.warning?TestStageState::Warning:TestStageState::Passed;
 add(L"functional",L"2. Functional verification",L"Display, keyboard, touch, camera, mic, stereo, Wi-Fi, Bluetooth",fs,f.passed+f.failed+f.warning,(unsigned)f.items.size(),true,L"CONTINUE FUNCTION TESTS");
 auto&p=r.hardware.stress.portPower;auto ps=!ready?TestStageState::Locked:p.overall==L"FAIL"?TestStageState::Failed:p.overall==L"PASS"?TestStageState::Passed:TestStageState::Incomplete;
 unsigned requiredRemaining=RequiredPortsRemaining(r.hardware.stress.chassisProfile),requiredTotal=(unsigned)r.hardware.stress.chassisProfile.ports.size();unsigned requiredDone=requiredTotal>requiredRemaining?requiredTotal-requiredRemaining:0;if(!requiredTotal){requiredTotal=std::max(1u,(unsigned)p.ports.size());requiredDone=(unsigned)p.ports.size();}
 add(L"ports",L"3. Physical ports & power",L"Model-aware required-port verification, USB4/Thunderbolt and AC evidence",ps,requiredDone,requiredTotal,true,L"TEST NEXT PORT");
 auto d=r.hardware.stress.decision.overall;auto ds=!ready?TestStageState::Locked:d==L"REJECT"?TestStageState::Failed:d==L"BUY"?TestStageState::Passed:TestStageState::Incomplete;
 add(L"decision",L"4. Final review",L"Coverage, confidence, factory mismatch and purchase verdict",ds,ready?1:0,1,false,L"OPEN REPORT");
 o.totalStages=(unsigned)o.stages.size();unsigned u=0,t=0;o.completedStages=0;
 for(auto&x:o.stages){t+=std::max(1u,x.total);u+=std::min(x.completed,std::max(1u,x.total));if(x.state==TestStageState::Passed||x.state==TestStageState::Warning||x.state==TestStageState::Failed)o.completedStages++;}
 o.percent=t?unsigned(100ull*u/t):0;
 if(!ready){o.nextAction=running?L"Wait for automatic audit to finish":L"Run Full Audit";o.overall=running?L"RUNNING":L"NOT STARTED";}
 else if(f.manualRequired||f.notTested){o.nextAction=L"Complete remaining functional tests";o.overall=L"IN PROGRESS";}
 else if(requiredRemaining>0||p.overall!=L"PASS"){o.nextAction=L"Verify next required physical port";o.overall=L"IN PROGRESS";}
 else{o.nextAction=L"Review final report";o.overall=d;}
}}

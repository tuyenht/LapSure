from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f"{label} anchor not found")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# Interaction hardening: controls may only react where they are actually drawn.
# ---------------------------------------------------------------------------
main_path = ROOT / "src" / "main.cpp"
main = main_path.read_text(encoding="utf-8")
main = replace_once(
    main,
    '''        case VK_LEFT:\n            if (gFocusIndex == 1) {\n''',
    '''        case VK_LEFT:\n            if (gFocusIndex == 1 && (gCurrentTab == MainTab::Dashboard || gCurrentTab == MainTab::NewSession)) {\n''',
    "left-mode keyboard gate",
)
main = replace_once(
    main,
    '''        case VK_RIGHT:\n            if (gFocusIndex == 1) {\n''',
    '''        case VK_RIGHT:\n            if (gFocusIndex == 1 && (gCurrentTab == MainTab::Dashboard || gCurrentTab == MainTab::NewSession)) {\n''',
    "right-mode keyboard gate",
)
main = replace_once(
    main,
    '''            if (actionFocus == 2) {\n                // Focus selects the visible top-level CTA; the current screen still\n                // decides whether that CTA is an audit action. No global StartAudit.\n                switch (gCurrentTab) {\n                case MainTab::Dashboard:\n                case MainTab::AutoAudit:\n                case MainTab::NewSession:\n                case MainTab::Stress:\n                    StartAudit(h);\n                    break;\n                default:\n                    break;\n                }\n                return 0;\n            }\n''',
    '''            if (actionFocus == 2) {\n                // Focus slot 2 belongs to the S01 top-level Start/Stop button only.\n                // Other screens must never inherit this invisible operation target.\n                if (gCurrentTab == MainTab::Dashboard) StartAudit(h);\n                return 0;\n            }\n''',
    "focus-2 dashboard-only gate",
)
mouse_start = main.find("        // 2. Start / Stop Button Click\n")
mouse_end = main.find("        // 3.1 S01 Dashboard Specific Click Hit-Tests\n", mouse_start)
if mouse_start < 0 or mouse_end < 0:
    raise SystemExit("S01 top strip mouse markers not found")
new_mouse = '''        // 2–3. S01 top mode strip and Start/Stop button. These hit regions exist\n        // only when the Dashboard renderer actually draws the matching controls.\n        int modeY = layout.contentRect.top + UiMetrics::Scale(70, dpi);\n        if (gCurrentTab == MainTab::Dashboard) {\n            int btnW = UiMetrics::Scale(230, dpi);\n            int btnH = UiMetrics::Scale(40, dpi);\n            RECT btnRect{ cr.right - btnW - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi), cr.right - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi) + btnH };\n            if (x >= btnRect.left && x <= btnRect.right && y >= btnRect.top && y <= btnRect.bottom) {\n                StartAudit(h);\n                return 0;\n            }\n\n            int mX = layout.contentRect.left + UiMetrics::Scale(134, dpi);\n            int pillW = UiMetrics::Scale(80, dpi);\n            int pillH = UiMetrics::Scale(28, dpi);\n            int gap = UiMetrics::Scale(6, dpi);\n            if (y >= modeY && y <= modeY + pillH) {\n                if (x >= mX && x <= mX + pillW) { gSelectedMode = L"Quick"; InvalidateRect(h, nullptr, FALSE); }\n                else if (x >= mX + pillW + gap && x <= mX + (pillW + gap) * 2) { gSelectedMode = L"Standard"; InvalidateRect(h, nullptr, FALSE); }\n                else if (x >= mX + (pillW + gap) * 2 && x <= mX + (pillW + gap) * 3) { gSelectedMode = L"Deep"; InvalidateRect(h, nullptr, FALSE); }\n            }\n        }\n\n'''
main = main[:mouse_start] + new_mouse + main[mouse_end:]
main_path.write_text(main, encoding="utf-8")


# ---------------------------------------------------------------------------
# Bundled diagnostic engines: re-verify at the execution boundary and launch
# only the canonical allowlisted path through the explicit process API.
# ---------------------------------------------------------------------------
eng_path = ROOT / "src" / "engines.cpp"
eng = eng_path.read_text(encoding="utf-8")
if '#include "lap/trust.h"' not in eng:
    eng = replace_once(eng, '#include "lap/process.h"\n', '#include "lap/process.h"\n#include "lap/trust.h"\n', "engines trust include")

smart_start = eng.find("void CollectSmartctl(")
smart_end = eng.find("void CollectNvidia(", smart_start)
if smart_start < 0 or smart_end < 0:
    raise SystemExit("CollectSmartctl function markers not found")
smart_fn = r'''void CollectSmartctl(AuditReport&r,const FactoryProfile&,const Capabilities&c,const std::wstring&dir,const std::atomic_bool* cancel){
 if(!c.smartctl){const bool native=!r.hardware.storage.empty()&&std::all_of(r.hardware.storage.begin(),r.hardware.storage.end(),[](const auto&d){return d.reliabilityReadable;});Add(r,L"Storage",L"Advanced SMART/NVMe log",L"smartctl unavailable",L"Advanced controller log",State::NotTested,native?Severity::Minor:Severity::Critical,Dimension::Health,native?L"Advanced enrichment unavailable; native reliability evidence remains available":L"No storage-health provider available");return;}
 auto scanRun=RunTrustedEngineCapture(dir,L"tools\\smartctl.exe",L"smartctl",{L"--scan-open"},15000,cancel);
 if(!scanRun.trust.hashMatches){Add(r,L"Storage",L"SMART trust gate",scanRun.trust.reason,L"Trusted smartctl SHA-256",State::NotTested,Severity::Critical,Dimension::Evidence,L"Execution blocked before launch");return;}
 const auto& scan=scanRun.process;
 if(!scan.launched||scan.timedOut||scan.output.empty()){Add(r,L"Storage",L"SMART device scan",scan.error.empty()?L"No devices returned":scan.error,L"Detected storage",State::Warning,Severity::Major,Dimension::Health,L"Trusted smartctl SHA256="+scanRun.trust.sha256);return;}
 std::vector<std::wstring> devices;for(auto&line:SplitLines(scan.output)){auto d=FirstToken(line);if(!d.empty()&&d[0]==L'/')devices.push_back(d);}
 Add(r,L"Storage",L"SMART devices",std::to_wstring(devices.size())+L" device(s)",L"Detected storage",devices.empty()?State::Warning:State::Pass,Severity::Major,Dimension::Identity,L"trusted smartctl --scan-open; SHA256="+scanRun.trust.sha256);
 for(const auto&dev:devices){
   auto run=RunTrustedEngineCapture(dir,L"tools\\smartctl.exe",L"smartctl",{L"-a",L"-j",dev},30000,cancel);
   if(!run.trust.hashMatches){Add(r,L"Storage",L"SMART "+dev,run.trust.reason,L"Trusted smartctl SHA-256",State::NotTested,Severity::Critical,Dimension::Evidence,L"Engine changed or trust gate failed before device query");continue;}
   const auto& pr=run.process;
   if(!pr.launched||pr.timedOut||pr.output.empty()){Add(r,L"Storage",L"SMART "+dev,pr.error.empty()?L"Could not read":pr.error,L"Readable SMART",State::Warning,Severity::Critical,Dimension::Health,L"Trusted smartctl SHA256="+run.trust.sha256);continue;}
   StorageDevice parsed{};std::wstring parseError;if(!ParseSmartctlHealthJson(pr.output,parsed,parseError)){Add(r,L"Storage",L"SMART "+dev,parseError,L"Explicit SMART health schema",State::NotTested,Severity::Critical,Dimension::Health,L"smartctl JSON rejected");continue;}
   auto model=parsed.model,serial=parsed.serialNumber,fw=parsed.firmware;auto crit=parsed.criticalWarning,used=parsed.percentageUsed,spare=parsed.availableSpare,spareTh=parsed.spareThreshold;
   auto media=parsed.mediaErrors,errlog=parsed.errorLogEntries,unsafe=parsed.unsafeShutdowns,hours=parsed.powerOnHours,cycles=parsed.powerCycles;
   auto temp=parsed.temperatureC,readUnits=parsed.dataUnitsRead,writeUnits=parsed.dataUnitsWritten;bool smart=parsed.smartPassed;

   StorageDevice* sd=MatchStorage(r,model,serial);sd->devicePath=dev;if(!model.empty())sd->model=model;if(!serial.empty())sd->serialNumber=serial;if(!fw.empty())sd->firmware=fw;
   sd->smartReadable=true;sd->smartPassed=smart;sd->criticalWarning=crit;sd->percentageUsed=used;sd->enduranceRemaining=(used>=0?std::max<long long>(0,100-used):-1);sd->availableSpare=spare;sd->spareThreshold=spareTh;
   sd->mediaErrors=media;sd->errorLogEntries=errlog;sd->unsafeShutdowns=unsafe;sd->powerOnHours=hours;sd->powerCycles=cycles;sd->temperatureC=temp;sd->dataUnitsRead=readUnits;sd->dataUnitsWritten=writeUnits;
   sd->approxDataReadTB=NvmeDataUnitsToTB(readUnits);sd->approxDataWrittenTB=NvmeDataUnitsToTB(writeUnits);

   State health=(!smart||crit>0||media>0)?State::Fail:(used>=20?State::Warning:(used>=10?State::Good:State::Pass));
   Add(r,L"Storage",L"Identity "+dev,sd->model+L" | SN "+sd->serialNumber+L" | FW "+sd->firmware,L"",State::Info,Severity::Info,Dimension::Identity,L"trusted smartctl SHA256="+run.trust.sha256);
   std::wstringstream h;h<<L"SMART="<<(smart?L"PASS":L"FAIL")<<L" | Endurance remaining="<<Fmt(sd->enduranceRemaining,L"%")<<L" | Used="<<Fmt(used,L"%")<<L" | Critical="<<Fmt(crit)<<L" | MediaErrors="<<Fmt(media)<<L" | Spare="<<Fmt(spare,L"%")<<L" | Temp="<<Fmt(temp,L" C");
   Add(r,L"Storage",L"Health "+dev,h.str(),L"Critical=0; MediaErrors=0",health,Severity::Critical,Dimension::Health,L"smartctl JSON; trusted SHA256="+run.trust.sha256);
   std::wstringstream u;u<<L"Power-on "<<Fmt(hours,L" h")<<L" | Cycles "<<Fmt(cycles)<<L" | Unsafe shutdowns "<<Fmt(unsafe)<<L" | Read "<<Fmt1(sd->approxDataReadTB,L" TB")<<L" | Written "<<Fmt1(sd->approxDataWrittenTB,L" TB")<<L" | Error log "<<Fmt(errlog);
   Add(r,L"Storage",L"Usage "+dev,u.str(),L"",State::Info,Severity::Info,Dimension::Usage,L"trusted smartctl SHA256="+run.trust.sha256);
 }
}

'''
eng = eng[:smart_start] + smart_fn + eng[smart_end:]

nvidia_start = eng.find("void CollectNvidia(")
nvidia_end = eng.find("void CollectVolumeIntegrityAudit(", nvidia_start)
if nvidia_start < 0 or nvidia_end < 0:
    raise SystemExit("CollectNvidia function markers not found")
nvidia_fn = r'''void CollectNvidia(AuditReport&r,const FactoryProfile&p,const Capabilities&c,const std::wstring&dir,const std::atomic_bool* cancel){
 if(!c.nvidiaSmi){Add(r,L"GPU",L"NVIDIA telemetry",L"nvidia-smi unavailable",L"GPU/VRAM telemetry",State::NotTested,Severity::Major,Dimension::Health);return;}
 auto run=RunTrustedEngineCapture(dir,L"tools\\nvidia-smi.exe",L"nvidia_smi",
   {L"--query-gpu=name,serial,uuid,vbios_version,driver_version,memory.total,temperature.gpu,temperature.gpu.tlimit,pstate,power.draw,power.limit,utilization.gpu,utilization.memory",L"--format=csv,noheader,nounits"},15000,cancel);
 if(!run.trust.hashMatches){Add(r,L"GPU",L"NVIDIA trust gate",run.trust.reason,L"Trusted nvidia-smi SHA-256",State::NotTested,Severity::Major,Dimension::Evidence,L"Execution blocked before launch");return;}
 const auto& pr=run.process;
 if(!pr.launched||pr.timedOut||pr.output.empty()){Add(r,L"GPU",L"NVIDIA telemetry",pr.error.empty()?L"No output":pr.error,L"",State::Warning,Severity::Major,Dimension::Health,L"Trusted nvidia-smi SHA256="+run.trust.sha256);return;}
 size_t parsed=0;
 for(const auto&line:SplitLines(pr.output)){
   GpuInfo g{};if(!ParseNvidiaCsvLine(line,g))continue;++parsed;auto*existing=MatchGpu(r,g.name);*existing=g;
   State factory=p.gpuContains.empty()?State::Info:(ContainsI(g.name,p.gpuContains)?State::Pass:State::Fail);
   Add(r,L"GPU",L"Adapter",g.name,p.gpuContains,factory,p.gpuContains.empty()?Severity::Info:Severity::Critical,p.gpuContains.empty()?Dimension::Identity:Dimension::Factory,L"trusted nvidia-smi SHA256="+run.trust.sha256);
   std::wstringstream d;d<<Fmt1((double)g.vramBytes/(1024.0*1024.0*1024.0),L" GiB")<<L" VRAM | Driver "<<g.driver<<L" | VBIOS "<<g.vbios;
   Add(r,L"GPU",L"Identity / VRAM",d.str(),p.gpuVramBytes?Fmt1((double)p.gpuVramBytes/(1024.0*1024.0*1024.0),L" GiB"):L"",State::Info,Severity::Major,Dimension::Identity,L"trusted nvidia-smi SHA256="+run.trust.sha256);
   std::wstringstream t;t<<L"Temp "<<Fmt1(g.temperatureC,L" C")<<L" / limit "<<Fmt1(g.tempLimitC,L" C")<<L" | "<<g.pstate<<L" | Power "<<Fmt1(g.powerW,L" W")<<L" / "<<Fmt1(g.powerLimitW,L" W")<<L" | GPU "<<Fmt1(g.gpuUtilPercent,L"%")<<L" | VRAM "<<Fmt1(g.memoryUtilPercent,L"%");
   Add(r,L"GPU",L"Live telemetry",t.str(),L"",State::Info,Severity::Info,Dimension::Usage,L"trusted nvidia-smi SHA256="+run.trust.sha256);
 }
 if(!parsed)Add(r,L"GPU",L"NVIDIA parse",L"Output present but no valid CSV row parsed",L"",State::Warning,Severity::Major,Dimension::Evidence,pr.output);
}


'''
eng = eng[:nvidia_start] + nvidia_fn + eng[nvidia_end:]
eng_path.write_text(eng, encoding="utf-8")


# ---------------------------------------------------------------------------
# GPU/VRAM stress engine: preflight trust for UX, then re-verify again exactly
# at execution through RunTrustedEngineCapture.
# ---------------------------------------------------------------------------
stress_path = ROOT / "src" / "stress.cpp"
stress = stress_path.read_text(encoding="utf-8")
stress = stress.replace('std::wstring Q(const std::wstring&s){return L"\\\""+s+L"\\\"";}\n', '', 1)
mem_start = stress.find("StressStageResult MemtestVulkanStage(")
mem_end = stress.find("\n}\n\nGpuVramMetrics ParseMemtestVulkanOutput", mem_start)
if mem_start < 0 or mem_end < 0:
    raise SystemExit("MemtestVulkanStage function markers not found")
mem_fn = r'''StressStageResult MemtestVulkanStage(const Capabilities&caps,const std::wstring&appDir,const std::wstring&sessionId,unsigned seconds,const std::atomic_bool*cancel){
 StressStageResult out{};out.name=L"GPU / VRAM integrity";out.plannedSeconds=seconds;
 const std::wstring rel=L"tools\\gpu\\memtest_vulkan.exe";
 auto preTrust=VerifyEngine(appDir,rel,L"memtest_vulkan");
 if(!preTrust.fileExists||!preTrust.manifestEntry||!preTrust.hashMatches){out.verdict=TestVerdict::NotTested;out.evidence=L"memtest_vulkan trust gate failed: "+preTrust.reason+L" SHA256="+preTrust.sha256;return out;}
 WriteStressJournal(appDir,sessionId,out.name,L"RUNNING");auto before=SnapshotEvents(caps,cancel);auto start=std::chrono::steady_clock::now();
 auto run=RunTrustedEngineCapture(appDir,rel,L"memtest_vulkan",{},seconds*1000+30000,cancel);
 if(!run.trust.hashMatches){out.verdict=TestVerdict::NotTested;out.evidence=L"memtest_vulkan changed before execution: "+run.trust.reason;WriteStressJournal(appDir,sessionId,out.name,L"NOT_TESTED");return out;}
 const auto& p=run.process;auto after=SnapshotEvents(caps,cancel);auto elapsed=(unsigned)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-start).count();
 out=Finish(out.name,seconds,elapsed,before,after,cancel&&cancel->load(),p.timedOut,L"Trusted memtest_vulkan SHA256="+run.trust.sha256+L"; exit="+std::to_wstring(p.exitCode));
 out.gpuVram=ParseMemtestVulkanOutputImpl(p.output);if(!p.launched)out.verdict=TestVerdict::NotTested;else if(p.timedOut||p.exitCode!=0||out.gpuVram.errors>0)out.verdict=TestVerdict::Fail;else if(out.gpuVram.standardFiveMinutePassed&&out.newWhea==0&&out.newDisplay==0&&out.newBugCheck==0)out.verdict=TestVerdict::Pass;else out.verdict=TestVerdict::Warning;
 out.evidence+=L"; VRAM errors="+std::to_wstring(out.gpuVram.errors)+L"; standard5min="+(out.gpuVram.standardFiveMinutePassed?L"yes":L"no");WriteStressJournal(appDir,sessionId,out.name,out.verdict==TestVerdict::Pass?L"COMPLETED":out.verdict==TestVerdict::Fail?L"FAILED":L"INCOMPLETE");return out;
}'''
stress = stress[:mem_start] + mem_fn + stress[mem_end:]
stress_path.write_text(stress, encoding="utf-8")

print("Round 4 runtime batch applied")

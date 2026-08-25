from pathlib import Path
from app_source_view import read_app_source

R = Path(__file__).resolve().parents[1]
journal_h = (R / "include/lap/journal.h").read_text(encoding="utf-8")
journal = (R / "src/journal.cpp").read_text(encoding="utf-8")
stress = (R / "src/stress.cpp").read_text(encoding="utf-8")
history_h = (R / "include/lap/session_history.h").read_text(encoding="utf-8")
history = (R / "src/session_history.cpp").read_text(encoding="utf-8")
report = (R / "src/report.cpp").read_text(encoding="utf-8")
publication = (R / "src/report_publication.cpp").read_text(encoding="utf-8")
app = read_app_source(R)
behavior = (R / "tests/behavioral_tests.cpp").read_text(encoding="utf-8")
publication_tests = (R / "tests/report_publication_tests.cpp").read_text(encoding="utf-8")

checks = [
    ("journal exposes stage status separately", "stageStatus" in journal_h and 'ValueOf(info.rawEvidence, L"stage_status")' in journal),
    ("journal keeps session RUNNING while stages advance", 'L"\\nstatus=RUNNING"' in journal and 'L"\\nstage_status=" << status' in journal),
    ("stress uses explicit resolved state root", "const auto stateRoot=ResolveReportDirectory" in stress and "WriteStressJournal(stateRoot" in stress),
    ("orderly stress cancellation discards active journal", "DiscardInterruptedStressJournal(stateRoot)" in stress),
    ("history supports bundle commit", "CommitSessionHistoryBundle" in history_h and "CommitSessionHistoryBundle" in history),
    ("history distinguishes partial artifact bundle", 'L"ARTIFACT_PARTIAL"' in history),
    ("individual report files use atomic temp replacement", "MOVEFILE_REPLACE_EXISTING" in report and "MOVEFILE_WRITE_THROUGH" in report and 'L".tmp"' in report),
    ("report bundle stages both artifacts before one publication move", ".staging-" in publication and "MoveFileExW(staging.c_str(), published.c_str(), MOVEFILE_WRITE_THROUGH)" in publication),
    ("history commit belongs to publication boundary", "CommitSessionHistoryBundle(report, finalHtml.wstring(), finalJson.wstring())" in publication),
    ("hardware decision is not rewritten by report I/O failure", "MarkReportPersistenceIncomplete" not in app and 'decision.overall = L"INCOMPLETE"' not in app),
    ("stress completion truth is independent of report filesystem", "CompleteStressJournal(gReportOutputDir)" in app and "PublishReportSnapshot(report)" in app and app.index("CompleteStressJournal(gReportOutputDir)") < app.index("PublishReportSnapshot(report)")),
    ("audit readiness and publication readiness are separate", "gAuditReady = !gCancel" in app and "gPublicationReady" in app),
    ("compiled publication failure injection exists", all(token in publication_tests for token in ["FailJsonStage", "FailFinalPublish", "FailHistoryCommit", "does not mutate hardware decision"])),
    ("behavioral recovery coverage remains", "journal remains recoverable after completed stage" in behavior and "history bundle becomes COMPLETE only with HTML and JSON" in behavior),
]

bad = []
for name, ok in checks:
    print(("PASS" if ok else "FAIL"), name)
    if not ok:
        bad.append(name)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))

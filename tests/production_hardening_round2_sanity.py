from pathlib import Path

R = Path(__file__).resolve().parents[1]
journal_h = (R / "include/lap/journal.h").read_text(encoding="utf-8")
journal = (R / "src/journal.cpp").read_text(encoding="utf-8")
stress = (R / "src/stress.cpp").read_text(encoding="utf-8")
history_h = (R / "include/lap/session_history.h").read_text(encoding="utf-8")
history = (R / "src/session_history.cpp").read_text(encoding="utf-8")
report = (R / "src/report.cpp").read_text(encoding="utf-8")
main = (R / "src/main.cpp").read_text(encoding="utf-8")
behavior = (R / "tests/behavioral_tests.cpp").read_text(encoding="utf-8")

checks = [
    ("journal exposes stage status separately", "stageStatus" in journal_h and 'ValueOf(info.rawEvidence, L"stage_status")' in journal),
    ("journal keeps session RUNNING while stages advance", 'L"\\nstatus=RUNNING"' in journal and 'L"\\nstage_status=" << status' in journal),
    ("stress no longer completes journal before report persistence", "CompleteStressJournal(appDir)" not in stress),
    ("orderly cancellation discards active journal", "DiscardInterruptedStressJournal(appDir)" in stress),
    ("history supports bundle commit", "CommitSessionHistoryBundle" in history_h and "CommitSessionHistoryBundle" in history),
    ("history distinguishes partial artifact bundle", 'L"ARTIFACT_PARTIAL"' in history),
    ("report writes atomically", "MOVEFILE_REPLACE_EXISTING" in report and "MOVEFILE_WRITE_THROUGH" in report and 'L".tmp"' in report),
    ("audit completes journal only after transactional report persistence", "CommitSessionHistoryBundle(report, result.htmlPath, result.jsonPath)" in main and "PersistReportBundle(report, out)" in main and "if (persisted.Complete()) CompleteStressJournal(gDir)" in main),
    ("report persistence failure cannot publish clean acceptance lifecycle", "MarkReportPersistenceIncomplete" in main),
    ("behavioral recovery transaction coverage exists", "journal remains recoverable after completed stage" in behavior and "history bundle becomes COMPLETE only with HTML and JSON" in behavior),
]

bad = []
for name, ok in checks:
    print(("PASS" if ok else "FAIL"), name)
    if not ok:
        bad.append(name)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))

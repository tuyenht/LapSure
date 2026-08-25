from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
MODEL = (ROOT / "include" / "lap" / "model.h").read_text(encoding="utf-8")
STRESS = (ROOT / "src" / "stress.cpp").read_text(encoding="utf-8")
REPORT = (ROOT / "src" / "report.cpp").read_text(encoding="utf-8")
PUBLICATION = (ROOT / "src" / "report_publication.cpp").read_text(encoding="utf-8")
HISTORY_H = (ROOT / "include" / "lap" / "session_history.h").read_text(encoding="utf-8")
HISTORY = (ROOT / "src" / "session_history.cpp").read_text(encoding="utf-8")
JOURNAL_H = (ROOT / "include" / "lap" / "journal.h").read_text(encoding="utf-8")
JOURNAL = (ROOT / "src" / "journal.cpp").read_text(encoding="utf-8")
UI = (ROOT / "src" / "ui_screens_s22_s23_v2.cpp").read_text(encoding="utf-8")
MAIN = read_app_source(ROOT)
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

assert "std::wstring sessionId;" in MODEL
assert "ss.sessionId=SessionId()" in STRESS
assert "ReportStem(const AuditReport&r)" in REPORT
assert "r.hardware.stress.sessionId" in REPORT
assert "CommitSessionHistoryBundle" in HISTORY_H
assert "CommitSessionHistoryBundle" in HISTORY
assert "PublishReportBundle" in MAIN and "CommitSessionHistoryBundle" in PUBLICATION
assert 'L"audit_"+ReportStem(r)+L".html"' in REPORT
assert 'L"audit_"+ReportStem(r)+L".json"' in REPORT

assert "struct SessionHistoryEntry" in HISTORY_H
assert "session_history.tsv" in HISTORY
assert "MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH" in HISTORY
assert "GetSessionHistorySnapshot()" in UI
assert "if (history.empty())" in UI
assert "CanonicalUiState::Empty" in UI
assert "Không tạo phiên mẫu" in UI
assert "không có lịch sử cloud" in UI
assert "DeleteSessionHistoryEntry" in MAIN
assert "YES: xóa mục lịch sử và artifact" in MAIN
assert "NO: chỉ xóa mục index" in MAIN

assert "InterruptedSessionInfo" in JOURNAL_H
assert "ReadInterruptedStressJournal" in JOURNAL
assert "DiscardInterruptedStressJournal" in JOURNAL
assert "ArchiveInterruptedSession" in HISTORY
assert 'e->status = L"INTERRUPTED"' in HISTORY
assert 'e->verdict = L"INCOMPLETE"' in HISTORY
assert "Không có PASS từ phiên này" in HISTORY
assert "ArchiveInterruptedSession(stateRoot, stateRoot)" in MAIN
assert "DiscardInterruptedStressJournal(gReportOutputDir)" in MAIN
assert "ReadInterruptedStressJournal(gReportOutputDir)" in MAIN
assert "LƯU JOURNAL & CHẠY LẠI" in UI
assert "ĐÓNG PHIÊN INCOMPLETE" in UI
assert "BỎ JOURNAL..." in UI
assert "tuyệt đối không tạo PASS" in UI

recover_archive = MAIN.find("ArchiveInterruptedSession(stateRoot, stateRoot)")
recover_start = MAIN.find("StartAudit(hwnd);", recover_archive)
assert recover_archive >= 0 and recover_start > recover_archive
recovery_slice = MAIN[recover_archive:recover_start]
assert 'overall = L"BUY"' not in recovery_slice
assert 'overall = L"PASS"' not in recovery_slice

assert "% sức khỏe" not in REPORT
assert "Full-charge / Design" in REPORT

assert "src/session_history.cpp" in CMAKE
assert "src/ui_screens_s22_s23_v2.cpp" in CMAKE
assert "RenderScreenS22_SessionHistory=RenderScreenS22_SessionHistory_Legacy" in CMAKE
assert "RenderScreenS23_InterruptedRecovery=RenderScreenS23_InterruptedRecovery_Legacy" in CMAKE
assert "src/main_round5.cpp" in CMAKE
assert "gHistorySelectedIndex" in MAIN

for literal in ["LS-20260824-001", "Dell Precision 5560", "24/08/2026 09:42:31", "96%", "12 / 12"]:
    assert literal not in UI, f"demo literal leaked into S22/S23: {literal}"

print("S22/S23 persistence and recovery sanity: OK")

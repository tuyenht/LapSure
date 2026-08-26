from pathlib import Path
from app_source_view import read_app_source

R = Path(__file__).resolve().parents[1]
MAIN = read_app_source(R)
UI_H = (R / "include/lap/ui_components.h").read_text(encoding="utf-8")
UI_CPP = (R / "src/ui_components.cpp").read_text(encoding="utf-8")
HISTORY_H = (R / "include/lap/session_history.h").read_text(encoding="utf-8")
HISTORY = (R / "src/session_history.cpp").read_text(encoding="utf-8")

assert "LifecycleStateFromDecision" in MAIN
assert "gCancel ? CanonicalUiState::Cancelled : CanonicalUiState::Pass" not in MAIN
assert "gSessionLifecycleState != CanonicalUiState::Interrupted" in MAIN
assert "gAuditReady = !gCancel" in MAIN

assert "RunAudioCameraWizard" in MAIN
assert "RunNetworkConnectivityWizard" in MAIN
assert "WM_COMMAND, 1212" in MAIN
assert "WM_COMMAND, 1213" in MAIN

assert "RECT GetNextActionButtonRect" in UI_H
assert "RECT GetNextActionButtonRect" in UI_CPP
assert "GetNextActionButtonRect(r, dpi)" in UI_CPP
assert "GetNextActionButtonRect(rail, dpi)" in MAIN

assert "IsTrustedSessionArtifactPath" in HISTORY_H
assert "weakly_canonical" in HISTORY
assert 'ext != L".html"' in HISTORY and 'ext != L".json"' in HISTORY and 'ext != L".txt"' in HISTORY
assert "is_regular_file" in HISTORY
assert "IsTrustedSessionArtifactPath(path)" in MAIN
assert "IsTrustedArtifactPathLocked(path)" in HISTORY

for forbidden in ["certutil.exe -addstore", "TrustedPublisher", "Silent automatic certificate trust"]:
    assert forbidden not in MAIN, f"silent trust-store mutation returned: {forbidden}"

print("Production hardening round 1 sanity: OK")

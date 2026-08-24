from pathlib import Path

path = Path("src/ui_screens.cpp")
text = path.read_text(encoding="utf-8")
needle = '''                              CanonicalUiState sessionLifecycleState, int auditCompletedItems, int auditTotalItems,
                              int focusIndex) {
    // 1. Page Header (C03)
'''
replacement = '''                              CanonicalUiState sessionLifecycleState, int auditCompletedItems, int auditTotalItems,
                              int focusIndex) {
    (void)sessionLifecycleState; // Legacy fallback only; canonical S01 v2 consumes lifecycle state.
    // 1. Page Header (C03)
'''
count = text.count(needle)
if count != 1:
    raise SystemExit(f"legacy S01 signature match count={count}")
path.write_text(text.replace(needle, replacement, 1), encoding="utf-8")
print("Applied legacy S01 strict-warning cleanup")

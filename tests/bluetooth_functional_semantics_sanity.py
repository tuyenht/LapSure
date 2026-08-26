from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src" / "functional_io.cpp").read_text(encoding="utf-8")

# Bluetooth radio/stack accessibility is presence evidence only. A clean PASS must require
# an operator-confirmed interaction with a known-good Bluetooth device.
assert "BluetoothProbe(HWND owner)" in SRC
assert "MB_YESNOCANCEL" in SRC
assert "FunctionalStatus::ManualRequired" in SRC
assert "known-good" in SRC or "đã biết hoạt động" in SRC
assert "BluetoothProbe(owner)" in SRC

# The previous shortcut must never return: successful BluetoothGetRadioInfo alone cannot PASS.
assert "er==ERROR_SUCCESS?FunctionalStatus::Pass" not in SRC
assert "this proves stack/radio access" not in SRC or "not RF" in SRC

print("Bluetooth functional evidence semantics: OK")

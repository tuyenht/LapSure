from pathlib import Path

p = Path(__file__).resolve().parent / "apply_round4_runtime_batch.py"
text = p.read_text(encoding="utf-8")

# The migration bodies must remain raw Python strings. Converting r'''...'''
# to ordinary triple-quoted strings collapses C++ path escapes such as \\ into \
# and makes MSVC /W4 /WX fail with C4129 (for example \s, \g, or \m).
raw_markers = [
    "smart_fn = r'''",
    "nvidia_fn = r'''",
    "mem_fn = r'''",
]
for marker in raw_markers:
    if marker not in text:
        raise SystemExit(f"expected raw migration literal marker missing: {marker}")

escaped_cpp_paths = [
    r'L"tools\\smartctl.exe"',
    r'L"tools\\nvidia-smi.exe"',
    r'L"tools\\gpu\\memtest_vulkan.exe"',
]
for literal in escaped_cpp_paths:
    if literal not in text:
        raise SystemExit(f"escaped C++ engine path missing from migration script: {literal}")

# Intentionally do not rewrite the generator. This step is now a guard that
# prevents accidental de-raw-string normalization before the candidate build.
print("Round 4 runtime migration literals verified; raw escaping preserved")

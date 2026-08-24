from pathlib import Path

p = Path(__file__).resolve().parent / "apply_round4_runtime_batch.py"
text = p.read_text(encoding="utf-8")
for old, new in [
    ("smart_fn = r'''", "smart_fn = '''"),
    ("nvidia_fn = r'''", "nvidia_fn = '''"),
    ("mem_fn = r'''", "mem_fn = '''"),
]:
    if old not in text:
        raise SystemExit(f"expected migration literal marker missing: {old}")
    text = text.replace(old, new, 1)
p.write_text(text, encoding="utf-8")
print("Round 4 runtime migration literals normalized")

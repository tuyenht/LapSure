from pathlib import Path

R = Path(__file__).resolve().parents[1]

def patch(rel, replacements):
    p = R / rel
    s = p.read_text(encoding="utf-8")
    for old, new, label in replacements:
        count = s.count(old)
        if count != 1:
            raise RuntimeError(f"{rel}: {label}: expected 1 match, got {count}")
        s = s.replace(old, new, 1)
    p.write_text(s, encoding="utf-8", newline="\n")

patch("src/ui_screens_s16_s21_v2.cpp", [
    (
        "std::min(reasonCard.bottom - 8, y + UiMetrics::Scale(44, dpi))",
        "std::min<LONG>(reasonCard.bottom - 8, static_cast<LONG>(y + UiMetrics::Scale(44, dpi)))",
        "LONG/int std::min"
    )
])

patch("src/ui_screens_s22_s23_v2.cpp", [
    (
        'L"HTML: " + (selected.htmlPath.empty() ? L"Không có" : L"Có")',
        'std::wstring(L"HTML: ") + (selected.htmlPath.empty() ? L"Không có" : L"Có")',
        "HTML label string concat"
    ),
    (
        'L"JSON: " + (selected.jsonPath.empty() ? L"Không có" : L"Có")',
        'std::wstring(L"JSON: ") + (selected.jsonPath.empty() ? L"Không có" : L"Có")',
        "JSON label string concat"
    ),
    (
        'L"Journal evidence: " + (selected.evidencePath.empty() ? L"Không có" : L"Có")',
        'std::wstring(L"Journal evidence: ") + (selected.evidencePath.empty() ? L"Không có" : L"Có")',
        "journal label string concat"
    )
])

patch("src/functional_io.cpp", [
    (
        'std::wstring radioEvidence=L"Native Bluetooth radio API accessible"+(device?L"; known/visible device enumerated":L"; no remembered/visible device enumerated");',
        'std::wstring radioEvidence=std::wstring(L"Native Bluetooth radio API accessible")+(device?L"; known/visible device enumerated":L"; no remembered/visible device enumerated");',
        "Bluetooth evidence string concat"
    )
])

print("Strict MSVC compile fixes applied")

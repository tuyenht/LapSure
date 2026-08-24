from pathlib import Path

root = Path(__file__).resolve().parents[1]
main_path = root / "src" / "main.cpp"
text = main_path.read_text(encoding="utf-8")

def replace_once(old: str, new: str, label: str) -> None:
    global text
    if old not in text:
        raise SystemExit(f"{label} anchor not found")
    text = text.replace(old, new, 1)

replace_once(
'''            if (actionFocus == 2) {
                // Focus selects the visible top-level CTA; the current screen still
                // decides whether that CTA is an audit action. No global StartAudit.
                switch (gCurrentTab) {
                case MainTab::Dashboard:
                case MainTab::AutoAudit:
                case MainTab::NewSession:
                case MainTab::Stress:
                    StartAudit(h);
                    break;
                default:
                    break;
                }
                return 0;
            }
''',
'''            if (actionFocus == 2) {
                // Focus slot 2 belongs to the S01 top-level Start/Stop button only.
                // Other screens must never inherit this invisible operation target.
                if (gCurrentTab == MainTab::Dashboard) StartAudit(h);
                return 0;
            }
''', "focus-2")

replace_once(
'''        case VK_LEFT:
            if (gFocusIndex == 1) {
''',
'''        case VK_LEFT:
            if (gFocusIndex == 1 && (gCurrentTab == MainTab::Dashboard || gCurrentTab == MainTab::NewSession)) {
''', "left mode")

replace_once(
'''        case VK_RIGHT:
            if (gFocusIndex == 1) {
''',
'''        case VK_RIGHT:
            if (gFocusIndex == 1 && (gCurrentTab == MainTab::Dashboard || gCurrentTab == MainTab::NewSession)) {
''', "right mode")

mouse_start = text.find("        // 2. Start / Stop Button Click\n")
mouse_end = text.find("        // 3.1 S01 Dashboard Specific Click Hit-Tests\n", mouse_start)
if mouse_start < 0 or mouse_end < 0:
    raise SystemExit("S01 mouse strip markers not found")

new_mouse = '''        // 2–3. S01 top mode strip and Start/Stop button. These hit regions exist
        // only when the Dashboard renderer actually draws the matching controls.
        int modeY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
        if (gCurrentTab == MainTab::Dashboard) {
            int btnW = UiMetrics::Scale(230, dpi);
            int btnH = UiMetrics::Scale(40, dpi);
            RECT btnRect{ cr.right - btnW - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi), cr.right - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi) + btnH };
            if (x >= btnRect.left && x <= btnRect.right && y >= btnRect.top && y <= btnRect.bottom) {
                StartAudit(h);
                return 0;
            }

            int mX = layout.contentRect.left + UiMetrics::Scale(134, dpi);
            int pillW = UiMetrics::Scale(80, dpi);
            int pillH = UiMetrics::Scale(28, dpi);
            int gap = UiMetrics::Scale(6, dpi);
            if (y >= modeY && y <= modeY + pillH) {
                if (x >= mX && x <= mX + pillW) { gSelectedMode = L"Quick"; InvalidateRect(h, nullptr, FALSE); }
                else if (x >= mX + pillW + gap && x <= mX + (pillW + gap) * 2) { gSelectedMode = L"Standard"; InvalidateRect(h, nullptr, FALSE); }
                else if (x >= mX + (pillW + gap) * 2 && x <= mX + (pillW + gap) * 3) { gSelectedMode = L"Deep"; InvalidateRect(h, nullptr, FALSE); }
            }
        }

'''
text = text[:mouse_start] + new_mouse + text[mouse_end:]
main_path.write_text(text, encoding="utf-8")

canonical_ci = '''name: windows-msvc-build
on: [push, pull_request]
permissions:
  contents: read

jobs:
  build:
    runs-on: windows-2022
    env:
      SOURCE_COMMIT: ${{ github.event.pull_request.head.sha || github.sha }}
    steps:
      - uses: actions/checkout@11d5960a326750d5838078e36cf38b85af677262

      - name: Full regression suite
        shell: cmd
        run: run_source_tests.cmd

      - name: Configure MSVC x64 strict
        shell: cmd
        run: cmake --preset msvc-x64-ci

      - name: Build Release
        shell: cmd
        run: cmake --build --preset build-msvc-x64-ci

      - name: Executable behavioral tests
        shell: cmd
        run: ctest --test-dir out/build/msvc-x64-ci -C Release --output-on-failure

      - name: Inventory-only provider preflight
        shell: powershell
        run: |
          $out = ".\\out\\inventory-preflight"
          $process = Start-Process -FilePath ".\\out\\build\\msvc-x64-ci\\Release\\LapSure.exe" -ArgumentList @("--inventory-only", "--output", $out) -WindowStyle Hidden -Wait -PassThru
          if ($process.ExitCode -ne 0) { throw "Inventory-only preflight exited $($process.ExitCode)" }
          $json = Get-ChildItem -LiteralPath $out -Filter "*.json" | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
          if (!$json) { throw "Inventory-only JSON report was not created" }
          $report = Get-Content -LiteralPath $json.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
          if ($report.hardware.stress.completed) { throw "Inventory-only preflight incorrectly completed stress" }
          if ($report.hardware.stress.stages.Count -ne 0) { throw "Inventory-only preflight emitted stress stages" }
          if ($report.hardware.stress.decision.overall -ne "INCOMPLETE") { throw "Inventory-only verdict must remain INCOMPLETE" }

      - name: Package portable distribution
        shell: powershell
        run: .\\package_portable.ps1 -BuildDir ".\\out\\build\\msvc-x64-ci\\Release" -OutputDir ".\\out\\portable"

      - name: Verify portable package integrity
        shell: powershell
        run: .\\validation\\verify_portable_package.ps1 -ZipPath ".\\out\\LapSure-windows-x64-portable.zip" -ExpectedCommit "$env:SOURCE_COMMIT"

      - name: Upload portable package
        uses: actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02
        with:
          name: LapSure-windows-x64-portable
          path: |
            out/LapSure-windows-x64-portable.zip
            out/LapSure-windows-x64-portable.zip.sha256
          if-no-files-found: error
'''
(root / ".github" / "workflows" / "windows-msvc-build.yml").write_text(canonical_ci, encoding="utf-8")
print("Hidden control gating patch applied; production CI restored in working tree")

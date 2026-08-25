from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


start = CMAKE.index("add_executable(LapSure WIN32")
end = CMAKE.index("\n)", start)
production_target = CMAKE[start:end]

require("src/ui_screens.cpp" not in production_target,
        "obsolete legacy screen renderer must not be compiled into the production LapSure target")

for source in [
    "src/ui_screens_s01_s04_v2.cpp",
    "src/ui_screens_s02_s09_v2.cpp",
    "src/ui_screens_s10_s15_v2.cpp",
    "src/ui_screens_s12_s14_v2.cpp",
    "src/ui_screens_s16_s21_v2.cpp",
    "src/ui_screen_s20_round5.cpp",
    "src/ui_screens_s22_s23_v2.cpp",
]:
    require(source in production_target, f"missing canonical production renderer source: {source}")

require("src/ui_components.cpp" in production_target,
        "shared UI component implementation must remain in the production target")
require("src/ui_shell_dynamic.cpp" in production_target,
        "canonical dynamic shell implementation must remain in the production target")

print("Round 5 legacy renderer production-compilation contract: PASS")

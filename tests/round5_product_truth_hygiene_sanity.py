from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEGACY_UI = (ROOT / "src/ui_screens_s16_s21_v2.cpp").read_text(encoding="utf-8")
S20_PATH = ROOT / "src/ui_screen_s20_round5.cpp"
S20 = S20_PATH.read_text(encoding="utf-8") if S20_PATH.exists() else ""
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
BUILD = (ROOT / "build.cmd").read_text(encoding="utf-8")
SYNC = (ROOT / "sync_release.cmd").read_text(encoding="utf-8") if (ROOT / "sync_release.cmd").exists() else ""
IGNORE = (ROOT / ".gitignore").read_text(encoding="utf-8")
PROFILES = ROOT / "profiles"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# S20 must not render zero counts as real evidence when Event Log acquisition failed.
require(S20_PATH.exists(), "Round 5 canonical S20 renderer must exist")
require("src/ui_screen_s20_round5.cpp" in CMAKE,
        "canonical Round 5 S20 renderer must be compiled")
require("RenderScreenS20_LogsEvents=RenderScreenS20_LogsEvents_Round5Legacy" in CMAKE,
        "older S20 implementation in the grouped renderer must be renamed out of the canonical symbol")
require("EventCountText" in S20 and "eventState" in S20,
        "S20 must centralize unavailable event-count rendering")
require(S20.count("eventState") >= 6,
        "all S20 event cards must share provider availability state")
require('if (log.state == 0) state = CanonicalUiState::Good;' not in S20,
        "normal runtime log state 0 must remain informational, not GOOD/PASS-like")
require('if (log.state == 0) state = CanonicalUiState::Good;' in LEGACY_UI,
        "test fixture must still prove the prior S20 implementation contained the false-positive mapping")

# build.cmd builds source only; release sync is a separate explicit verified operation.
require("gh release download" not in BUILD.lower(),
        "build.cmd must not silently substitute a downloaded binary for a build")
require("local build only" in BUILD.lower(),
        "build.cmd must state that it is a local source build")
require("EXPECTED_SHA256" in SYNC and "Get-FileHash" in SYNC and "--tag" in SYNC,
        "sync_release.cmd must require an explicit release tag and SHA-256 verification")

# Generated evidence and mutable state must not be easy to commit accidentally.
for pattern in ["/reports/", "session_history.tsv", "*.journal*", "/profiles/cache/"]:
    require(pattern in IGNORE, f"missing runtime/generated artifact ignore rule: {pattern}")

# Public reviewed profile fixture must be synthetic rather than a machine-specific identifier.
root_profiles = sorted(p for p in PROFILES.glob("*.json") if p.is_file())
require(root_profiles, "at least one reviewed profile fixture is expected")
for profile in root_profiles:
    require("SAMPLE" in profile.stem.upper(),
            f"public root profile filename must be explicitly synthetic: {profile.name}")
    text = profile.read_text(encoding="utf-8")
    require('"serviceTag": "SAMPLE' in text,
            f"public root profile Service Tag must be synthetic: {profile.name}")

print("Round 5 product-truth/repository-hygiene contract: PASS")

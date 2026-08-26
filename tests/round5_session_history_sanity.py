from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
H = (ROOT / "include/lap/session_history.h").read_text(encoding="utf-8")
CPP = (ROOT / "src/session_history.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# Live memory may change only after a candidate snapshot has durably persisted.
require("PersistIndexLocked(const std::vector<SessionHistoryEntry>&" in CPP,
        "history persistence must accept an explicit candidate snapshot")
require("auto candidate = gHistory" in CPP,
        "mutations must start from a candidate copy of live history")
require("gHistory.swap(candidate)" in CPP,
        "live history must swap only after candidate persistence succeeds")

# Persisted input is hostile and must be bounded/version-aware.
for token in ["kHistorySchemaVersion", "kMaxHistoryFileBytes", "kMaxHistoryLineBytes", "kMaxHistoryFieldChars", "kMaxHistoryEntries"]:
    require(token in CPP, f"missing persisted-history bound/schema constant: {token}")
require("file_size" in CPP, "history loader must bound file size before reading")
require("ValidateEntry" in CPP and "ValidateCandidate" in CPP,
        "history loader/writer must validate entry and candidate schema")
require("ValidSessionId" in CPP and "ValidVerdict" in CPP and "ValidStatus" in CPP,
        "history identity/verdict/status values must be allowlisted")
require("#LapSureSessionHistory" in CPP,
        "new history files must carry a versioned schema header")
require("unordered_set" in CPP,
        "duplicate persisted session identities must be rejected")

# Artifact deletion must be reversible until index transition is durable.
require(".delete-" in CPP or "quarantine" in CPP.lower(),
        "artifact delete must stage files in a reversible quarantine")
require("RollbackMovedArtifacts" in CPP,
        "failed artifact move/index persistence must restore original paths")

# Failure-injection must cover real index write/publish and artifact move boundaries.
require("SessionHistoryFault" in H and "SetSessionHistoryFaultForTesting" in H,
        "session history must expose compiled failure-injection hooks")
require("LapSureSessionHistoryTests" in CMAKE and "tests/session_history_tests.cpp" in CMAKE,
        "compiled session-history transaction tests must be part of CTest")

print("Round 5 transactional/bounded session-history contract: PASS")

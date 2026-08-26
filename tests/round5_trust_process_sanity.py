from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRUST = (ROOT / "src/trust.cpp").read_text(encoding="utf-8")
PROCESS = (ROOT / "src/process.cpp").read_text(encoding="utf-8")
TRUST_TEST = (ROOT / "tests/trust_security_tests.cpp").read_text(encoding="utf-8")
PROCESS_TEST = (ROOT / "tests/process_security_tests.cpp").read_text(encoding="utf-8")
SECURITY = (ROOT / "SECURITY.md").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# Manifest identity must be unique; first-match-wins is not an allowlist policy.
require("logicalMatches" in TRUST and "logicalMatches > 1" in TRUST,
        "VerifyEngine must count and reject duplicate logical manifest entries")
require("duplicate entries" in TRUST,
        "duplicate allowlist failure must be explicit")
require("break;" not in TRUST[TRUST.index("while (std::getline(file, line))"):TRUST.index("if (logicalMatches == 0)")],
        "manifest scan must not stop at the first logical-name match")

# Reparse/redirection policy applies to the engine path and trust manifest path.
require("IsReparsePoint" in TRUST and "HasReparseUnderRoot" in TRUST,
        "trust boundary must explicitly inspect reparse points")
require("HasReparseUnderRoot(root, rel" in TRUST,
        "engine relative path must be checked for reparse components")
require("HasReparseUnderRoot(root, manifestRel" in TRUST,
        "manifest path must be checked for reparse components")
require("Application root contains path redirection/reparse semantics" in TRUST,
        "redirected application roots must fail closed")
require("reparse" in TRUST_TEST.lower() and "duplicate logical allowlist entries" in TRUST_TEST,
        "compiled trust tests must cover duplicate and reparse behavior")

# Child process inheritance must be explicit and bounded to intended stdio handles.
for token in ["STARTUPINFOEXW", "PROC_THREAD_ATTRIBUTE_HANDLE_LIST", "UpdateProcThreadAttribute",
              "EXTENDED_STARTUPINFO_PRESENT", "inheritedHandles"]:
    require(token in PROCESS, f"missing restricted process inheritance primitive: {token}")
require('CreateFileW(L"NUL"' in PROCESS,
        "child stdin must be an explicit bounded handle rather than arbitrary parent stdin")
require("GetStdHandle(STD_INPUT_HANDLE)" not in PROCESS,
        "parent stdin handle must not be implicitly inherited")
require("--handle-child" in PROCESS_TEST and "inheritable sentinel pipe" in PROCESS_TEST and "PeekNamedPipe" in PROCESS_TEST,
        "compiled process tests must prove an unrelated inheritable parent handle is absent in the child")

require("requires one unambiguous manifest entry" in SECURITY and "rejects traversal/reparse paths" in SECURITY,
        "implementation must remain aligned with the documented trust policy")

print("Round 5 trust/process boundary contract: PASS")

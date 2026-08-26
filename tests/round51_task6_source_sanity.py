from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "src" / "main_round5.cpp").read_text(encoding="utf-8")
AUDIT = (ROOT / "src" / "app_audit.ipp").read_text(encoding="utf-8")
RUNTIME = (ROOT / "src" / "app_runtime_state.ipp").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# Trust boundaries must be explicit at production call sites. Preprocessor aliasing can
# hide an unsafe raw loader behind a safe-looking compiled path and is not auditable.
for macro in [
    "#define LoadFactoryProfile",
    "#define LookupFactoryProfileOnline",
    "#define LoadChassisProfile",
]:
    require(macro not in MAIN, f"Task 6 RED: remove production trust-routing macro: {macro}")

# Both normal production acquisition paths must name the decision-safe APIs directly.
for source_name, source in [("app_audit.ipp", AUDIT), ("app_runtime_state.ipp", RUNTIME)]:
    require("LoadDecisionFactoryProfile(" in source,
            f"Task 6 RED: {source_name} must explicitly call LoadDecisionFactoryProfile")
    require("LookupFactoryProfileForDecision(" in source,
            f"Task 6 RED: {source_name} must explicitly call LookupFactoryProfileForDecision")
    require("LoadDecisionChassisProfile(" in source,
            f"Task 6 RED: {source_name} must explicitly call LoadDecisionChassisProfile")

# Every production verdict rebuild must explicitly freeze one typed DecisionContext
# immediately before invoking the context-aware scorer. Legacy one-argument wrappers
# may remain as compatibility APIs for non-production callers during this tranche, but
# the app fragments may not use them.
require("BuildDecisionContext(report)" in AUDIT,
        "Task 6 RED: full audit must build a typed DecisionContext")
require("BuildAuditDecision(report, context)" in AUDIT,
        "Task 6 RED: full audit must score with its typed DecisionContext")
require("BuildAuditDecision(report);" not in AUDIT,
        "Task 6 RED: full audit must not use the no-context decision wrapper")

require(RUNTIME.count("BuildDecisionContext(") >= 2,
        "Task 6 RED: inventory-only and manual rebuild paths must each build a typed DecisionContext")
require(RUNTIME.count("BuildAuditDecision(") >= 2,
        "Task 6 RED: inventory-only and manual rebuild paths must each score explicitly")
require("BuildAuditDecision(report);" not in RUNTIME and
        "BuildAuditDecision(gReport);" not in RUNTIME,
        "Task 6 RED: runtime production paths must not use no-context decision wrappers")

print("Round 5.1A Task 6 explicit decision-boundary contract: PASS")

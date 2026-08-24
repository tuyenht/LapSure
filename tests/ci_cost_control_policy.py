from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WINDOWS_CI = ROOT / ".github" / "workflows" / "windows-msvc-build.yml"
APPLY_PATCH = ROOT / ".github" / "workflows" / "apply-s01-s04-design-patch.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


workflow = WINDOWS_CI.read_text(encoding="utf-8")

require("\n  push:\n" not in workflow, "normal Windows CI must not run on ordinary push commits")
require("workflow_dispatch:" in workflow, "manual checkpoint workflow_dispatch must remain available")
require("cancel-in-progress: true" in workflow, "CI must cancel superseded runs")
require("github.event.pull_request.draft == false" in workflow, "Draft PRs must not allocate a Windows runner")
require("contents: read" in workflow, "validation CI must be read-only")
require("git push" not in workflow and "git commit" not in workflow, "validation CI must never commit or push")
require(workflow.count("github.event_name == 'workflow_dispatch'") >= 3, "packaging and artifacts must be manual-only")

if APPLY_PATCH.exists():
    patch_workflow = APPLY_PATCH.read_text(encoding="utf-8")
    require("workflow_dispatch:" in patch_workflow, "design patch workflow must be manual-only")
    require("\n  push:\n" not in patch_workflow, "design patch workflow must not run on push")
    require("git push" not in patch_workflow and "git commit" not in patch_workflow, "design patch workflow must not self-commit")

print("CI cost-control policy: PASS")

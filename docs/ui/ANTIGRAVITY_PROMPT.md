# Recommended Google Antigravity 2.0 Start Prompt

Use this after opening `tuyenht/LapSure` as the active Project and enabling the LapSure workspace rules.

```text
You are the lead implementation agent for LapSure Professional Dashboard.

Repository: tuyenht/LapSure

Read and obey @AGENTS.md first.

For UI work use the repository's agent-executable UI contract:
@docs/ui/README.md
@docs/ui/LAPSURE_UI_MASTER_SPEC.md
@docs/ui/SCREEN_INDEX.md
@docs/ui/UI_STATE_MODEL.md
@docs/ui/DATA_BINDING_CONTRACT.md
@docs/ui/KNOWN_MOCKUP_DEVIATIONS.md
@docs/ui/UI_ACCEPTANCE_GATES.md
@docs/ui/references/MANIFEST.yaml

Use `.agents/skills/lapsure-ui/SKILL.md`.

Goal:
Implement the Professional Dashboard in the existing native C++20/Win32 application, preserving evidence correctness, Windows/WinPE graceful degradation, trusted-engine policy, bounded cancellation, report contracts and no-false-PASS semantics.

Execution:
1. Audit current UI/model/providers/tests against the contract.
2. Create/use feature branch `feature/professional-dashboard`.
3. Build shared design/state/data components first.
4. Implement screens in phases S01–S08, S09–S15, S16–S23.
5. For each screen read only its contract, referenced components, state/data contracts and matching approved visual.
6. Never copy illustrative values from mockups. Unsupported values render explicit unavailable/unknown/not-tested states.
7. Build strict MSVC x64 Release and run repository-supported regression tests after each meaningful phase.
8. Launch/capture real UI when possible and compare with the approved visual direction.
9. Use separate visual and evidence reviews before marking a screen complete.
10. Continue autonomously through every feasible P0 gate; do not stop at planning.

Completion report must include:
phases/screens, changed files, build/test results, visual artifacts, evidence audit, DPI/accessibility audit, gates not run, remaining P0/P1 gaps, branch, commit hashes and PR link if created.

Never report done while a known P0 evidence-semantic violation remains.
```

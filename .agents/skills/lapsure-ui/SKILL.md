---
name: lapsure-ui
description: Implements and audits LapSure native Win32 Professional Dashboard screens using screen/component/data/state contracts and approved visual references.
---

# LapSure UI Skill

## Required context
1. `/AGENTS.md`
2. `/docs/ui/README.md`
3. `/docs/ui/SCREEN_INDEX.md`
4. target screen contract
5. linked component contracts
6. `/docs/ui/UI_STATE_MODEL.md`
7. `/docs/ui/DATA_BINDING_CONTRACT.md`
8. `/docs/ui/KNOWN_MOCKUP_DEVIATIONS.md`
9. matching approved visual
10. current model/providers/code/tests

## Method
1. Identify Screen ID and user outcome.
2. Audit real available data; classify direct/derived/operator/unavailable.
3. Audit current renderer/navigation/threading.
4. Implement/reuse shared components.
5. Implement all applicable states, not just happy path.
6. Audit Vietnamese copy.
7. Build/test.
8. Launch/capture and compare visually.
9. Perform independent evidence-semantic audit.
10. Report files, tests, deviations and remaining gaps.

Never modify diagnostic semantics merely to make a mockup look complete.

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
Identify Screen ID/outcome → audit real data as direct/derived/operator/unavailable → audit renderer/navigation/threading → implement/reuse shared components → implement applicable non-happy states → audit Vietnamese copy → build/test → launch/capture/compare → independent evidence-semantic audit → report files/tests/deviations/gaps.

Never modify diagnostic semantics merely to make a mockup look complete.

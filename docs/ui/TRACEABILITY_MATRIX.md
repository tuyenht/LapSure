# UI Traceability Matrix

| Requirement | Screens | Components | Data/model | Verification |
|---|---|---|---|---|
| Missing evidence never PASS | S01,S04,S09–S18,S23 | C04,C06,C08,C12 | coverage/state/provider | behavior/decision tests |
| Presence != functionality | S05,S07,S12–S14 | C04,C08 | functional/port results | functional/port tests |
| Factory mismatch != health | S16,S18 | C04,C08,C09 | claim/profile/findings | comparison/decision |
| Automatic before interactive | S04,S05 | C07,C10 | orchestrator | orchestrator tests |
| Interrupted != PASS | S08,S23 | C04,C06,C11,C12 | journal/session | recovery tests |
| Adapter wattage trusted only | S07,S09 | C05,C08 | power probe | provider tests |
| No generic health score | S01,S09,S10,S18 | C05,C06 | named real metrics | UI review/source scan |
| Manual evidence labeled | S05,S06,S07,S12,S13 | C04,C08 | operator results | report/functional tests |
| Required incomplete blocks BUY | S01,S18 | C04,C06,C10 | decision/coverage | decision fixtures |
| WinPE gap != hardware fail | applicable | C04,C12 | environment/capability | WinPE/provider tests |
| HTML/JSON preserve uncertainty | S18,S19 | C08,C09 | report model | report contract tests |
| Native responsive UI | all | C01–C12 | theme/layout | build + DPI screenshot audit |

Update exact code/test names as implementation is refactored.

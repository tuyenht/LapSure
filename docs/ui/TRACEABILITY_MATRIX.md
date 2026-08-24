# UI Traceability Matrix

| Product invariant / requirement | Screens | Components | Data/model | Expected verification |
|---|---|---|---|---|
| Missing evidence never becomes PASS | S01,S04,S09–S18,S23 | C04,C06,C08,C12 | Coverage/State/provider fields | behavioral decision/state tests |
| Presence != functionality | S05,S07,S12–S14 | C04,C08 | FunctionalItemResult/PortProbeResult | functional/port tests |
| Factory mismatch != health failure | S16,S18 | C04,C08,C09 | SellerClaim/FactoryProfile/Findings | comparison/decision tests |
| Automatic snapshot before interactive | S04,S05 | C07,C10 | OrchestratorSummary | orchestrator behavior |
| Interrupted session != PASS | S08,S23 | C04,C06,C11,C12 | stress journal/session | journal recovery tests |
| Adapter wattage requires trusted evidence | S07,S09 | C05,C08 | PowerProbeResult | provider/decision tests |
| No generic health score | S01,S09,S10,S18 | C05,C06 | real named metrics only | UI review + source scan |
| Manual evidence is labeled | S05,S06,S07,S12,S13 | C04,C08 | operator-confirmed results | functional/report tests |
| Required incomplete coverage blocks BUY | S01,S18 | C04,C06,C10 | AuditDecision/CoverageDomain | decision fixtures |
| WinPE capability gap != hardware fail | all applicable | C04,C12 | environment/capabilities | WinPE/provider tests |
| HTML/JSON preserve uncertainty | S18,S19 | C08,C09 | report model | report contract tests |
| Native responsive UI | all | C01–C12 | shared theme/layout | MSVC build + screenshot/DPI audit |

When code/tests are refactored, update this matrix with exact file/test names.

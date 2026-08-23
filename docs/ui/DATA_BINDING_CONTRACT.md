# UI Data-Binding Contract

## Allowed value classes
Every displayed technical value is exactly one of: direct validated model evidence; deterministic documented derivation; operator-confirmed evidence; explicit unavailable/unknown/not-tested/unsupported/incomplete state. Mockup literals are never a data source.

## Canonical mappings
| UI concept | Model/source | Missing behavior |
|---|---|---|
| Device model/tag/environment | `AuditReport` | unknown |
| CPU | `HardwareSnapshot.cpu*` | unknown |
| RAM/DIMMs | `installedRamBytes`, `memoryModules[]` | explicit empty/unknown |
| Storage | `storage[]` | no data state |
| SMART | `smartReadable/smartPassed` | no SMART PASS if unreadable |
| Native reliability | `reliabilityReadable/reliabilityHealthy` | no native-health PASS if unreadable |
| NVMe wear/endurance | actual fields >=0 | never generic health % |
| Storage temperature | `temperatureC` >=0 | unavailable |
| GPU telemetry | actual values >=0 from supported source | unavailable |
| Battery capacities/wear | valid `BatteryInfo` values/derivation | unavailable if invalid |
| Adapter wattage | trusted `PowerProbeResult` | UNKNOWN unless proven |
| Display | EDID/current/native `DisplayInfo` | explicit partial |
| Security | `SecurityInfo` | known/unknown separately |
| PnP problems | `pnpProblems[]` | empty is not universal certification |
| Stress/event deltas | controlled stress/event evidence | history != controlled delta |
| CPU benchmark | `CpuBenchmarkResult` + source/confidence | NOT SCORED |
| Functional devices | `functional.items[]` | presence != PASS |
| Ports | stimulus `portPower.ports[]` | NOT TESTED until stimulus |
| Chassis profile | `chassisProfile` | disclose draft/generic/confidence |
| Final verdict | `AuditDecision.overall` | evidence-gated |
| Coverage | real coverage/orchestrator data | never call health |
| Confidence | actual `Confidence` | no arbitrary percentage |

## Derived metrics
Document inputs, formula, units, missing-input behavior, source and tests.

## UI/view-model boundary
Prefer a presentation mapper for state wording, units and availability. It never mutates diagnostic truth or runs slow providers.

## Mockup conflict
Preserve layout, replace unsupported value with correct unavailable state, record intentional deviation, and never modify diagnostics merely to fill a visual slot.

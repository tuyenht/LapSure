# UI Data-Binding Contract

**Purpose:** prevent the UI from inventing data and define how screen objects obtain evidence.

## 1. Allowed value classes

Every displayed technical value must be one of:
1. **Direct evidence** — present in `AuditReport` or a related validated model.
2. **Deterministic derivation** — computed from validated fields with a documented formula.
3. **Operator-confirmed evidence** — recorded through guided human stimulus/judgment.
4. **Unavailable state** — explicitly UNKNOWN / NOT TESTED / UNSUPPORTED / INCOMPLETE.

Mockup literals are never a fifth data source.

## 2. Canonical mappings

| UI concept | Preferred model/source | Missing-data behavior |
|---|---|---|
| Device model | `AuditReport.model` | `Không xác định` |
| Service Tag | `AuditReport.serviceTag` | `Không xác định` |
| Environment | `AuditReport.environment` | neutral unknown |
| CPU identity | `HardwareSnapshot.cpuName/cores/threads` | unknown |
| RAM total | `installedRamBytes` | unknown |
| DIMMs | `memoryModules[]` | empty/not available |
| Storage identity | `storage[]` | empty/not detected |
| SMART readability | `StorageDevice.smartReadable` | no SMART PASS |
| SMART result | `smartPassed` only when readable | explicit missing evidence otherwise |
| Native reliability | `reliabilityReadable/reliabilityHealthy` | no native-health PASS if unreadable |
| NVMe wear | `percentageUsed` / endurance fields when >=0 | do not invent health % |
| Storage temp | `temperatureC` when >=0 | unavailable |
| GPU | `gpus[]` | unknown |
| GPU temp/power | values >=0 and source-supported | unavailable |
| Battery design/full capacity | `BatteryInfo` readable fields | unavailable |
| Battery wear | deterministic from valid design/full capacities | do not compute from invalid/zero data |
| Adapter wattage | trusted `PowerProbeResult.adapterWatts` + confidence | UNKNOWN if not trusted |
| Display | `displays[]` EDID/current/native fields | explicit partial |
| Security | `security` | known/unknown separately |
| PnP problems | `pnpProblems[]` | empty means no recorded problems, not universal hardware certification |
| Event deltas | stress/event evidence | historical counts must not be confused with controlled deltas |
| CPU benchmark | `cpuBenchmark` with baseline source/confidence | NOT SCORED if unsupported |
| Functional devices | `functional.items[]` | presence does not imply PASS |
| Ports | `portPower.ports[]` stimulus results | NOT TESTED until physical stimulus |
| Chassis profile | `chassisProfile` | disclose draft/generic/low confidence |
| Final verdict | `AuditDecision.overall` | INCOMPLETE when gating requires |
| Coverage | `CoverageDomain` / orchestrator data | never replace with generic health |
| Confidence | real `Confidence` only | do not synthesize an arbitrary % |

## 3. Derived metric rules

Any new derived metric must document:
- input fields,
- formula,
- units,
- behavior for missing/invalid inputs,
- source attribution,
- test coverage.

## 4. UI/view-model boundary

Prefer a presentation/view-model mapping layer between `AuditReport` and rendering:
- normalize state wording,
- format units,
- expose availability,
- never mutate diagnostic truth,
- never run slow providers from paint/layout code.

## 5. Mockup conflict rule

If a mockup displays data unsupported by the model/provider:
1. preserve layout intent,
2. replace value with correct unavailable state,
3. record the intentional deviation in `KNOWN_MOCKUP_DEVIATIONS.md`,
4. do not modify diagnostic semantics merely to fill the visual slot.

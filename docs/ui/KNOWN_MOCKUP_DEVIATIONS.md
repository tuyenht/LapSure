# Known Mockup Deviations

Approved visuals are design references, not diagnostic truth. These deviations are intentional and mandatory.

| ID | Screen | Mockup concept | Required implementation |
|---|---|---|---|
| DEV-S01-001 | S01 | Generic overall health/score gauge if present | Replace with clearly named `Độ bao phủ bằng chứng` or another real metric; keep verdict separate |
| DEV-S01-002 | S01 | BUY while mandatory items remain untested | Render `CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN` |
| DEV-S04-001 | S04 | Green completion from enumeration alone | Enumeration/presence may be informative but cannot imply functional PASS |
| DEV-S09-001 | S09 | Charger wattage shown as known without trusted source | Render UNKNOWN/unavailable unless trusted provider proves it |
| DEV-S10-001 | S10 | “SSD Health 98%” generic percentage | Use exact Percentage Used/endurance/capacity metric only when hardware exposes it and label accurately |
| DEV-S11-001 | S11 | Online RAM clean = fully certified | State partial online coverage; preboot full-memory certification remains separate |
| DEV-S13-001 | S13 | Camera/mic detected = PASS | Require actual sample/capture evidence |
| DEV-S16-001 | S16 | Factory mismatch styled as hardware failure | Keep identity mismatch separate from health |
| DEV-S18-001 | S18 | “Điểm sức khỏe 88/100” | Do not implement generic health score; separate recommendation, coverage, confidence |
| DEV-S18-002 | S18 | `CÓ THỂ MUA` with incomplete required coverage | Force INCOMPLETE recommendation until mandatory coverage complete |
| DEV-S18-003 | S18 | 96% total coverage ambiguous | If BUY is allowed, explicitly show 100% mandatory coverage; optional coverage may be lower |
| DEV-GLOBAL-001 | Global | Emoji-only navigation icons | Use a consistent native/resource icon system with text fallback |
| DEV-GLOBAL-002 | Global | Illustrative sample measurements | Bind to real model/provider or explicit unavailable state |

Add a new deviation whenever an approved reference intentionally differs from implementable evidence semantics.

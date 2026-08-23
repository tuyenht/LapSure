# Known Mockup Deviations

Approved visuals are design references, not diagnostic truth.

| ID | Screen | Mockup concept | Required implementation |
|---|---|---|---|
| DEV-S01-001 | S01 | generic health/score gauge | use clearly named evidence coverage/real metric; verdict separate |
| DEV-S01-002 | S01 | BUY while mandatory items untested | show incomplete recommendation |
| DEV-S04-001 | S04 | green completion from enumeration | enumeration alone never functional PASS |
| DEV-S09-001 | S09 | charger wattage without trusted source | UNKNOWN unless proven |
| DEV-S10-001 | S10 | “SSD Health 98%” | use exact hardware wear/endurance metric only when valid and accurately labeled |
| DEV-S11-001 | S11 | online RAM clean = fully certified | show partial online coverage |
| DEV-S13-001 | S13 | camera/mic detected = PASS | require sample/capture evidence |
| DEV-S16-001 | S16 | factory mismatch as hardware failure | keep mismatch separate from health |
| DEV-S18-001 | S18 | “Điểm sức khỏe 88/100” | do not implement generic health score; separate verdict/coverage/confidence |
| DEV-S18-002 | S18 | BUY with incomplete required coverage | force INCOMPLETE |
| DEV-S18-003 | S18 | ambiguous 96% total coverage | BUY requires explicit complete mandatory coverage |
| DEV-GLOBAL-001 | Global | emoji-only icons | native/resource icon system + text fallback |
| DEV-GLOBAL-002 | Global | illustrative measurements | real binding or unavailable state |

Add a deviation whenever an approved reference intentionally differs from implementable evidence semantics.

# Used-Laptop Expert Acquisition Audit

## Product objective

LapSure must answer whether a used laptop matches the seller's claim, is healthy and stable, has all required functions, and is safe to buy at the proposed condition. A recommendation is evidence-gated: `BUY`, `BUY WITH NOTES`, `REJECT`, or `INCOMPLETE`.

LapSure is the only software workflow the operator uses. Physical observations and known-good peripheral stimulus remain necessary, but LapSure must guide and record them instead of asking the operator to reconcile other tools.

## Cross-source expert matrix

| Domain | Risks repeatedly identified by the supplied sources | Required LapSure evidence | Current state | Required action |
|---|---|---|---|---|
| Identity and provenance | Wrong advertised model/configuration, inconsistent serial/label, unclear origin, warranty mismatch, estimated age presented as fact | BIOS/CIM identity, physical-label confirmation, OEM lookup evidence, seller claim snapshot | Partial | Add claim-entry/import, label-photo/confirmation and OEM provenance adapter. CPU generation is only an age estimate. |
| Exterior and structure | Cracks, dents, impact damage, chassis gaps, loose/over-tight hinge, stripped/missing screws, suspicious disassembly | Guided physical checklist with operator confirmation and optional photos | Missing | Add mandatory Physical Condition Wizard. Tamper signs are warnings, not automatic proof of bad repair. Structural cracks/hinge separation are critical. |
| Liquid and electrical safety | Corrosion, stains, burnt smell, damaged charger cable/plug, unstable charging, abnormal charger heat | Guided visual/smell inspection, AC transition, charge-rate stability, OEM wattage/identity when exposed | Partial | Add safety wizard and timed charge-stability capture. Reject exposed conductors, burning smell, corrosion or unstable power. |
| Display | Dead/stuck pixels, lines, flicker, mura, image retention, color cast, backlight bleed, poor brightness, wrong/replaced panel | Native EDID/configuration plus full-screen solid-color visual confirmation | Partial | Existing color wizard must be localized and expanded to separate defect types and severity. EDID cannot prove visual quality. |
| Keyboard and touchpad | Dead, intermittent, stuck or duplicated keys; weak travel; dead touchpad zones; click/gesture failure | Per-key state map, duplicate/stuck-key detection, full touchpad path/click/gesture wizard | Partial | Existing keyboard wizard counts events only. Add expected-key map and touchpad coverage/gesture confirmation. |
| CPU/GPU/RAM performance | Wrong components, underperformance, crashes, memory errors, thermal throttling | Typed inventory, versioned benchmark baseline, sustained stress, error deltas, trusted thermal/power telemetry | Partial | Inventory/stress exist. Trusted CPU thermal/throttle provider and certified VRAM engine remain mandatory gaps. |
| Storage | Wrong capacity/model, weak health, SSD wear, media/uncorrectable errors, filesystem faults, abnormal performance | Native reliability, optional controller log, event history, safe read/write benchmark and non-destructive filesystem check | Partial | Native health/wear exists. Add bounded storage performance baseline and non-destructive volume scan; retain advanced log gap explicitly. |
| Battery | Design/full capacity, wear, cycles, rapid discharge, unstable charging, swelling | Windows battery evidence plus standardized timed discharge/charge observation and physical swelling check | Partial | Capacity/wear/cycles exist. Add controlled battery drain/charge session and swelling wizard. Do not infer runtime from a short uncontrolled sample. |
| Audio/camera/microphone | Distorted or one-sided speakers, poor/failed camera, weak/noisy microphone | Stereo left/right stimulus, camera frame capture, microphone recording/level and human quality confirmation | Partial | Native probes exist; localize prompts and retain human quality confirmation. |
| Wireless | Weak Wi-Fi reception, unstable association, Bluetooth inability to connect | Adapter identity/driver, signal and association stability over time, known-good Bluetooth device stimulus | Partial | Wi-Fi state exists. Add timed loss/latency sampling and guided Bluetooth pairing. Network speed alone must not condemn the laptop. |
| Physical ports | Dead/loose USB, HDMI/DP, LAN, audio, card reader, USB-C power/video/data lanes | Model-aware port map, one known-good stimulus per physical port/capability, before/after PnP evidence | Partial | Framework exists. Complete profile certification and guided per-capability stimulus. Controller presence alone never passes a port. |
| Cooling and acoustics | Overheating, throttling, abnormal fan noise, blocked vents | Trusted temperatures/clocks/power, sustained workload, fan/acoustic operator confirmation | Missing/partial | Add trusted sensor adapter and abnormal-noise/airflow wizard. Temperature thresholds must be model/workload aware. |
| Firmware/security/OS | BIOS/driver faults, device errors, unexpected restarts, WHEA/storage/display errors, unsupported firmware | BIOS/TPM/Secure Boot, Device Manager problem codes, event deltas/history with correct attribution | Partial | Existing evidence is substantial. Add PnP problem-code inventory and OEM BIOS currency as advisory, not automatic reject. |
| Commercial decision | Price ignores battery/screen/storage replacement, unclear warranty/return policy | Defect severity, estimated repair class, seller price and warranty/return inputs | Missing | Add negotiation/repair-cost policy after technical truth is stable. Price never changes a safety/critical REJECT into BUY. |

## Decision policy

### Immediate reject

- wrong CPU/GPU/RAM/storage relative to an explicitly recorded seller claim;
- structural crack or separating hinge that threatens the display/chassis;
- liquid/corrosion evidence, burning smell, exposed charger conductors or unstable power;
- storage critical health, uncorrectable/media errors, or a failed SMART/native health verdict;
- repeatable WHEA, memory, GPU/VRAM, storage, display-driver or bugcheck failure attributable to the test window;
- failed mandatory display, keyboard, input, charging or required-port function when the buyer did not explicitly accept that defect.

### Buy with notes / negotiate

- cosmetic wear without structural impact;
- evidence of prior opening without proof of bad repair;
- moderate battery wear with stable operation;
- minor display bleed/cosmetic issue explicitly accepted by the buyer;
- missing optional capability or repairable non-safety defect with disclosed replacement class.

### Incomplete

- any mandatory domain lacks reliable evidence;
- a physical/manual check was skipped;
- the test environment, permissions or provider prevented reliable collection;
- seller claim, device identity or physical-label cross-check is absent when authenticity is being asserted.

## Evidence rules

1. Presence is not functional proof.
2. A short benchmark is not stability certification.
3. A short uncontrolled battery sample is not a runtime estimate.
4. CPU generation is not an exact production date.
5. An intact or broken seal is not proof of originality or bad repair.
6. Network throughput includes router/ISP effects and must not be attributed solely to the laptop.
7. HDD/SSD filesystem checks and controller-health checks answer different questions.
8. Human visual/audio/physical evidence must identify the operator action and confidence.
9. Unknown and unsupported values remain explicit and can never become PASS.
10. Every recommendation must cite the exact findings that caused it.

## Supplied-source audit status

The accessible articles consistently covered exterior/hinges, display colors and defects, keyboard/touchpad, claimed configuration, storage health, battery wear and real use, charger, audio/camera/microphone, Wi-Fi/Bluetooth, physical ports, stress/temperature, serial/warranty/origin and return policy. Some thresholds conflict (for example acceptable battery health/wear), so LapSure must use documented policy bands and buyer needs instead of copying a shop's single cutoff.

The three supplied YouTube pages did not expose their page content or transcript through the available retrieval channel during this audit. Their content is therefore not treated as verified requirements. They remain pending transcript/manual review.

## Sources supplied for the audit

- https://www.thegioididong.com/hoi-dap/cach-test-laptop-cu-chi-tiet-1590512
- https://www.youtube.com/watch?v=3ubXo3VrlSo
- https://www.youtube.com/watch?v=n3oQlHmgCdM
- https://www.youtube.com/watch?v=BEvM77gFKpw
- https://phongvu.vn/cong-nghe/cach-test-laptop-cu-day-du-tu-a-den-z-de-khong-lo-ho-khi-mua-hang/
- https://thinkpro.vn/noi-dung/cach-test-laptop
- https://www.thegioididong.com/tin-tuc/cach-kiem-tra-laptop-cu-1372295
- https://laptopphuctho.vn/pages/cach-kiem-tra-laptop-cu-truoc-khi-mua-de-tranh-hang-kem-chat-luong
- https://khoavang.vn/blog/10-cach-kiem-tra-test-laptop-cu-va-kinh-nghiem-truoc-khi-di-mua-p2780.html
- https://lapvip.vn/tin-tuc/thu-thuat/12-buoc-kiem-tra-chat-luong-laptop-cu-truoc-khi-mua
- https://www.phucanh.vn/huong-dan-chi-tiet-cach-mua-va-test-laptop-cu-gia-re-an-toan-va-chat-luong.html
- https://gearvn.com/blogs/thu-thuat-giai-dap/kiem-tra-nam-san-xuat-laptop
- https://dienthoaivui.com.vn/cach-test-laptop-cu
- https://onlylap.vn/cach-kiem-tra-laptop-khi-mua-laptop-cu
- https://2tmobile.com/cach-kiem-tra-laptop-cu

# LapSure

<div align="center">

![Platform](https://img.shields.io/badge/platform-Windows%20x64%20%7C%20WinPE-blue?style=for-the-badge&logo=windows)
![Build](https://img.shields.io/badge/MSVC-C%2B%2B20%20%2FW4%20%2FWX-brightgreen?style=for-the-badge&logo=visualstudio)
![CI](https://img.shields.io/github/actions/workflow/status/tuyenht/LapSure/windows-msvc-build.yml?branch=main&style=for-the-badge&label=Windows%20CI)
![Release](https://img.shields.io/github/v/release/tuyenht/LapSure?style=for-the-badge&color=orange)
![License](https://img.shields.io/badge/license-Proprietary%20%2F%20All%20Rights%20Reserved-lightgrey?style=for-the-badge)

**Laptop Verification & Diagnostics for Windows / WinPE**

> **Kiểm đúng máy. Biết đúng tình trạng. Mua đúng giá.**

</div>

---

## Trạng thái dự án

LapSure hiện ở giai đoạn **Beta 0.1.1 / production-hardening**. Mục tiêu của dự án là kiểm định laptop đã qua sử dụng bằng bằng chứng kỹ thuật có thể truy vết, thay vì suy đoán hoặc tạo trạng thái PASS khi dữ liệu chưa đủ.

Round 5 đã đóng các contract chính về inspection identity, transactional report publication, transactional/bounded session history, process/external-engine trust, cloud privacy/provenance, product truth và production renderer. Một independent security review sau package freeze còn phát hiện thêm **P1 — Acceptance Verdict Integrity**: factory/chassis metadata trong portable directory có thể bị sửa nhưng trước đó vẫn có đường ảnh hưởng trực tiếp đến verdict. P1 này đã được sửa theo fail-closed policy và được kiểm lại trên exact production-source SHA:

`b5b04711941dc469e8a0a06a61710f60df7c8328`

Fresh Windows checkpoint `32851743158` (attempt 2, job `97814496751`) xác nhận:

- full source regression: **PASS**;
- strict MSVC x64 Release `/W4 /WX`: **PASS**;
- **6/6 CTest PASS**, gồm `LapSureProfileProvenanceTests`;
- inventory-only transactional bundle/history preflight: **PASS**;
- inventory-only vẫn giữ verdict **INCOMPLETE** và không tạo stress evidence giả.

Đây là source/build/behavioral/integration evidence, **không phải physical-hardware certification**.

### Package formal-pilot hiện tại

Package cũ tại commit `7116ffbacce92011f60be5f9858b8afeb9fe4227`, artifact `9560018587`, vẫn là historical build evidence nhưng đã **superseded** sau khi P1 được phát hiện. **Không dùng artifact đó cho formal Precision pilot.**

Trước formal pilot cần một manual `workflow_dispatch` mới trên final PR head để lặp lại source/build/6 CTest/inventory checks, tạo ZIP mới, xác minh provenance và ghi nhận SHA-256 của ZIP + `LapSure.exe`.

Các nguyên tắc phát hành hiện tại:

- Thiếu, lỗi, timeout, stale, mâu thuẫn, unsupported hoặc untrusted evidence **không được biến thành PASS/BUY**.
- Required coverage chưa đủ thì kết luận phải giữ **INCOMPLETE / CHƯA ĐỦ DỮ LIỆU**.
- Functional presence không đồng nghĩa với functional PASS; Wi-Fi, Bluetooth, cổng vật lý và các phép kiểm tra tương tác cần evidence/stimulus phù hợp.
- Hardware decision và trạng thái lưu/phát hành report là hai sự thật riêng; lỗi file I/O không được dùng để giả lập hoặc sửa nghĩa chẩn đoán phần cứng.
- External diagnostic engines chỉ được chạy qua trust boundary fail-closed; build Beta hiện không bật engine ngoài bằng hash allowlist rỗng.
- Cloud/cache/static portable factory metadata chỉ là advisory nếu chưa có exact identity + authenticated/trusted provenance.
- Chassis `.profile` trong portable directory có thể hướng dẫn kiểm cổng nhưng **không được tự khai `physical-verified` để mở khóa BUY**.
- Tagged Beta là bản thử nghiệm có thể kiểm chứng, **không phải tuyên bố Stable/Production certification**.

## Bản phát hành

| Phiên bản | Trạng thái | Ghi chú |
| --- | --- | --- |
| **v0.1.1-beta** | Beta / validation | Evidence-correctness, behavioral/security regression gates, runtime/port coverage semantics và portable package verification. |
| **v0.1.0-beta** | Archive | Core diagnostic engine ban đầu. |

Xem các gói đã gắn tag tại [GitHub Releases](https://github.com/tuyenht/LapSure/releases). Nhánh/PR hardening mới hơn tag không được xem là release cho tới khi các gate phát hành tương ứng hoàn tất.

---

## LapSure kiểm tra những gì?

### 1. Nhận diện và đối chiếu cấu hình

- CPU, RAM/DIMM, storage, GPU, BIOS/SMBIOS và Service Tag khi provider hỗ trợ.
- Native EDID để xác minh màn hình, độ phân giải gốc và evidence liên quan thay vì dùng current desktop resolution làm bằng chứng panel.
- Seller Claim để đối chiếu cấu hình người bán quảng cáo với phần cứng thực tế.
- Model/chassis profile được dùng như kỳ vọng tham chiếu khi provenance cho phép; profile thiếu, mutable hoặc chưa được chứng nhận không tự tạo Factory PASS/clean BUY.

### 2. Storage, pin và sức khỏe phần cứng

- Windows Storage Management reliability counters cho health/operational state, nhiệt độ, wear và error evidence khi khả dụng.
- SMART/NVMe enrichment qua engine đã allowlist khi được đóng gói và xác minh.
- Battery design/full-charge capacity, wear, cycle evidence và fallback có ghi rõ nguồn khi provider chính không đọc được.
- Event/forensics evidence cho WHEA, storage/display errors, unexpected restart và các tín hiệu ổn định khác mà Windows cung cấp.

### 3. Kiểm tra chức năng có tương tác

- Bàn phím, touchpad/click/scroll, màn hình và phân loại lỗi hiển thị.
- Camera, microphone và stereo audio.
- Wi-Fi và Bluetooth với semantics bảo thủ: phát hiện radio/device chỉ là presence evidence, không tự động chứng minh chức năng hoàn chỉnh.
- Ngoại hình/an toàn vật lý: chassis, bản lề, dấu va đập/can thiệp, chất lỏng/ăn mòn, pin phồng và an toàn nguồn/sạc.

### 4. Ports & Power

LapSure lưu evidence theo từng phép thử cổng/nguồn. Controller hoặc host-router presence không được dùng thay cho bằng chứng thiết bị/stimulus thực tế. Khi không thể chứng minh tốc độ, DP Alt Mode, Thunderbolt/USB4 path hoặc công suất adapter một cách đáng tin cậy, kết quả phải giữ unknown/not-tested thay vì tự suy diễn.

Chassis metadata dạng portable có thể được dùng làm **advisory guidance** cho operator, nhưng không phải authenticated physical certification. Trạng thái `physical-verified` chỉ có ý nghĩa acceptance khi đến từ trust boundary được bảo vệ, không phải một trường text có thể sửa trong file.

### 5. Stress & Stability

- Bounded CPU sustained-load stage.
- Online partial-coverage RAM integrity test; không tuyên bố thay thế pre-boot memory diagnostics.
- GPU/VRAM stage khi trusted engine phù hợp có mặt.
- Event deltas và telemetry được gắn vào từng stress stage.
- Cancellation, timeout, crash/reboot/interrupted-session recovery không được chuyển thành clean PASS.

---

## Evidence-first decision model

LapSure tách bốn khái niệm dễ bị trộn lẫn:

1. **Evidence** — dữ liệu thật thu được từ provider, thao tác người dùng hoặc artifact đã lưu.
2. **Provenance / Trust** — evidence/profile đến từ đâu và có đủ thẩm quyền để ảnh hưởng verdict hay chỉ là advisory.
3. **Coverage** — required domain nào đã đủ bằng chứng, partial hoặc chưa test.
4. **Decision** — kết luận mua/không mua chỉ được tạo sau khi trust, coverage và severity cho phép.

Các kết luận sản phẩm được thiết kế xoay quanh `BUY`, `BUY WITH NOTES`, `INCOMPLETE` và `REJECT`; UI/report phải bảo toàn uncertainty thay vì che nó bằng phần trăm hoặc trạng thái mẫu.

HTML và JSON report dùng cùng một inspection identity. Report publication hiện dùng transactional `bundle-*` generation: HTML/JSON được stage cùng nhau, publish theo generation boundary và session history được commit trong publication flow. Compiled failure-injection/regression evidence đã PASS; publication readiness vẫn tách riêng khỏi hardware decision truth.

---

## Professional Dashboard

Ứng dụng sử dụng native Win32/C++20 và hệ thống màn hình evidence-bound S01–S23. Các thành phần giao diện chính gồm app shell, grouped navigation, page header, status badge, metric cards, progress/coverage, guided stepper, evidence rows, data table, next-action panel, confirmation dialog và explicit empty/error states.

Mục tiêu thiết kế là:

- dữ liệu runtime động, không dùng sample hardware/fixed percentage/pre-filled PASS;
- status không chỉ dựa vào màu;
- DPI-aware Win32 rendering;
- CTA/hit-test chỉ hoạt động khi control tương ứng thực sự được render;
- lịch sử phiên, export và interrupted recovery lấy từ artifact/journal đã persisted thay vì dữ liệu demo.

---

## Build từ source

Yêu cầu Windows với Visual Studio 2022/MSVC và CMake đủ mới theo `CMakePresets.json`.

Build Release để phát triển cục bộ:

```powershell
cmake --preset msvc-x64-release
cmake --build --preset build-msvc-x64-release
```

Chạy source regression:

```cmd
run_source_tests.cmd
```

Checkpoint strict tương đương CI:

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci
ctest --test-dir out/build/msvc-x64-ci -C Release --output-on-failure
```

Không bỏ `/W4 /WX`, không tắt test hoặc hạ evidence gate chỉ để làm build xanh.

---

## GitHub Actions & kiểm soát chi phí

Workflow Windows được cố ý thiết kế để không build liên tục trong lúc phát triển:

- PR ở trạng thái **Draft** không cấp Windows build runner thông thường.
- `docs/**`, `design/**` và Markdown-only changes được bỏ qua.
- Các run superseded được cancel bằng concurrency policy.
- Portable packaging, integrity verification và artifact upload chỉ chạy khi **manual `workflow_dispatch`**.
- Trong quá trình phát triển, ưu tiên source tests/build cục bộ; chỉ dùng remote CI cho checkpoint có giá trị xác nhận rõ ràng.

Workflow validation không được tự sửa code hoặc tự commit/push. Không bật packaging trên ordinary PR event chỉ để tránh manual release gate.

---

## Round 5 — Contract Closure & Pilot Readiness

Source of truth:

- `docs/superpowers/specs/2026-08-25-contract-closure-pilot-readiness-design.md`
- `docs/superpowers/plans/2026-08-25-contract-closure-pilot-readiness.md`
- `docs/PRODUCTION_HARDENING_STATUS.md`

Các production contract chính đã được đóng và fresh strict checkpoint trên `b5b04711941dc469e8a0a06a61710f60df7c8328` đã PASS. Independent review cũng đã đóng P1 mutable portable-profile verdict authority bằng decision-safe factory/cloud/chassis boundaries và compiled provenance regression test.

**Gate còn lại trước formal physical pilot:** tạo lại package/provenance bằng manual `workflow_dispatch` trên final PR head và ghi nhận artifact/hash mới. Sau đó mới dùng đúng package đó để chạy Precision pilot.

Không mở rộng feature scope trong vòng này nếu không có defect thật được chứng minh.

---

## Real-machine validation gate

Repo có bộ tài liệu trong `validation/`:

- `PILOT_RUNBOOK.md`
- `VALIDATION_CHECKLIST.md`
- `REAL_MACHINE_MATRIX.tsv`
- `SESSION_RECORD_TEMPLATE.md`
- `DISCREPANCY_LOG_TEMPLATE.tsv`
- `REFERENCE_COMPARATORS.md`
- `verify_portable_package.ps1`

Formal Precision runtime-acceptance pilot phải dùng đúng replacement candidate/package đã được ghi nhận SHA/provenance sau P1 remediation.

Hiện các entry Precision 5560 / 5570 / 7670 trong real-machine matrix vẫn là **NOT RUN** cho full audit/runtime gate. Sau một formal pilot đạt yêu cầu vẫn chưa đồng nghĩa profile được chứng nhận; profile/model certification cần physical evidence độc lập và mọi discrepancy quan trọng được disposition.

---

## Security & trust

Xem [SECURITY.md](SECURITY.md) cho security policy và trust model. Một số nguyên tắc cốt lõi:

- explicit executable path thay vì phụ thuộc PATH resolution cho security-sensitive processes;
- external engine verification fail-closed;
- unauthenticated cloud/cache/static portable factory data không được trở thành Factory Exact;
- mutable chassis metadata không được tự nâng thành physical-verification authority;
- session/history/report artifacts được coi là untrusted input khi mở lại và phải được validation/bounding phù hợp;
- persistence/publication failure không được biến thành một clean hardware verdict giả;
- broad administrator elevation hiện là Beta limitation và cần tiếp tục thu hẹp trong kiến trúc production dài hạn.

---

## Định hướng tiếp theo

Ưu tiên hiện tại là **replacement package/provenance → formal Precision pilot → discrepancy disposition → Ready/Merge review**. Production source P1-remediated đã có fresh source/strict-build/6-CTest/inventory evidence, nhưng chưa được coi là production-ready cho tới khi package mới và physical runtime/visual/accessibility evidence hoàn tất.

LapSure là công cụ hỗ trợ kiểm định và quyết định mua máy; kết luận cuối cùng luôn phải được đọc cùng evidence, provenance, coverage, publication state và giới hạn của từng phiên kiểm tra.

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

Bản hardening hiện tại trên PR #2 đã vượt qua checkpoint tự động gồm source regression, MSVC x64 Release strict `/W4 /WX`, CTest behavioral/security tests, inventory-only provider preflight và kiểm tra package/provenance. Tuy vậy, **đây chưa phải chứng nhận production-ready**: ma trận máy thật vẫn cần full-workflow pilot trên Precision đại diện, và profile chỉ được coi là đủ bằng chứng sau khi có tối thiểu hai máy vật lý được xác minh độc lập theo runbook.

Các nguyên tắc phát hành hiện tại:

- Thiếu, lỗi, timeout, stale, mâu thuẫn, unsupported hoặc untrusted evidence **không được biến thành PASS/BUY**.
- Required coverage chưa đủ thì kết luận phải giữ **INCOMPLETE / CHƯA ĐỦ DỮ LIỆU**.
- Functional presence không đồng nghĩa với functional PASS; Wi-Fi, Bluetooth, cổng vật lý và các phép kiểm tra tương tác cần evidence/stimulus phù hợp.
- External diagnostic engines chỉ được chạy qua trust boundary có canonical path và SHA-256 allowlist; không xác minh được thì không chạy.
- Tagged Beta là bản thử nghiệm có thể kiểm chứng, **không phải tuyên bố Stable/Production certification**.

## Bản phát hành

| Phiên bản | Trạng thái | Ghi chú |
| --- | --- | --- |
| **v0.1.1-beta** | Beta / validation | Evidence-correctness, behavioral regression gates, runtime/port coverage semantics và portable package verification. |
| **v0.1.0-beta** | Archive | Core diagnostic engine ban đầu. |

Xem các gói đã gắn tag tại [GitHub Releases](https://github.com/tuyenht/LapSure/releases). Nhánh/PR hardening mới hơn tag không được xem là release cho tới khi các gate phát hành tương ứng hoàn tất.

---

## LapSure kiểm tra những gì?

### 1. Nhận diện và đối chiếu cấu hình

- CPU, RAM/DIMM, storage, GPU, BIOS/SMBIOS và Service Tag khi provider hỗ trợ.
- Native EDID để xác minh màn hình, độ phân giải gốc và evidence liên quan thay vì dùng current desktop resolution làm bằng chứng panel.
- Seller Claim để đối chiếu cấu hình người bán quảng cáo với phần cứng thực tế.
- Model/chassis profile được dùng như kỳ vọng tham chiếu; profile thiếu hoặc chưa được chứng nhận không tự tạo PASS.

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

### 5. Stress & Stability

- Bounded CPU sustained-load stage.
- Online partial-coverage RAM integrity test; không tuyên bố thay thế pre-boot memory diagnostics.
- GPU/VRAM stage khi trusted engine phù hợp có mặt.
- Event deltas và telemetry được gắn vào từng stress stage.
- Cancellation, timeout, crash/reboot/interrupted-session recovery không được chuyển thành clean PASS.

---

## Evidence-first decision model

LapSure tách ba khái niệm thường bị trộn lẫn:

1. **Evidence** — dữ liệu thật thu được từ provider, thao tác người dùng hoặc artifact đã lưu.
2. **Coverage** — required domain nào đã đủ bằng chứng, partial hoặc chưa test.
3. **Decision** — kết luận mua/không mua chỉ được tạo sau khi coverage và severity cho phép.

Các kết luận sản phẩm được thiết kế xoay quanh `BUY`, `BUY WITH NOTES`, `INCOMPLETE` và `REJECT`; UI/report phải bảo toàn uncertainty thay vì che nó bằng phần trăm hoặc trạng thái mẫu.

HTML và JSON report dùng cùng inspection identity, giữ provenance/evidence semantics và được lưu theo transaction để tránh công bố một cặp report không đồng bộ.

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

Workflow không được tự sửa code hoặc tự commit/push. One-shot migration/validation helpers chỉ được dùng như công cụ chuyển tiếp có chủ đích và phải được xóa khỏi candidate sau khi hoàn thành.

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

Trước khi nâng trạng thái từ Beta hardening lên production-ready, cần ít nhất một full runtime-acceptance workflow trên máy Precision mục tiêu với report đồng thuận, không có critical false PASS và mọi discrepancy quan trọng đã được reviewer disposition. Profile certification cần tối thiểu hai máy vật lý được xác minh độc lập.

Hiện các entry Precision 5560 / 5570 / 7670 trong real-machine matrix vẫn là **NOT RUN** cho full audit/runtime gate; vì vậy README, release notes và UI không được tuyên bố các profile này đã được chứng nhận.

---

## Security & trust

Xem [SECURITY.md](SECURITY.md) cho security policy và trust model. Một số nguyên tắc cốt lõi:

- explicit executable path thay vì phụ thuộc PATH resolution cho security-sensitive processes;
- canonical system PowerShell path cho privileged diagnostics;
- external engine verification fail-closed;
- session/history/report artifacts được coi là untrusted input khi mở lại và phải được validation/bounding phù hợp;
- persistence failure không được thay đổi hardware decision thành một kết luận sạch giả tạo.

---

## Định hướng tiếp theo

Ưu tiên tiếp theo không phải mở rộng marketing claim mà là hoàn tất real-machine evidence: chạy pilot Precision theo runbook, disposition discrepancy, xác minh report/port/profile behavior và chỉ sau đó mới cân nhắc Ready for Review/merge/release candidate.

LapSure là công cụ hỗ trợ kiểm định và quyết định mua máy; kết luận cuối cùng vẫn cần được đọc cùng evidence và giới hạn coverage của từng phiên kiểm tra.

# LapSure

<div align="center">

![Platform](https://img.shields.io/badge/platform-Windows%20x64%20%7C%20WinPE-blue?style=for-the-badge&logo=windows)
![Build](https://img.shields.io/badge/MSVC-C%2B%2B20%20Strict%20%2FW4%20%2FWX-brightgreen?style=for-the-badge&logo=visualstudio)
![CI](https://img.shields.io/github/actions/workflow/status/tuyenht/LapSure/windows-msvc-build.yml?branch=main&style=for-the-badge&label=GitHub%20CI)
![Tests](https://img.shields.io/badge/tests-193%2F193%20PASSED-success?style=for-the-badge&logo=checkmarx)
![Release](https://img.shields.io/github/v/release/tuyenht/LapSure?style=for-the-badge&color=orange)
![License](https://img.shields.io/badge/license-Proprietary%20%2F%20All%20Rights%20Reserved-lightgrey?style=for-the-badge)

**Phần mềm Kiểm định, Chẩn đoán & Pháp y Laptop Chuyên nghiệp dành cho Windows và WinPE**

> *"Kiểm đúng máy. Biết đúng tình trạng. Mua đúng giá."*

[Tải về Bản phát hành](#-tải-về-bản-phát-hành--downloads) • [Kiến trúc Dashboard](#-giao-diện-professional-dashboard-23-màn-hình) • [Tính năng Nổi bật](#-tính-năng-nổi-bật) • [Hệ sinh thái Hỗ trợ](#-hỗ-trợ-chuyên-sâu-các-dòng-máy) • [Hướng dẫn Sử dụng](#-hướng-dẫn-sử-dụng) • [Quy trình Build](#-hướng-dẫn-build-từ-mã-nguồn)

</div>

---

## 📥 Tải về Bản phát hành / Downloads

Tất cả các bản dựng đều được biên dịch tự động qua GitHub Actions MSVC x64 Native Pipeline với chữ ký Checksum SHA-256 xác thực:

| Phiên bản | Trạng thái | Gói tải về Portable (.zip) | Ghi chú phát hành |
| :--- | :--- | :--- | :--- |
| **v0.1.1-Beta (Mới nhất)** | 🟢 Stable Release | [👉 **LapSure-windows-x64-portable.zip**](https://github.com/tuyenht/LapSure/releases/download/v0.1.1-beta/LapSure-windows-x64-portable.zip) | Ra mắt **Professional Dashboard** (23 màn hình Win32 native, C01–C12 Design System, Per-Monitor V2 DPI Scaling, 100% 0-Defect Audit) |
| **v0.1.0-Beta** | ⚪ Archive | [👉 **LapSure-windows-x64-portable.zip**](https://github.com/tuyenht/LapSure/releases/download/v0.1.0-beta/LapSure-windows-x64-portable.zip) | Bản thử nghiệm Core Engine ban đầu (Dell fleet, Base36, PnP Code 43 audit, Battery telemetry) |

🔗 **Trang tổng hợp tất cả các phiên bản (Releases Page)**:  
👉 **[https://github.com/tuyenht/LapSure/releases](https://github.com/tuyenht/LapSure/releases)**

---

## 🖥️ Giao diện Professional Dashboard (23 Màn hình)

LapSure sở hữu giao diện đồ họa **Native Win32 C++20 Professional Dashboard** tối ưu hoàn hảo cho kỹ thuật viên và người mua máy:

* **Không phụ thuộc Framework cồng kềnh**: 100% Pure Win32 GDI double-buffered, khởi chạy tức thì trong 0.2 giây cả trên Windows 11 lẫn môi trường cứu hộ USB WinPE.
* **Chuẩn Design System 4px Scale & Per-Monitor V2 DPI Awareness**: Tự động hiển thị sắc nét từ màn hình laptop HD 1366x768 đến màn hình 4K 200% scaling mà không bị mờ hay vỡ layout.
* **12 Thành phần Tái sử dụng (C01–C12 Reusable Component Library)**:
  * **C01 App Shell**: Layout 3 vùng tiêu chuẩn (Sidebar 240px, Content canvas, Status footer).
  * **C02 Sidebar & Grouped Navigation**: 4 nhóm điều hướng chuẩn mực (*Quy trình, Chi tiết thiết bị, Đánh giá & Hồ sơ, Hệ thống*).
  * **C03 Page Header**: Tiêu đề trang kèm thẻ trạng thái và mã định danh phiên kiểm tra.
  * **C04 Status Badge**: Thẻ trạng thái 3 cấp độ kết hợp màu sắc, biểu tượng và chữ tiếng Việt chuẩn hóa.
  * **C05 Metric Card & KPI**: Thẻ số liệu lớn hiển thị sức khỏe pin, độ mòn SSD, nhiệt độ và xung nhịp.
  * **C06 Progress & Coverage**: Thanh tiến trình kép phân biệt rõ ràng giữa *Tiến độ thời gian* và *Mức độ bao phủ bằng chứng*.
  * **C07 Guided Stepper**: Hướng dẫn từng bước kiểm tra tuần tự có kiểm soát rủi ro.
  * **C08 Evidence Row**: Hàng đối chiếu bằng chứng kỹ thuật với dấu thời gian và nguồn dữ liệu.
  * **C09 Data Table**: Bảng dữ liệu đa cột hỗ trợ phông chữ Monospace cho địa chỉ thanh ghi và mã Serial.
  * **C10 Next Action Panel**: Khung hướng dẫn hành động tiếp theo thông minh.
  * **C11 Dialog Confirmation**: Hộp thoại xác nhận thao tác quan trọng có cảnh báo rủi ro.
  * **C12 Empty / Error State**: Giao diện hiển thị rõ ràng khi thiếu dữ liệu hoặc không có quyền truy cập.

---

## 🌟 Tính năng Nổi bật

LapSure được thiết kế theo triết lý **"Zero-Blind, Zero-Defect, Không bao giờ cấp PASS giả"**, cung cấp bằng chứng kỹ thuật trực quan và minh bạch:

### 1. Nhận diện & Đối chiếu Cấu hình Chi tiết
- **Bộ xử lý (CPU)**: Tên chip đầy đủ, xung nhịp, số nhân/luồng.
- **Bộ nhớ (RAM)**: Chi tiết từng thanh DIMM cắm trên bo mạch (Hãng sản xuất, Part Number, Serial Number, Bus thực tế/Bus thiết kế).
- **Ổ lưu trữ (NVMe/SSD)**: Đọc dữ liệu SMART, sức khỏe phần trăm tuổi thọ còn lại (Endurance), nhiệt độ, chu kỳ bật/tắt (Power Cycles), số giờ hoạt động (Power-On Hours), số lần sập nguồn đột ngột (Unsafe Shutdowns).
- **Đồ họa (GPU)**: Nhận diện cả iGPU tích hợp và dGPU rời (NVIDIA/AMD), dung lượng VRAM, phiên bản Driver, VBIOS, nhiệt độ, công suất (W).
- **Màn hình**: Đọc trực tiếp bộ mã **EDID Descriptor** từ phần cứng màn hình (độ phân giải gốc native, tần số quét Hz, tên mã tấm nền panel) — chống chiêu trò ép độ phân giải ảo qua phần mềm.
- **Bảo mật & Firmware**: Kiểm tra trạng thái TPM 2.0, Secure Boot, phiên bản BIOS/SMBIOS.

### 2. Pháp y & Bảo vệ Người Mua Used Laptop
- **Quét mã lỗi Driver ẩn (PnP Yellow Bang Detector)**: Quét toàn bộ cây thiết bị qua Windows SetupAPI & Configuration Manager:
  - Phát hiện `Code 43` (`CM_PROB_FAILED_POST_START` — lỗi chết chip GPU dGPU đóng lại chân/hấp chip) $	o$ **Lập tức ra phán quyết `REJECT (KHÔNG NÊN MUA)`**.
  - Phát hiện `Code 10` (lỗi khởi động thiết bị), `Code 28` (thiếu driver), `Code 22` (bị cố tình tắt).
- **Kiểm tra An toàn Hệ thống tệp (Filesystem Dirty-Bit Integrity)**:
  - Gửi lệnh Win32 `FSCTL_IS_VOLUME_DIRTY` quét toàn bộ phân vùng ổ đĩa (C:, D:...).
  - Phát hiện phân vùng bị cờ DIRTY do BSOD crash, sập nguồn đột ngột hoặc hỏng sector metadata.
- **Đo Tốc độ Xả Pin thời gian thực (Controlled Battery Discharge Telemetry)**:
  - Đọc công suất xả tức thời (mW / W), dung lượng thực tế còn lại (mWh), điện áp (V) từ ngăn xếp `root/wmi:BatteryStatus`.
  - Tự động phát hiện pin tụt áp đột ngột hoặc cell pin bị chập/ăn nguồn (> 65W).
- **Đối chiếu Cam kết Người bán (Seller Claim Form)**:
  - So sánh cấu hình người bán quảng cáo (Model, CPU, RAM, Ổ cứng, Giá tiền, Ngày bảo hành) với phần cứng thực tế. Bất kỳ sai lệch nào đều kích hoạt bằng chứng cảnh báo nghiêm trọng.
- **Kiểm tra Ngoại hình 6 Điểm Vật lý**:
  - Bản lề & khung vỏ (Hinges & Chassis)
  - Móp méo, cong vênh do rơi rớt
  - Vết can thiệp ốc vít, rách tem bảo hành
  - Dấu vết vào nước, ẩm mốc (Liquid damage)
  - Phồng pin, kênh trackpad
  - Sạc zin theo máy & đúng công suất

### 3. Bộ Wizard Tương tác Trực quan 100% Tiếng Việt
- ⌨️ **Bàn phím Ma trận 68 phím ANSI**: Theo dõi thời gian thực từng nút bấm với giao diện GDI double-buffered chống giật, đếm tỷ lệ % hoàn tất và chỉ điểm phím liệt.
- 🖱️ **Lưới Cảm ứng Trackpad 80 ô**: Kiểm tra toàn bộ diện tích cảm ứng, click trái, click phải và cuộn 2 ngón tay (Two-finger scroll).
- 🖥️ **Phân loại Khuyết tật Màn hình 5 Cấp độ**: Hiển thị tuần tự qua 6 nền màu chuẩn (Đỏ, Xanh lá, Xanh dương, Trắng, Đen, Xám) để phát hiện điểm chết (Dead pixel), hở sáng (Backlight bleed), ố vàng, sọc chỉ, đốm phản quang.
- 🔊 **Loa Stereo Trái/Phải (L/R)**: Phát âm tần số chuẩn 660Hz tách biệt 2 kênh để xác nhận loa không bị rè, không bị chập kênh.
- 📷 **Thiết bị Ngoại vi Tự động**: Kiểm tra luồng khung hình Camera (Media Foundation), Microphone (WaveIn PCM), card mạng Wi-Fi và Bluetooth.

### 4. Cơ sở Dữ liệu Điểm chuẩn Silicon CPU (Silicon Baseline DB)
- Tích hợp sẵn bảng điểm chuẩn tham chiếu `baselines/cpu_microbench.tsv` cho các dòng chip phổ biến:
  - **Intel Core H-Series**: i7-11800H, i7-11850H, i9-11900H, i9-11950H, i7-10750H, i7-12700H, i7-12800H, i9-12900H, i7-13700H, i9-13900H.
  - **Intel Core P/U-Series**: i7-1165G7, i7-1185G7, i5-1135G7, i5-1145G7, i7-1260P, i7-1280P, i5-1240P, i5-1250P.
  - **AMD Ryzen Pro & Mobile**: Ryzen 7 5800U, Ryzen 7 PRO 5850U, Ryzen 5 5600U, Ryzen 5 PRO 5650U, Ryzen 7 6800U, Ryzen 7 PRO 6850U.
- Nếu điểm bài test Microbench thấp hơn mức sàn $	o$ Báo động **`BELOW BASELINE` (CPU bị bóp xung/thermal throttle do keo khô hoặc tản nhiệt hỏng)**.

---

## 💻 Hỗ trợ Chuyên sâu các Dòng máy

### Hệ sinh thái Dell Toàn diện
1. **Dell Latitude**: 7490, 7400, 7410, 7420, 7430, 5400, 5410, 5420, 5430, 3420.
2. **Dell XPS**: XPS 13 (9300, 9310, 9320 Plus), XPS 15 (9500, 9510, 9520), XPS 17 (9710).
3. **Dell Precision Workstation**: 3560, 5550, 5560, 5570, 7550, 7560, 7670.
4. **Dell Inspiron & Vostro**: Vostro 5410, Inspiron 14/15/16.
5. **Dell Gaming**: G15 5511 (Thunderbolt 4, Killer 2.5G LAN, HDMI 2.1).
6. **Dell Universal Architecture Engine**: Tự động nhận diện và sinh layout cổng cho bất kỳ máy Dell nào kể cả khi chưa có profile tĩnh.
7. **Dell Base36 Express Service Code**: Tự động chuyển đổi Service Tag sang Express Service Code và sinh link tra cứu trực tiếp trên Dell Support Portal.

### Hệ sinh thái Lenovo ThinkPad
- **ThinkPad X1 Carbon**: Gen 9, Gen 10.
- **ThinkPad T-Series**: T14 Gen 2 (Intel), T14s Gen 2, T14 Gen 3.
- **ThinkPad P-Series**: P1 Gen 4 Workstation.

---

## 🚀 Hướng dẫn Sử dụng

1. Tải file [**`LapSure-windows-x64-portable.zip`**](https://github.com/tuyenht/LapSure/releases/download/v0.1.0-beta/LapSure-windows-x64-portable.zip) và giải nén ra thư mục bất kỳ (hoặc chép vào USB mang đi).
2. Nhấp chuột phải vào `LapSure.exe` và chọn **Run as administrator** (Chạy với quyền Quản trị viên để phần mềm đọc được bảng điều khiển phần cứng từ Ring-0/WMI/SetupAPI).
3. Chọn chế độ kiểm tra:
   - **Nhanh (Quick - khuyên dùng)**: 60 giây kiểm tra tổng thể.
   - **Tiêu chuẩn (Standard)**: 3 phút kiểm tra độ ổn định.
   - **Chuyên sâu (Deep)**: Kiểm tra toàn diện bộ nhớ RAM và VRAM.
4. Nhấn **BẮT ĐẦU KIỂM TRA** để hệ thống tự động quét toàn bộ.
5. Sau khi quét xong, nhấn **TIẾP TỤC BƯỚC KẾ** để thực hiện các bài test tương tác (Màn hình, Bàn phím, Trackpad, Loa, Cổng cắm, Ngoại hình).
6. Nhấn **XEM KẾT QUẢ** để mở báo cáo kỹ thuật `audit_report.html` đẹp mắt, chi tiết.

---

## 🛠️ Hướng dẫn Build từ Mã nguồn

### Yêu cầu Môi trường:
- Windows 10/11 x64
- Visual Studio 2022 (MSVC Toolset v143+) có cài đặt CMake và C++20 Desktop Development.

### Lệnh Build:
Mở **x64 Native Tools Command Prompt for VS 2022** tại thư mục dự án và chạy:
```cmd
cmake --preset build-msvc-x64-release
cmake --build --preset build-msvc-x64-release
```

### Chạy Bộ Kiểm thử Hồi quy (Regression Suite):
```cmd
run_source_tests.cmd
```
*Tất cả 37 Sanity Tests và 17 Behavioral Tests sẽ được tự động thực thi.*

---

## 📄 Bản quyền & Tác giả

- **Tác giả & Kiến trúc sư trưởng**: TuyenHT (LapSure Core Engineering Team)
- **Repo chính thức**: [https://github.com/tuyenht/LapSure](https://github.com/tuyenht/LapSure)
- **Bản quyền**: © 2026 LapSure Project. All rights reserved.

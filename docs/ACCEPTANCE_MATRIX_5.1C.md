# LapSure v0.1.1-beta Physical Smoke & Acceptance Matrix (Round 5.1C)

## 1. Mục đích & Nguyên tắc cốt lõi
Tài liệu này xác định ma trận kiểm thử thực tế trên phần cứng vật lý (Dell Precision 5560 / 5570 / 7670 và các dòng máy tương đương) trước khi phát hành phiên bản chính thức.

### Nguyên tắc bất biến:
- **Evidence before verdict**: Bằng chứng kỹ thuật phải có trước khi đưa ra kết luận.
- **Presence is not functionality**: Nhận diện được thiết bị không đồng nghĩa với hoạt động hoàn hảo.
- **Factory mismatch is not hardware failure**: Mismatch cấu hình xuất xưởng là cảnh báo nguồn gốc/rủi ro thay thế, không phải lỗi hỏng phần cứng.
- **Fail-closed**: Thiếu dữ liệu, timeout, thiếu quyền hạn hoặc engine không tin cậy phải trả về `INCOMPLETE`, tuyệt đối không tự ý gán `PASS`.
- **One session validates only that session**: Một phiên test chỉ chứng thực cho chính chiếc máy và phiên đó, không thể tự chứng thực cho cả một dòng sản phẩm.

---

## 2. Ma trận Viewport & Tỉ lệ thu phóng DPI

| Cấu hình màn hình | Độ phân giải | DPI Scaling | Tiêu chí hiển thị |
|---|---|---|---|
| **Compatibility Base** | 1366 × 768 | 100%, 125%, 150% | Sidebar, Card, Table hiển thị đầy đủ, không tràn màn hình, không mất chữ. |
| **Precision FHD+ Standard** | 1920 × 1200 | 100%, 125%, 150% | Tỉ lệ chuẩn 16:10, phông chữ Segoe UI sắc nét, layout 2 cột cân đối. |
| **Precision UHD+ High-DPI** | 3840 × 2400 | 150%, 200%, 225% | Không vỡ icon, không mờ text, Direct2D/GDI render đúng scaling factor. |
| **FHD Legacy (Optional)** | 1920 × 1080 | 100%, 125% | Regression kiểm tra tương thích ngược 16:9. |

---

## 3. Quy trình Smoke Test vật lý (S01 – S23)

### Bước 1: Khởi động & Khảo sát tự động (S01 -> S04)
1. Chạy `LapSure.exe` với quyền **Administrator**.
2. **S01 Overview**: Kiểm tra thông tin tóm tắt, nút "BẮT ĐẦU KIỂM TRA".
3. **S02 New Session**: Nhập Service Tag/Serial, chọn profile xuất xưởng (nếu có).
4. **S03 Seller Claim**: Nhập thông tin người bán cam kết (CPU, RAM, Ổ cứng, Giá tiền).
5. **S04 Auto Audit**: Kiểm tra quá trình quét phần cứng tự động chạy ngầm (CPU, RAM DIMM, Disk NVMe/SATA, Battery live telemetry, EDID Display, GPU).

### Bước 2: Kiểm tra tương tác vật lý (S05 – S07)
1. **S05 Phím & Chuột**:
   - Gõ lần lượt từng phím trên bàn phím -> Ma trận phím (Visual Keyboard Matrix) đổi màu xanh.
   - Click chuột trái, chuột phải, cuộn bánh xe touchpad.
2. **S06 An toàn & Ngoại hình**:
   - Hoàn thành wizard 6 bước kiểm tra ngoại hình: Bản lề, Vỏ máy, Ốc vít/Dấu vết can thiệp, Dấu hiệu vào nước, Biến dạng pin, Củ sạc.
3. **S07 Cổng kết nối (SessionPortAttestation)**:
   - Cắm thiết bị USB/Type-C/HDMI/SD vào từng cổng được chỉ dẫn.
   - Kiểm tra hệ thống nhận diện cổng theo ID tĩnh, cập nhật trạng thái `Present` và `PASS`.

### Bước 3: Ổn định & Đo tải (S08)
1. Chạy bài kiểm tra CPU load, RAM online integrity và VRAM stress.
2. Kiểm tra biểu đồ nhiệt độ và công suất thời gian thực.
3. Xác nhận không có lỗi phần cứng bất thường (WHEA, Disk, Display, BugCheck).

### Bước 4: Xem kết quả & Xuất báo cáo (S18, S19, S22, S23)
1. **S18 Final Report**: Kiểm tra kết luận tổng thể (`BUY`, `CAUTION`, `REJECT`, hoặc `INCOMPLETE`).
2. **S19 Export**: Xuất báo cáo kép `session_<id>.html` và `session_<id>.json`. Kiểm tra file mở được trên trình duyệt với CSS chuẩn.
3. **S22 History**: Xem lại lịch sử phiên đã lưu (Schema v2).
4. **S23 Recovery**: Giả lập đóng đột ngột ứng dụng trong lúc stress -> Mở lại xác nhận có thông báo phiên bị gián đoạn.

---

## 4. Tiêu chí nghiệm thu Đạt (Pass Criteria)
- [x] Không có lỗi sập phần mềm (0 crash / 0 unhandled exception).
- [x] 0 lỗi mức P0 / P1.
- [x] Không có trường hợp nhận diện sai hoặc bịa đặt thông số kỹ thuật.
- [x] Báo cáo JSON & HTML lưu đúng các trường metadata chính sách `decisionPolicyVersion="5.1.0"`, `coveragePolicyVersion="5.1.0"`, `authorityPolicyVersion="5.1.0"`.
- [x] Gói Portable ZIP giải nén và chạy được ngay trên Windows 10/11 và WinPE mà không cần cài đặt.

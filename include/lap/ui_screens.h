#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "lap/model.h"
#include "lap/ui_theme.h"
#include "lap/ui_state.h"
#include "lap/ui_components.h"

namespace lap {

// S01 — Tổng quan
void RenderScreenS01_Overview(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                              const std::wstring& selectedMode, bool running, bool auditReady,
                              CanonicalUiState sessionLifecycleState, int auditCompletedItems, int auditTotalItems,
                              int focusIndex = 0);

// S02 — Phiên kiểm định mới
void RenderScreenS02_NewSession(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int focusIndex = 0);

// S03 — Cam kết người bán
void RenderScreenS03_SellerClaim(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int focusIndex = 0);

// S04 — Kiểm tra Tự động
void RenderScreenS04_AutoAudit(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                               const std::wstring& selectedMode, bool running, bool paused, int auditCompletedItems,
                               int auditTotalItems, int auditCurrentStage, int auditElapsedSec,
                               const std::vector<LiveLogEntry>& liveLogs, int focusIndex = 0);

// S05 — Kiểm tra Chức năng
void RenderScreenS05_Functional(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int subTab, const std::vector<int>& keyStates,
                                const std::vector<POINT>& touchpadTrail, bool touchpadDone,
                                int focusIndex = 0);

// S06 — Ngoại hình & An toàn
void RenderScreenS06_PhysicalSafety(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int activeStep, int activeHotspot, const std::vector<int>& checkState,
                                    int focusIndex = 0);

// S07 — Cổng & Nguồn
void RenderScreenS07_PortsPower(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activePort, int focusIndex = 0);

// S08 — Stress & Ổn định
void RenderScreenS08_StressStability(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                     bool running, int elapsedSec, const std::vector<float>& cpuTemps,
                                     const std::vector<float>& gpuTemps, const std::vector<float>& freqs,
                                     const std::vector<float>& powers, const std::vector<LiveLogEntry>& liveLogs,
                                     int focusIndex = 0);

// S09 — Pin & Năng lượng
void RenderScreenS09_BatteryPower(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                  const std::vector<float>& chargeHistory, const std::vector<float>& powerHistory,
                                  int focusIndex = 0);

// S10 — Lưu trữ
void RenderScreenS10_Storage(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             int selectedDrive, int tableScrollOffset = 0, int focusIndex = 0);

// S11 — Bộ nhớ (RAM)
void RenderScreenS11_Memory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                            int tableScrollOffset = 0, int focusIndex = 0);

// S12 — Hiển thị (Màn hình)
void RenderScreenS12_Display(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             int colorIndex, const std::vector<int>& defectCheckStates,
                             int focusIndex = 0);

// S13 — Âm thanh & Camera
void RenderScreenS13_AudioCamera(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int activeTest, int focusIndex = 0);

// S14 — Mạng & Kết nối
void RenderScreenS14_Network(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             const std::vector<float>& rssiHistory, const std::vector<LiveLogEntry>& netLogs,
                             int focusIndex = 0);

// S15 — Thông tin Hệ thống
void RenderScreenS15_SystemInfo(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int tableScrollOffset = 0, int focusIndex = 0);

// S16 — Hồ sơ & Đối chiếu
void RenderScreenS16_FactoryCompare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int tableScrollOffset = 0, int focusIndex = 0);

// S17 — Thư viện Bằng chứng
void RenderScreenS17_EvidenceLibrary(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                     int activeFilter, int selectedItem, int viewMode,
                                     int focusIndex = 0);

// S18 — Đánh giá cuối cùng & Báo cáo
void RenderScreenS18_FinalReport(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int focusIndex = 0);

// S19 — Xuất báo cáo & Chia sẻ
void RenderScreenS19_ExportShare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int selectedFormat, int shareFlags, int focusIndex = 0);

// S20 — Nhật ký & Sự kiện
void RenderScreenS20_LogsEvents(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activeFilter, int selectedLogIdx, const std::vector<LiveLogEntry>& liveLogs,
                                int tableScrollOffset = 0, int focusIndex = 0);

// S21 — Cài đặt
void RenderScreenS21_Settings(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                              int selectedCategory, int focusIndex = 0);

// S22 — Lịch sử phiên kiểm định
void RenderScreenS22_SessionHistory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int tableScrollOffset = 0, int focusIndex = 0);

// S23 — Khôi phục Phiên bị Gián đoạn
void RenderScreenS23_InterruptedRecovery(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                         int focusIndex = 0);

} // namespace lap

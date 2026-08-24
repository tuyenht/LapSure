@echo off
chcp 65001 >nul
echo ========================================================
echo   CÀI ĐẶT CHỨNG CHỈ KÝ SỐ LAPSURE (TRUSTED PUBLISHER)
echo ========================================================
echo.

set "CER_PATH=%~dp0LapSure_CodeSigning.cer"
if not exist "%CER_PATH%" (
    set "CER_PATH=%~dp0..\resources\LapSure_CodeSigning.cer"
)

if not exist "%CER_PATH%" (
    echo [LỖI] Không tìm thấy file LapSure_CodeSigning.cer!
    pause
    exit /b 1
)

echo Đang nạp chứng chỉ vào Trusted Root & Trusted Publisher...
certutil -addstore -f "Root" "%CER_PATH%"
certutil -addstore -f "TrustedPublisher" "%CER_PATH%"

echo.
echo ========================================================
echo [THÀNH CÔNG] Đã xác minh nhà phát hành LapSure!
echo Hộp thoại Windows UAC giờ đây sẽ hiển thị màu xanh:
echo "Verified publisher: LapSure Laptop Diagnostics"
echo ========================================================
echo.
pause

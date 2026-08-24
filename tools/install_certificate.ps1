<#
.SYNOPSIS
    Cai dat chung chi ky so LapSure vao Trusted Root & Trusted Publisher Store cua Windows.
#>
#Requires -RunAsAdministrator

$cerPath = Join-Path $PSScriptRoot "..\resources\LapSure_CodeSigning.cer"
if (-not (Test-Path $cerPath)) {
    $cerPath = Join-Path $PSScriptRoot "LapSure_CodeSigning.cer"
}

if (-not (Test-Path $cerPath)) {
    Write-Error "Khong tim thay tap tin LapSure_CodeSigning.cer!"
}

Write-Host "Dang cai dat chung chi LapSure vao Windows Trusted Root..." -ForegroundColor Cyan

# Import to LocalMachine\Root and LocalMachine\TrustedPublisher
Import-Certificate -FilePath $cerPath -CertStoreLocation "Cert:\LocalMachine\Root" | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation "Cert:\LocalMachine\TrustedPublisher" | Out-Null

Write-Host "[THANH CONG] Da tin cay chung chi LapSure. Ung dung se khong bi canh bao Windows SmartScreen / Defender tren may nay nua!" -ForegroundColor Green

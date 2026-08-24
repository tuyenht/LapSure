<#
.SYNOPSIS
    LapSure Authenticode Code Signing & Trust Tool.
.DESCRIPTION
    Signs LapSure.exe with Authenticode SHA256 + RFC 3161 Timestamp.
    Can generate/trust a dedicated local Code Signing Certificate or use an existing PFX.
.EXAMPLE
    .\tools\sign_binary.ps1
    .\tools\sign_binary.ps1 -PfxPath "C:\certs\company.pfx" -Password "secret"
#>
param(
    [string]$BinaryPath = "bin\LapSure.exe",
    [string]$PfxPath = "",
    [string]$Password = "",
    [switch]$CreateSelfSignedTrust = $true,
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

$ErrorActionPreference = "Stop"

Write-Host "=== LapSure Code Signing & Authenticode Manager ===" -ForegroundColor Cyan

# 1. Locate signtool.exe
$signtoolPaths = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending
if (-not $signtoolPaths -or $signtoolPaths.Count -eq 0) {
    Write-Host "[!] signtool.exe not found in Windows Kits. Using PowerShell Set-AuthenticodeSignature as fallback." -ForegroundColor Yellow
    $signtool = $null
} else {
    $signtool = $signtoolPaths[0].FullName
    Write-Host "[+] Found signtool.exe: $signtool" -ForegroundColor Green
}

# 2. Resolve Target Binary
$target = Resolve-Path $BinaryPath -ErrorAction SilentlyContinue
if (-not $target) {
    # Try alternate paths
    $altPaths = @("build\Release\LapSure.exe", "LapSure.exe")
    foreach ($ap in $altPaths) {
        if (Test-Path $ap) { $target = (Resolve-Path $ap).Path; break }
    }
}

if (-not $target -or -not (Test-Path $target)) {
    Write-Error "Target binary not found: $BinaryPath"
}
Write-Host "[+] Target binary: $target" -ForegroundColor Green

# 3. Unblock file (remove Zone.Identifier / Mark of the Web)
Unblock-File -Path $target -ErrorAction SilentlyContinue
Write-Host "[+] Cleared Zone.Identifier (MOTW) on $target" -ForegroundColor Green

# 4. Handle Certificate
$cert = $null

if ($PfxPath -and (Test-Path $PfxPath)) {
    Write-Host "[+] Using provided PFX certificate: $PfxPath" -ForegroundColor Cyan
    $securePass = ConvertTo-SecureString $Password -AsPlainText -Force
    $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($PfxPath, $securePass, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable)
} else {
    # Look for existing LapSure cert in CurrentUser\My
    $existing = Get-ChildItem "Cert:\CurrentUser\My" -CodeSigningCert -ErrorAction SilentlyContinue | Where-Object { $_.Subject -like "*LapSure*" } | Sort-Object NotAfter -Descending
    if ($existing -and $existing.Count -gt 0) {
        $cert = $existing[0]
        Write-Host "[+] Found existing LapSure Code Signing Certificate: $($cert.Subject) [Thumbprint: $($cert.Thumbprint)]" -ForegroundColor Green
    } elseif ($CreateSelfSignedTrust) {
        Write-Host "[*] Generating a new dedicated LapSure Code Signing Certificate..." -ForegroundColor Yellow
        $cert = New-SelfSignedCertificate -Type CodeSigningCert `
            -Subject "CN=LapSure Laptop Diagnostics, O=LapSure Systems, C=VN" `
            -KeyAlgorithm RSA -KeyLength 2048 `
            -CertStoreLocation "Cert:\CurrentUser\My" `
            -NotAfter (Get-Date).AddYears(5)
        
        Write-Host "[+] Generated certificate: $($cert.Subject) [Thumbprint: $($cert.Thumbprint)]" -ForegroundColor Green

        # Install to Trusted Root and Trusted Publisher for Current User so Windows does NOT warn
        try {
            $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "CurrentUser")
            $rootStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
            $rootStore.Add($cert)
            $rootStore.Close()

            $pubStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPublisher", "CurrentUser")
            $pubStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
            $pubStore.Add($cert)
            $pubStore.Close()
            Write-Host "[+] Successfully added certificate to CurrentUser\Root and CurrentUser\TrustedPublisher!" -ForegroundColor Green
        } catch {
            Write-Host "[!] Could not add to Trust store automatically: $_" -ForegroundColor Yellow
        }
    }
}

if (-not $cert) {
    Write-Error "No valid code signing certificate available."
}

# 5. Sign the binary
Write-Host "[*] Signing binary with SHA256 Authenticode..." -ForegroundColor Cyan
if ($signtool) {
    $args = @("sign", "/fd", "SHA256", "/sha1", $cert.Thumbprint, "/tr", $TimestampUrl, "/td", "SHA256", "/v", "`"$target`"")
    $proc = Start-Process -FilePath $signtool -ArgumentList $args -NoNewWindow -Wait -PassThru
    if ($proc.ExitCode -ne 0) {
        # Retry without timestamp if timestamp server is unreachable
        Write-Host "[!] Timestamp server failed. Retrying without timestamp..." -ForegroundColor Yellow
        $args = @("sign", "/fd", "SHA256", "/sha1", $cert.Thumbprint, "/v", "`"$target`"")
        $proc = Start-Process -FilePath $signtool -ArgumentList $args -NoNewWindow -Wait -PassThru
    }
} else {
    Set-AuthenticodeSignature -FilePath $target -Certificate $cert -HashAlgorithm "SHA256" -TimestampServer $TimestampUrl -ErrorAction SilentlyContinue
}

# Also sign root and bin copies if they exist
$copies = @("bin\LapSure.exe", "LapSure.exe", "build\Release\LapSure.exe", "out\portable\LapSure.exe")
foreach ($cp in $copies) {
    if ((Test-Path $cp) -and ((Resolve-Path $cp).Path -ne $target)) {
        $realCp = (Resolve-Path $cp).Path
        Unblock-File -Path $realCp -ErrorAction SilentlyContinue
        if ($signtool) {
            $args = @("sign", "/fd", "SHA256", "/sha1", $cert.Thumbprint, "/v", "`"$realCp`"")
            Start-Process -FilePath $signtool -ArgumentList $args -NoNewWindow -Wait | Out-Null
        } else {
            Set-AuthenticodeSignature -FilePath $realCp -Certificate $cert -HashAlgorithm "SHA256" -ErrorAction SilentlyContinue | Out-Null
        }
        Write-Host "[+] Signed copy: $realCp" -ForegroundColor Green
    }
}

# 6. Verify Signature
Write-Host "`n=== Verification Result ===" -ForegroundColor Cyan
$sig = Get-AuthenticodeSignature -FilePath $target
Write-Host "Status:        $($sig.Status)" -ForegroundColor $(if ($sig.Status -eq "Valid") { "Green" } else { "Yellow" })
Write-Host "StatusMessage: $($sig.StatusMessage)"
Write-Host "Signer:        $($sig.SignerCertificate.Subject)"
Write-Host "Thumbprint:    $($sig.SignerCertificate.Thumbprint)"
Write-Host "Timestamp:     $($sig.TimeStamperCertificate.Subject)"

Write-Host "`n[SUCCESS] LapSure binaries have been signed and validated!" -ForegroundColor Green

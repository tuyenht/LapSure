param(
  [Parameter(Mandatory = $true)][string]$ZipPath,
  [string]$ChecksumPath = "",
  [string]$ExpectedCommit = ""
)

$ErrorActionPreference = "Stop"
$zip = (Resolve-Path -LiteralPath $ZipPath).Path
if (!$ChecksumPath) { $ChecksumPath = "$zip.sha256" }
$checksum = (Resolve-Path -LiteralPath $ChecksumPath).Path

$expectedZipHash = ((Get-Content -LiteralPath $checksum -Raw).Trim() -split '\s+')[0].ToLowerInvariant()
if ($expectedZipHash -notmatch '^[0-9a-f]{64}$') { throw "Invalid ZIP checksum file: $checksum" }
$actualZipHash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualZipHash -ne $expectedZipHash) { throw "ZIP SHA-256 mismatch. Expected $expectedZipHash, got $actualZipHash" }

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("LapSureVerify-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot | Out-Null
try {
  Expand-Archive -LiteralPath $zip -DestinationPath $tempRoot
  foreach ($required in @("LapSure.exe", "BUILD_HASH.txt", "BUILD_INFO.txt", "profiles", "validation", "tools")) {
    if (!(Test-Path -LiteralPath (Join-Path $tempRoot $required))) { throw "Portable package is missing: $required" }
  }

  $hashLine = (Get-Content -LiteralPath (Join-Path $tempRoot "BUILD_HASH.txt") -Raw).Trim()
  if ($hashLine -notmatch '^LapSure\.exe sha256=([0-9a-fA-F]{64})$') { throw "BUILD_HASH.txt has an invalid format" }
  $expectedExeHash = $Matches[1].ToLowerInvariant()
  $actualExeHash = (Get-FileHash -LiteralPath (Join-Path $tempRoot "LapSure.exe") -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($actualExeHash -ne $expectedExeHash) { throw "LapSure.exe SHA-256 mismatch. Expected $expectedExeHash, got $actualExeHash" }

  $buildInfo = @{}
  Get-Content -LiteralPath (Join-Path $tempRoot "BUILD_INFO.txt") | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { $buildInfo[$Matches[1]] = $Matches[2] }
  }
  if ($buildInfo.product -ne "LapSure") { throw "Unexpected product in BUILD_INFO.txt" }
  if ($buildInfo.version -ne "0.1.1-beta") { throw "Unexpected version in BUILD_INFO.txt" }
  if (!$buildInfo.commit -or $buildInfo.commit -notmatch '^[0-9a-fA-F]{40}$') { throw "Missing or invalid commit provenance" }
  if ($ExpectedCommit -and $buildInfo.commit.ToLowerInvariant() -ne $ExpectedCommit.ToLowerInvariant()) {
    throw "Commit provenance mismatch. Expected $ExpectedCommit, got $($buildInfo.commit)"
  }

  Write-Host "PASS portable package integrity"
  Write-Host "ZIP SHA-256: $actualZipHash"
  Write-Host "EXE SHA-256: $actualExeHash"
  Write-Host "Commit: $($buildInfo.commit)"
} finally {
  if (Test-Path -LiteralPath $tempRoot) { Remove-Item -LiteralPath $tempRoot -Recurse -Force }
}

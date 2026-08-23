param(
  [string]$BuildDir = ".\out\build\msvc-x64-release\Release",
  [string]$OutputDir = ".\out\portable"
)
$ErrorActionPreference = "Stop"
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$archive = Join-Path (Split-Path $OutputDir -Parent) "LapSure-windows-x64-portable.zip"
$archiveHash = Join-Path (Split-Path $OutputDir -Parent) "LapSure-windows-x64-portable.zip.sha256"
if (Test-Path $OutputDir) { Remove-Item -LiteralPath $OutputDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$exe = Join-Path $BuildDir "LapSure.exe"
if (!(Test-Path $exe)) { throw "LapSure.exe not found: $exe" }
Copy-Item $exe $OutputDir -Force
foreach ($d in @("profiles","baselines","tools")) {
  if (Test-Path ".\$d") { Copy-Item ".\$d" $OutputDir -Recurse -Force }
}
foreach ($f in @("README.md","SECURITY.md","CHANGELOG.md")) {
  if (Test-Path ".\$f") { Copy-Item ".\$f" $OutputDir -Force }
}
foreach ($d in @("docs","validation")) {
  if (Test-Path ".\$d") { Copy-Item ".\$d" $OutputDir -Recurse -Force }
}
$hash=(Get-FileHash (Join-Path $OutputDir "LapSure.exe") -Algorithm SHA256).Hash.ToLower()
"LapSure.exe sha256=$hash" | Set-Content (Join-Path $OutputDir "BUILD_HASH.txt") -Encoding ascii
$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { (git rev-parse HEAD 2>$null) }
@(
  "product=LapSure"
  "version=0.1.1-beta"
  "commit=$commit"
  "configuration=Release"
  "target=windows-x64"
  "compiler=MSVC"
  "capability_manifest=tools/engine_manifest.txt"
) | Set-Content (Join-Path $OutputDir "BUILD_INFO.txt") -Encoding ascii
if (Test-Path $archive) { Remove-Item -LiteralPath $archive -Force }
Compress-Archive -Path (Join-Path $OutputDir "*") -DestinationPath $archive -CompressionLevel Optimal
$zipHash=(Get-FileHash $archive -Algorithm SHA256).Hash.ToLower()
"$zipHash  $(Split-Path $archive -Leaf)" | Set-Content $archiveHash -Encoding ascii
Write-Host "Portable package created: $archive"

param(
  [string]$BuildDir = ".\out\build\msvc-x64-release\Release",
  [string]$OutputDir = ".\out\portable"
)
$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$exe = Join-Path $BuildDir "LapSure.exe"
if (!(Test-Path $exe)) { throw "LapSure.exe not found: $exe" }
Copy-Item $exe $OutputDir -Force
foreach ($d in @("profiles","baselines","tools")) {
  if (Test-Path ".\$d") { Copy-Item ".\$d" $OutputDir -Recurse -Force }
}
Copy-Item ".\README.md" $OutputDir -Force
$hash=(Get-FileHash (Join-Path $OutputDir "LapSure.exe") -Algorithm SHA256).Hash.ToLower()
"sha256=$hash" | Set-Content (Join-Path $OutputDir "BUILD_HASH.txt") -Encoding ascii
Write-Host "Portable package created: $OutputDir"

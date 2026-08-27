param(
    [string]$OutputRoot = "$env:USERPROFILE\Desktop",
    [string]$Version = "0.2.0_20260826_r3"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repoRoot "bin\Release"
$packageName = "TunnelDetection_FieldTrial_$Version"
$destination = Join-Path $OutputRoot $packageName
$zipPath = "$destination.zip"

if (-not (Test-Path -LiteralPath (Join-Path $source "TunnelDetection.exe"))) {
    throw "Release build is missing: $source"
}
if ((Test-Path -LiteralPath $destination) -or (Test-Path -LiteralPath $zipPath)) {
    throw "Refusing to overwrite an existing field package: $destination"
}

New-Item -ItemType Directory -Path $destination | Out-Null
$filePatterns = @("*.exe", "*.dll", "*.xml", "*.qm")
foreach ($pattern in $filePatterns) {
    Get-ChildItem -LiteralPath $source -Filter $pattern -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $destination
    }
}
foreach ($directoryName in @("generic", "iconengines", "imageformats", "networkinformation",
                              "platforms", "styles", "tls", "translations", "config")) {
    $directory = Join-Path $source $directoryName
    if (Test-Path -LiteralPath $directory) {
        Copy-Item -LiteralPath $directory -Destination (Join-Path $destination $directoryName) -Recurse
    }
}

# Camera SDK runtime DLLs. Sapera drivers and licensing still require installation.
$saperaRuntime = @(
    "C:\Windows\System32\SapClassBasic86.dll",
    "C:\Windows\System32\CorApi.dll",
    "C:\Windows\System32\libiomp5md.dll",
    "C:\Windows\System32\msvcp120.dll",
    "C:\Windows\System32\msvcr120.dll"
)
foreach ($runtime in $saperaRuntime) {
    if (Test-Path -LiteralPath $runtime) { Copy-Item -LiteralPath $runtime -Destination $destination }
}
$hikRuntime = "C:\Program Files (x86)\Common Files\MVS\Runtime\Win64_x64"
if (Test-Path -LiteralPath $hikRuntime) {
    Get-ChildItem -LiteralPath $hikRuntime -File -Filter "*.dll" | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $destination
    }
}
$msvcRuntime = @(
    "C:\Windows\System32\msvcp140.dll",
    "C:\Windows\System32\vcruntime140.dll",
    "C:\Windows\System32\vcruntime140_1.dll"
)
foreach ($runtime in $msvcRuntime) {
    if (Test-Path -LiteralPath $runtime) { Copy-Item -LiteralPath $runtime -Destination $destination }
}

Copy-Item -LiteralPath (Join-Path $repoRoot "tools\Field_Self_Test.bat") -Destination $destination
Copy-Item -LiteralPath (Join-Path $repoRoot "tools\Run_TunnelDetection_Field.bat") -Destination $destination
Copy-Item -LiteralPath (Join-Path $repoRoot "tools\Collect_Field_Diagnostics.ps1") -Destination $destination
Copy-Item -LiteralPath (Join-Path $repoRoot "FIELD_TRIAL_ZARR.md") -Destination $destination

$gitSha = (& git -C $repoRoot rev-parse HEAD).Trim()
$dirty = if ((& git -C $repoRoot status --porcelain)) { "yes" } else { "no" }
$buildInfo = @"
product=TunnelDetection
version=$Version
build_id=0.2.0-fieldtrial-20260826
source_commit=$gitSha
source_worktree_dirty=$dirty
built_at=$(Get-Date -Format o)
zarr_format=v3
"@
[System.IO.File]::WriteAllText((Join-Path $destination "BUILD_INFO.txt"), $buildInfo,
    [System.Text.UTF8Encoding]::new($false))

Compress-Archive -LiteralPath $destination -DestinationPath $zipPath -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
Write-Output "PACKAGE=$destination"
Write-Output "ZIP=$zipPath"
Write-Output "SHA256=$hash"

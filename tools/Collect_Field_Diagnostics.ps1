param(
    [string]$DataRoot = "",
    [string]$OutputZip = ""
)

$ErrorActionPreference = "Stop"
$appDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
if ([string]::IsNullOrWhiteSpace($OutputZip)) {
    $OutputZip = Join-Path $appDir "TunnelDetection_Diagnostics_$stamp.zip"
}
if (Test-Path -LiteralPath $OutputZip) {
    throw "Refusing to overwrite diagnostics archive: $OutputZip"
}

$staging = Join-Path ([System.IO.Path]::GetTempPath()) "TunnelDetection_Diagnostics_$stamp"
$tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$stagingFull = [System.IO.Path]::GetFullPath($staging)
if (-not $stagingFull.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    -not ([System.IO.Path]::GetFileName($stagingFull)).StartsWith("TunnelDetection_Diagnostics_")) {
    throw "Unsafe diagnostics staging path: $stagingFull"
}
if (Test-Path -LiteralPath $staging) {
    throw "Unexpected existing staging directory: $staging"
}
New-Item -ItemType Directory -Path $staging | Out-Null
try {
    $logDir = Join-Path $appDir "logs"
    if (Test-Path -LiteralPath $logDir) {
        Copy-Item -LiteralPath $logDir -Destination (Join-Path $staging "logs") -Recurse
    }
    foreach ($name in @("field_self_test.log", "Config.xml", "BUILD_INFO.txt")) {
        $source = Join-Path $appDir $name
        if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $staging }
    }
    if (-not [string]::IsNullOrWhiteSpace($DataRoot) -and (Test-Path -LiteralPath $DataRoot)) {
        $diagnosticsDir = Join-Path $staging "acquisition_manifests"
        New-Item -ItemType Directory -Path $diagnosticsDir | Out-Null
        Get-ChildItem -LiteralPath $DataRoot -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -in @("acquisition_manifest.json", "acquisition_failure.json") } |
            ForEach-Object {
                $safeName = ($_.FullName -replace '[:\\/]', '_')
                Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $diagnosticsDir $safeName)
            }
    }
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $OutputZip -CompressionLevel Optimal
    Write-Output "Diagnostics: $OutputZip"
}
finally {
    if (Test-Path -LiteralPath $stagingFull) {
        Remove-Item -LiteralPath $stagingFull -Recurse -Force
    }
}

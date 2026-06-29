$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue

if ($cmakeCommand) {
    $cmakePath = $cmakeCommand.Source
} else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "cmake.exe is not on PATH and vswhere.exe was not found. Install CMake or Visual Studio C++ CMake tools."
    }

    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
    if (-not $installPath) {
        $installPath = & $vswhere -latest -products * -property installationPath
    }

    $cmakePath = Join-Path $installPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        throw "Visual Studio's cmake.exe was not found at: $cmakePath"
    }
}

& $cmakePath @args
exit $LASTEXITCODE

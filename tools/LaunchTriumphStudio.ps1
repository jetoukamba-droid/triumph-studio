param(
    [string] $BuildDir = "",
    [switch] $NoLaunch
)

$ErrorActionPreference = "Stop"

function Select-ReleaseFile($Files) {
    $releaseFile = $Files |
        Where-Object { $_.FullName -match "\\Release\\" } |
        Select-Object -First 1
    if ($null -ne $releaseFile) {
        return $releaseFile
    }
    return $Files | Select-Object -First 1
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot "build"
}

$buildRoot = Resolve-Path -ErrorAction SilentlyContinue $BuildDir
if ($null -eq $buildRoot) {
    throw "Build folder not found: $BuildDir"
}

$app = Select-ReleaseFile (
    Get-ChildItem -Path $buildRoot -Recurse -File -Filter "Triumph Studio.exe"
)
if ($null -eq $app) {
    throw "Triumph Studio.exe was not found under $buildRoot. Build Release first."
}

$scanner = Select-ReleaseFile (
    Get-ChildItem -Path $buildRoot -Recurse -File -Filter "TriumphPluginScanner.exe"
)
$appDirectory = Split-Path -Parent $app.FullName
$scannerNames = @("TriumphPluginScanner.exe", "Triumph Plugin Scanner.exe")
$scannerBesideApp = $scannerNames |
    ForEach-Object { Join-Path $appDirectory $_ } |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1

if ($null -eq $scannerBesideApp) {
    if ($null -eq $scanner) {
        throw "TriumphPluginScanner.exe was not found under $buildRoot. Rebuild Release."
    }
    foreach ($name in $scannerNames) {
        Copy-Item $scanner.FullName (Join-Path $appDirectory $name) -Force
    }
}

Write-Host "Triumph Studio: $($app.FullName)"
Write-Host "Scanner folder: $appDirectory"

if (-not $NoLaunch) {
    & $app.FullName
}

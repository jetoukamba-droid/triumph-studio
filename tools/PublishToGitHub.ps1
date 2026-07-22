param(
    [Parameter(Mandatory = $true)]
    [string] $RemoteUrl,

    [string] $Branch = "main",
    [string] $Message = "Add Triumph Studio Windows release gate"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git was not found. Install Git for Windows, then run this script again."
}

if (-not (Test-Path ".git")) {
    git init
}

git checkout -B $Branch

$origin = git remote get-url origin 2>$null
if ([string]::IsNullOrWhiteSpace($origin)) {
    git remote add origin $RemoteUrl
} elseif ($origin -ne $RemoteUrl) {
    git remote set-url origin $RemoteUrl
}

git add .
git commit -m $Message
git push -u origin $Branch

$commit = git rev-parse HEAD
Write-Host "Pushed Triumph Studio to $RemoteUrl on branch $Branch"
Write-Host "Commit: $commit"

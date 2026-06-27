# Quick prerequisite check before building OrcaSlicer-bambulab on Windows 11.
# Run from PowerShell: .\scripts\verify_windows_prereqs.ps1

$ErrorActionPreference = 'Continue'
$fail = 0

function Test-CommandExists([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Report([string]$Label, [bool]$Ok, [string]$Hint = '') {
    if ($Ok) {
        Write-Host "[OK]   $Label" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] $Label" -ForegroundColor Red
        if ($Hint) { Write-Host "       $Hint" -ForegroundColor Yellow }
        $script:fail++
    }
}

Write-Host ""
Write-Host "OrcaSlicer-bambulab — Windows build prerequisite check" -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host ""

Report "Git" (Test-CommandExists git) "winget install Git.Git"
Report "CMake" (Test-CommandExists cmake) "winget install Kitware.CMake"
Report "Python" (Test-CommandExists python) "winget install Python.Python.3.12"
Report "Perl (Strawberry)" (Test-CommandExists perl) "choco install strawberryperl -y"
Report "MSBuild (Visual Studio 2022)" (Test-CommandExists msbuild) "Install VS 2022 with Desktop development with C++"
Report "WSL" (Test-CommandExists wsl) "Admin: wsl --install --no-distribution"
Report "Docker (optional, for rootfs)" (Test-CommandExists docker) "Docker Desktop with WSL2 integration"

if (Test-CommandExists cmake) {
    $cmakePath = (Get-Command cmake).Source
    Write-Host "       cmake at: $cmakePath"
}
if (Test-CommandExists perl) {
    $perlPath = (Get-Command perl).Source
    Write-Host "       perl at:  $perlPath"
}

Write-Host ""
Write-Host "PATH order (CMake must appear BEFORE Strawberry\c\bin):" -ForegroundColor Cyan
$pathEntries = $env:Path -split ';'
$cmakeIdx = ($pathEntries | Select-String -Pattern 'CMake\\bin' | Select-Object -First 1)
$strawIdx = ($pathEntries | Select-String -Pattern 'Strawberry\\c\\bin' | Select-Object -First 1)
if ($cmakeIdx -and $strawIdx) {
    $ci = [array]::IndexOf($pathEntries, $cmakeIdx.ToString())
    $si = [array]::IndexOf($pathEntries, $strawIdx.ToString())
    if ($ci -ge 0 -and $si -ge 0 -and $ci -lt $si) {
        Write-Host "[OK]   CMake is before Strawberry in PATH" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Strawberry\c\bin is before CMake — configure will fail" -ForegroundColor Red
        $fail++
    }
}

Write-Host ""
Write-Host "Bridge artifacts (required before slicer build):" -ForegroundColor Cyan
$repoRoot = if ($PSScriptRoot) {
    [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
} else {
    (Get-Location).Path
}

$bridgeChecks = @(
    @{ Path = Join-Path $repoRoot 'tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\pjarczak_bambu_linux_host'; Label = 'Linux host binary' },
    @{ Path = Join-Path $repoRoot 'tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\pjarczak_bambu_linux_host_abi0'; Label = 'Linux host abi0' },
    @{ Path = Join-Path $repoRoot 'tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\pjarczak_bambu_linux_host_abi1'; Label = 'Linux host abi1' },
    @{ Path = Join-Path $repoRoot 'tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\ca-certificates.crt'; Label = 'CA certificates' },
    @{ Path = Join-Path $repoRoot 'tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar'; Label = 'WSL rootfs tar' }
)

foreach ($check in $bridgeChecks) {
    Report $check.Label (Test-Path $check.Path) $check.Path
}

Write-Host ""
if ($fail -eq 0) {
    Write-Host "All checks passed. Ready to run .\build_release_vs.bat deps" -ForegroundColor Green
} else {
    Write-Host "$fail check(s) failed. See docs/WINDOWS_CURSOR_QUICKSTART.md" -ForegroundColor Red
}
Write-Host ""
exit $fail

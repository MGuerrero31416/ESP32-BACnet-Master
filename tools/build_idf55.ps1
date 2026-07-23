param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$IdfArguments
)

$ErrorActionPreference = "Stop"

$ExpectedIdfPath = "C:\esp\v5.5.4\esp-idf"
$ExportScript = Join-Path $ExpectedIdfPath "export.ps1"

if (-not (Test-Path $ExportScript)) {
    throw "Required ESP-IDF installation not found: $ExpectedIdfPath"
}

Write-Host "Activating required ESP-IDF: $ExpectedIdfPath"

# Activate the exact ESP-IDF installation required by this project.
& $ExportScript

if ($LASTEXITCODE -ne 0) {
    throw "ESP-IDF 5.5.4 environment activation failed."
}

$ResolvedExpectedPath = (Resolve-Path $ExpectedIdfPath).Path.TrimEnd("\")
$ResolvedActualPath = (Resolve-Path $env:IDF_PATH).Path.TrimEnd("\")

if ($ResolvedActualPath -ne $ResolvedExpectedPath) {
    throw @"
Incorrect ESP-IDF environment.

Required: $ResolvedExpectedPath
Active:   $ResolvedActualPath

Build aborted before CMake or Ninja could run.
"@
}

$IdfVersion = (& idf.py --version 2>&1 | Out-String).Trim()

if ($LASTEXITCODE -ne 0) {
    throw "Unable to determine the active ESP-IDF version."
}

if ($IdfVersion -notmatch "5\.5\.4") {
    throw @"
Incorrect ESP-IDF version.

Required: ESP-IDF 5.5.4
Detected: $IdfVersion

Build aborted.
"@
}

if (-not $IdfArguments -or $IdfArguments.Count -eq 0) {
    $IdfArguments = @("build")
}

Write-Host ""
Write-Host "Environment verified:"
Write-Host "  IDF_PATH: $env:IDF_PATH"
Write-Host "  Version:  $IdfVersion"
Write-Host "  Command:  idf.py $($IdfArguments -join ' ')"
Write-Host ""

& idf.py @IdfArguments
exit $LASTEXITCODE
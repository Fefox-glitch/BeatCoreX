param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'BeatCoreX.uproject'
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$versionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path -LiteralPath $buildScript) -or -not (Test-Path -LiteralPath $versionFile)) {
    Write-Output 'BLOCKED: UE5.6 Build.bat/Build.version not found. Specify -EngineRoot with a real installation.'
    exit 2
}
$engineVersion = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 6) {
    Write-Output 'BLOCKED: expected Unreal Engine 5.6.'
    exit 2
}
$evidenceDirectory = Join-Path $projectRoot 'Saved\Validation'
New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
$buildLog = Join-Path $evidenceDirectory 'EditorBuild.log'
& $buildScript BeatCoreXEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE 2>&1 |
    Tee-Object -FilePath $buildLog
$buildExitCode = $LASTEXITCODE
Write-Output "Editor build exit code: $buildExitCode"
exit $buildExitCode

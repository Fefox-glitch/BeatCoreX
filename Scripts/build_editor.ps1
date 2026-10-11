param(
    [string]$EngineRoot
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'BeatCoreX.uproject'
$evidenceDirectory = Join-Path $projectRoot 'Saved\Validation'
New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
if (-not $EngineRoot) {
    # Prefer a real version file; folder names and Launcher manifests alone
    # are not evidence that an installation matches the required engine.
    $candidateRoots = @((Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'))
    $launcherManifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $launcherManifest) {
        $manifest = Get-Content -LiteralPath $launcherManifest -Raw | ConvertFrom-Json
        $candidateRoots += @($manifest.InstallationList | ForEach-Object { $_.InstallLocation })
    }
    $registeredBuilds = Get-ItemProperty 'HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds' -ErrorAction SilentlyContinue
    if ($registeredBuilds) {
        $candidateRoots += @($registeredBuilds.PSObject.Properties |
            Where-Object { $_.Name -notlike 'PS*' } | ForEach-Object { $_.Value })
    }
    foreach ($candidate in ($candidateRoots | Where-Object { $_ } | Select-Object -Unique)) {
        $candidateVersion = Join-Path $candidate 'Engine\Build\Build.version'
        if (Test-Path -LiteralPath $candidateVersion) {
            $version = Get-Content -LiteralPath $candidateVersion -Raw | ConvertFrom-Json
            if ($version.MajorVersion -eq 5 -and $version.MinorVersion -eq 8) {
                $EngineRoot = $candidate
                break
            }
        }
    }
}
if (-not $EngineRoot) {
    'BLOCKED: UE5.8 not found. Specify -EngineRoot with a real installation.' |
        Tee-Object -FilePath (Join-Path $evidenceDirectory 'EditorBuild.log')
    exit 2
}
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$versionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path -LiteralPath $buildScript) -or -not (Test-Path -LiteralPath $versionFile)) {
    Write-Output 'BLOCKED: UE5.8 Build.bat/Build.version not found. Specify -EngineRoot with a real installation.'
    exit 2
}
$engineVersion = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8) {
    Write-Output 'BLOCKED: expected Unreal Engine 5.8.'
    exit 2
}
$projectDescriptor = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
if ($projectDescriptor.EngineAssociation -ne '5.8') {
    Write-Output 'BLOCKED: project EngineAssociation must be 5.8.'
    exit 2
}
$engineVersion | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidenceDirectory 'EngineVersion.json')
$buildLog = Join-Path $evidenceDirectory 'EditorBuild.log'
"Engine: $EngineRoot ($($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion), CL $($engineVersion.Changelist))" |
    Tee-Object -FilePath $buildLog
# Let UBT detect the actual VS/MSVC/SDK and run UHT as part of the build.
# Do not infer build success from a successful preflight or IDE folder.
& $buildScript BeatCoreXEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE 2>&1 |
    Tee-Object -FilePath $buildLog -Append
$buildExitCode = $LASTEXITCODE
$buildExitCode | Set-Content -LiteralPath (Join-Path $evidenceDirectory 'EditorBuild.exitcode')
"Editor build exit code: $buildExitCode" | Tee-Object -FilePath $buildLog -Append
exit $buildExitCode

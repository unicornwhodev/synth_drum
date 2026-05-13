param(
    [string]$Configuration = "Release",
    [string]$BuildDir = "build",
    [string]$JuceDir = "",
    [switch]$BootstrapJuce,
    [switch]$RunTests,
    [switch]$SkipBuild,
    [string]$InnoSetupPath = "",
    [string]$AppVersion = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-BuildPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot,
        [Parameter(Mandatory = $true)]
        [string]$BuildDir
    )

    if ([System.IO.Path]::IsPathRooted($BuildDir)) {
        return $BuildDir
    }

    return (Join-Path $RepoRoot $BuildDir)
}

function Resolve-AppVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot,
        [string]$ExplicitVersion
    )

    if ($ExplicitVersion) {
        return $ExplicitVersion
    }

    $cmakeFile = Join-Path $RepoRoot "synth_drum\CMakeLists.txt"
    if (Test-Path $cmakeFile) {
        $cmakeText = Get-Content -LiteralPath $cmakeFile -Raw
        $versionMatch = [regex]::Match($cmakeText, 'project\([^\s\)]+\s+VERSION\s+([0-9]+(?:\.[0-9]+){0,3})')
        if ($versionMatch.Success) {
            return $versionMatch.Groups[1].Value
        }
    }

    return "1.0.1"
}

function Resolve-InnoCompiler {
    param([string]$ExplicitPath)

    $candidates = @()
    if ($ExplicitPath) {
        $candidates += $ExplicitPath
    }

    $pathCommand = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($pathCommand) {
        $candidates += $pathCommand.Source
    }

    $candidates += @(
        "D:\InnoSetup6\ISCC.exe",
        "D:\Programs\Inno Setup 6\ISCC.exe",
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe"
    )

    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Inno Setup compiler ISCC.exe was not found. Pass -InnoSetupPath with the full path to ISCC.exe."
}

$repoRoot = Split-Path -Parent $PSCommandPath
$installerDir = Join-Path $repoRoot "installer"
$issPath = Join-Path $installerDir "UWdeVST_Drum.iss"
$buildPath = Resolve-BuildPath -RepoRoot $repoRoot -BuildDir $BuildDir
$resolvedVersion = Resolve-AppVersion -RepoRoot $repoRoot -ExplicitVersion $AppVersion

if (-not (Test-Path -LiteralPath $issPath)) {
    throw "Inno Setup script not found at $issPath."
}

if (-not $SkipBuild) {
    $buildScript = Join-Path $repoRoot "_build_all.ps1"
    $buildArgs = @{
        Configuration = $Configuration
        BuildDir = $BuildDir
    }

    if ($JuceDir) {
        $buildArgs["JuceDir"] = $JuceDir
    }

    if ($BootstrapJuce) {
        $buildArgs["BootstrapJuce"] = $true
    }

    if ($RunTests) {
        $buildArgs["RunTests"] = $true
    }

    & $buildScript @buildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed. Installer generation stopped."
    }
}

$artifactRoot = Join-Path $buildPath "UWdeVST_drum_artefacts\$Configuration"
$standaloneExe = Join-Path $artifactRoot "Standalone\UWdeVST_drum.exe"
$vst3Bundle = Join-Path $artifactRoot "VST3\UWdeVST_drum.vst3"

if (-not (Test-Path -LiteralPath $standaloneExe -PathType Leaf)) {
    throw "Standalone executable not found at $standaloneExe. Run the build first or remove -SkipBuild."
}

if (-not (Test-Path -LiteralPath $vst3Bundle -PathType Container)) {
    throw "VST3 bundle not found at $vst3Bundle. Run the build first or remove -SkipBuild."
}

$stageDir = Join-Path $installerDir "staging"
$standaloneStageDir = Join-Path $stageDir "Standalone"
$vst3StageDir = Join-Path $stageDir "VST3"

if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}

New-Item -ItemType Directory -Path $standaloneStageDir -Force | Out-Null
New-Item -ItemType Directory -Path $vst3StageDir -Force | Out-Null

Copy-Item -LiteralPath $standaloneExe -Destination $standaloneStageDir -Force
Copy-Item -LiteralPath $vst3Bundle -Destination $vst3StageDir -Recurse -Force

$iscc = Resolve-InnoCompiler -ExplicitPath $InnoSetupPath
$innoArgs = @(
    "/DAppVersion=$resolvedVersion",
    $issPath
)

Write-Host "Using Inno Setup: $iscc"
Write-Host "Packaging version: $resolvedVersion"

& $iscc @innoArgs
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE."
}

$setupPath = Join-Path $installerDir "output\UWdeVST_Drum_${resolvedVersion}_Windows_x64_Setup.exe"
if (Test-Path -LiteralPath $setupPath) {
    Write-Host "Installer created: $setupPath"
} else {
    Write-Host "Installer generation completed. Check the output folder: $(Join-Path $installerDir 'output')"
}

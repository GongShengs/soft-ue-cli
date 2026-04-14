<#
.SYNOPSIS
    Install SoftUEBridge bundled CodeBuddy skills into the current UE project.

.DESCRIPTION
    Copies all skills from Plugins/SoftUEBridge/skills/ to <ProjectRoot>/.codebuddy/skills/.
    Detects the project root by walking up from the script location looking for a .uproject file.
    Existing skills with the same name will be overwritten (updated).

.EXAMPLE
    # Run from any directory — the script auto-detects the project root:
    powershell -ExecutionPolicy Bypass -File "Plugins/soft-ue-cli/install-skills.ps1"

    # Or specify a target project explicitly:
    powershell -ExecutionPolicy Bypass -File "install-skills.ps1" -ProjectRoot "D:\MyProject"
#>

param(
    [string]$ProjectRoot = ""
)

$ErrorActionPreference = "Stop"

# ── Locate paths ─────────────────────────────────────────────────────────────

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SkillsSourceDir = Join-Path $ScriptDir "skills"

if (-not (Test-Path $SkillsSourceDir)) {
    Write-Host "ERROR: Skills source directory not found: $SkillsSourceDir" -ForegroundColor Red
    Write-Host "Make sure this script is in the soft-ue-cli plugin root alongside the 'skills' folder." -ForegroundColor Yellow
    exit 1
}

# Auto-detect project root if not specified
if ([string]::IsNullOrEmpty($ProjectRoot)) {
    $SearchDir = $ScriptDir
    while ($SearchDir -and $SearchDir -ne [System.IO.Path]::GetPathRoot($SearchDir)) {
        $UProjectFiles = Get-ChildItem -Path $SearchDir -Filter "*.uproject" -File -ErrorAction SilentlyContinue
        if ($UProjectFiles.Count -gt 0) {
            $ProjectRoot = $SearchDir
            break
        }
        $SearchDir = Split-Path -Parent $SearchDir
    }

    if ([string]::IsNullOrEmpty($ProjectRoot)) {
        Write-Host "ERROR: Could not find a .uproject file in any parent directory." -ForegroundColor Red
        Write-Host "Please specify -ProjectRoot explicitly." -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "Project root: $ProjectRoot" -ForegroundColor Cyan

$TargetDir = Join-Path (Join-Path $ProjectRoot ".codebuddy") "skills"

# ── Install skills ────────────────────────────────────────────────────────────

$SkillDirs = Get-ChildItem -Path $SkillsSourceDir -Directory -ErrorAction SilentlyContinue

if ($SkillDirs.Count -eq 0) {
    Write-Host "No skills found in $SkillsSourceDir" -ForegroundColor Yellow
    exit 0
}

Write-Host ""
Write-Host "Installing $($SkillDirs.Count) skill(s) from soft-ue-cli..." -ForegroundColor Green
Write-Host "  Source: $SkillsSourceDir"
Write-Host "  Target: $TargetDir"
Write-Host ""

$InstalledCount = 0

foreach ($SkillDir in $SkillDirs) {
    $SkillName = $SkillDir.Name
    $DestPath = Join-Path $TargetDir $SkillName

    # Check if SKILL.md exists (valid skill directory)
    $SkillMd = Join-Path $SkillDir.FullName "SKILL.md"
    if (-not (Test-Path $SkillMd)) {
        Write-Host "  SKIP: $SkillName (no SKILL.md found)" -ForegroundColor Yellow
        continue
    }

    # Create target directory
    if (-not (Test-Path $DestPath)) {
        New-Item -Path $DestPath -ItemType Directory -Force | Out-Null
    }

    # Copy all files recursively
    Copy-Item -Path (Join-Path $SkillDir.FullName "*") -Destination $DestPath -Recurse -Force

    $InstalledCount++
    if (Test-Path (Join-Path $DestPath "SKILL.md")) {
        Write-Host "  OK: $SkillName" -ForegroundColor Green
    } else {
        Write-Host "  WARN: $SkillName (copied but SKILL.md not found in target)" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Done! Installed $InstalledCount skill(s)." -ForegroundColor Green
Write-Host "Skills are now available in CodeBuddy IDE." -ForegroundColor Cyan

# ================================================================
#  X-Phage Installer for Windows (PowerShell)
#  Usage: irm https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/scripts/install.ps1 | iex
# ================================================================
$ErrorActionPreference = "Stop"

$REPO     = "AeonCoreX-Lab/X-Phage"
$BIN_DIR  = "$env:LOCALAPPDATA\xphage\bin"
$TMP_DIR  = "$env:TEMP\xphage-install-$PID"

function Write-Info  { param($m) Write-Host "info:  $m" -ForegroundColor Cyan }
function Write-Ok    { param($m) Write-Host $m -ForegroundColor Green }
function Write-Fail  { param($m) Write-Host "error: $m" -ForegroundColor Red; exit 1 }

# ── Banner ──────────────────────────────────────────────────────
Write-Host ""
Write-Host "  ██╗  ██╗      ██████╗ ██╗  ██╗ █████╗  ██████╗ ███████╗" -ForegroundColor Magenta
Write-Host "  ╚██╗██╔╝     ██╔══██╗██║  ██║██╔══██╗██╔════╝ ██╔════╝" -ForegroundColor Magenta
Write-Host "   ╚███╔╝ █████╗██████╔╝███████║███████║██║  ███╗█████╗  " -ForegroundColor Magenta
Write-Host "   ██╔██╗ ╚════╝██╔═══╝ ██╔══██║██╔══██║██║   ██║██╔══╝  " -ForegroundColor Magenta
Write-Host "  ██╔╝ ██╗      ██║     ██║  ██║██║  ██║╚██████╔╝███████╗" -ForegroundColor Magenta
Write-Host "  ╚═╝  ╚═╝      ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝" -ForegroundColor Magenta
Write-Host ""
Write-Host "  The X-Phage Programming Language Installer" -ForegroundColor Cyan
Write-Host "  https://github.com/$REPO" -ForegroundColor DarkGray
Write-Host ""

# ── Detect arch ─────────────────────────────────────────────────
$arch = if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64" -or
            [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq
            [System.Runtime.InteropServices.Architecture]::Arm64) {
    "arm64"
} else { "x64" }

$target = if ($arch -eq "arm64") { "xphage_windows_arm64" } else { "xphage_windows_x64" }

Write-Host "  Detected: windows $arch" -ForegroundColor DarkGray
Write-Host ""

New-Item -ItemType Directory -Force -Path $TMP_DIR | Out-Null

# ── Resolve latest release ──────────────────────────────────────
Write-Info "syncing channel updates for 'stable'"

try {
    $release = Invoke-RestMethod `
        -Uri "https://api.github.com/repos/$REPO/releases/latest" `
        -Headers @{ Accept = "application/vnd.github+json" }
    $version = $release.tag_name
} catch {
    Write-Fail "could not resolve latest version. Check your connection."
}

Write-Info "latest stable version is $version"

# ── Download ────────────────────────────────────────────────────
$base    = "https://github.com/$REPO/releases/download/$version"
$bin_url = "$base/$target"
$sha_url = "$base/$target.sha256"
$tmp_bin = "$TMP_DIR\xphage.exe"
$tmp_sha = "$TMP_DIR\xphage.sha256"

Write-Info "downloading xphage $version"

$wc = New-Object System.Net.WebClient
$wc.DownloadFile($bin_url, $tmp_bin)

if ((Get-Content $tmp_bin -Raw -ErrorAction SilentlyContinue) -match "<!DOCTYPE") {
    Write-Fail "binary not found for $target in $version"
}

# ── SHA256 verify ───────────────────────────────────────────────
Write-Info "verifying checksum for xphage"
try {
    $wc.DownloadFile($sha_url, $tmp_sha)
    $expected = (Get-Content $tmp_sha).Trim().Split(" ")[0].ToLower()
    $actual   = (Get-FileHash $tmp_bin -Algorithm SHA256).Hash.ToLower()
    if ($actual -eq $expected) {
        Write-Info "checksum verified"
    } else {
        Write-Fail "checksum mismatch - expected $expected, got $actual"
    }
} catch {
    Write-Host "  (checksum unavailable, skipped)" -ForegroundColor DarkGray
}

# ── Install ─────────────────────────────────────────────────────
Write-Info "installing to $BIN_DIR"
New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null
Copy-Item $tmp_bin "$BIN_DIR\xphage.exe" -Force

# ── PATH setup ──────────────────────────────────────────────────
$user_path = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($user_path -notlike "*$BIN_DIR*") {
    [Environment]::SetEnvironmentVariable("PATH", "$user_path;$BIN_DIR", "User")
    $env:PATH = "$env:PATH;$BIN_DIR"
}

# ── Cleanup ─────────────────────────────────────────────────────
Remove-Item -Recurse -Force $TMP_DIR -ErrorAction SilentlyContinue

# ── Done ────────────────────────────────────────────────────────
Write-Host ""
Write-Ok "  xphage is installed now. Great!"
Write-Host ""
Write-Host "  To get started you may need to restart your shell."
Write-Host "  This would reload your " -NoNewline
Write-Host "PATH" -ForegroundColor Cyan -NoNewline
Write-Host " environment variable to"
Write-Host "  include X-Phage's bin directory ($BIN_DIR)."
Write-Host ""
Write-Host "  USAGE:" -ForegroundColor DarkGray
Write-Host "  " -NoNewline; Write-Host "xphage --version" -ForegroundColor Cyan -NoNewline
Write-Host "          Print version info and exit"
Write-Host "  " -NoNewline; Write-Host "xphage init" -ForegroundColor Cyan -NoNewline
Write-Host "               Create a new project"
Write-Host "  " -NoNewline; Write-Host "xphage build" -ForegroundColor Cyan -NoNewline
Write-Host "              Build current project"
Write-Host ""
Write-Host "  https://github.com/$REPO" -ForegroundColor DarkGray
Write-Host ""

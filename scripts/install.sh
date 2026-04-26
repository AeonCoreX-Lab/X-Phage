#!/bin/bash
# ================================================================
#  🧬 X-Phage Installer
#  curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/scripts/install.sh | bash
# ================================================================
set -e

R='\033[1;31m' G='\033[1;32m' Y='\033[1;33m'
B='\033[1;34m' P='\033[1;35m' C='\033[1;36m'
DIM='\033[2m' NC='\033[0m'

REPO="AeonCoreX-Lab/X-Phage"
BIN_DIR="${XPHAGE_HOME:-$HOME/.xphage}/bin"
TMP_DIR="${TMPDIR:-/tmp}/xphage-install-$$"
trap 'rm -rf "$TMP_DIR"' EXIT
mkdir -p "$TMP_DIR"

# ── Banner ──────────────────────────────────────────────────────
echo ""
echo -e "${P}  ██╗  ██╗      ██████╗ ██╗  ██╗ █████╗  ██████╗ ███████╗${NC}"
echo -e "${P}  ╚██╗██╔╝     ██╔══██╗██║  ██║██╔══██╗██╔════╝ ██╔════╝${NC}"
echo -e "${P}   ╚███╔╝ █████╗██████╔╝███████║███████║██║  ███╗█████╗  ${NC}"
echo -e "${P}   ██╔██╗ ╚════╝██╔═══╝ ██╔══██║██╔══██║██║   ██║██╔══╝  ${NC}"
echo -e "${P}  ██╔╝ ██╗      ██║     ██║  ██║██║  ██║╚██████╔╝███████╗${NC}"
echo -e "${P}  ╚═╝  ╚═╝      ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝${NC}"
echo ""
echo -e "  ${C}The X-Phage Programming Language Installer${NC}"
echo -e "  ${DIM}https://github.com/${REPO}${NC}"
echo ""

# ── Detect platform ─────────────────────────────────────────────
_os="" _arch="" _target="" _ext=""
_uname_s="$(uname -s 2>/dev/null || echo unknown)"
_uname_m="$(uname -m 2>/dev/null || echo unknown)"

if [ -d "/data/data/com.termux/files/usr" ]; then
    _os="android"; _arch="arm64"
    _target="xphage_android_arm64"
    BIN_DIR="/data/data/com.termux/files/usr/bin"

elif [[ "$_uname_s" == MINGW* || "$_uname_s" == MSYS* || "$_uname_s" == CYGWIN* ]]; then
    _os="windows"; _ext=".exe"
    if [[ "$_uname_m" == aarch64 || "$_uname_m" == arm64 ]]; then
        _arch="arm64"; _target="xphage_windows_arm64"
    else
        _arch="x64"; _target="xphage_windows_x64"
    fi
    BIN_DIR="${USERPROFILE:-$HOME}/AppData/Local/xphage/bin"

elif [[ "$_uname_s" == "Darwin" ]]; then
    _os="macos"
    _target="xphage_mac_universal"
    [[ "$_uname_m" == "arm64" ]] && _arch="arm64" || _arch="x64"

elif [[ "$_uname_s" == "Linux" ]]; then
    _os="linux"
    if [[ "$_uname_m" == aarch64 || "$_uname_m" == arm64 ]]; then
        _arch="arm64"; _target="xphage_linux_arm64"
    elif [[ "$_uname_m" == x86_64 ]]; then
        _arch="x64"; _target="xphage_linux_x64"
    else
        echo -e "${R}error:${NC} unsupported architecture: $_uname_m" >&2; exit 1
    fi
else
    echo -e "${R}error:${NC} unsupported OS: $_uname_s" >&2; exit 1
fi

echo -e "${DIM}  Detected: ${_os} ${_arch}${NC}"
echo ""

command -v curl &>/dev/null || { echo -e "${R}error:${NC} curl is required" >&2; exit 1; }

# ── Resolve latest release ──────────────────────────────────────
echo -e "${B}info:${NC}  syncing channel updates for 'stable'"

_release="$TMP_DIR/release.json"
curl -sL --max-time 30 \
    -H "Accept: application/vnd.github+json" \
    "https://api.github.com/repos/${REPO}/releases/latest" \
    -o "$_release"

_version=$(grep '"tag_name"' "$_release" | head -1 \
    | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')

[ -z "$_version" ] && {
    echo -e "${R}error:${NC} could not resolve latest version" >&2; exit 1; }

echo -e "${B}info:${NC}  latest stable version is ${G}${_version}${NC}"

# ── Download ────────────────────────────────────────────────────
_base="https://github.com/${REPO}/releases/download/${_version}"
_tmp_bin="$TMP_DIR/xphage${_ext}"
_tmp_sha="$TMP_DIR/xphage.sha256"

echo -e "${B}info:${NC}  downloading xphage ${_version}"
curl -L --progress-bar --max-time 120 "${_base}/${_target}" -o "$_tmp_bin"

# Detect HTML error page
if file "$_tmp_bin" 2>/dev/null | grep -qi "HTML\|text"; then
    echo -e "${R}error:${NC} binary not found for this platform in ${_version}" >&2
    echo "  Visit: https://github.com/${REPO}/releases" >&2
    exit 1
fi

# ── SHA256 verify ───────────────────────────────────────────────
echo -e "${B}info:${NC}  verifying checksum for xphage"
_sha_ok=false
if curl -sL --max-time 10 "${_base}/${_target}.sha256" -o "$_tmp_sha" 2>/dev/null; then
    _expected=$(awk '{print $1}' "$_tmp_sha" 2>/dev/null)
    if [ -n "$_expected" ] && ! echo "$_expected" | grep -qi "not found\|404\|<!"; then
        if command -v sha256sum &>/dev/null; then
            _actual=$(sha256sum "$_tmp_bin" | awk '{print $1}')
        elif command -v shasum &>/dev/null; then
            _actual=$(shasum -a 256 "$_tmp_bin" | awk '{print $1}')
        fi
        if [ "$_actual" = "$_expected" ]; then _sha_ok=true
        else
            echo -e "${R}error:${NC} checksum mismatch — corrupted download" >&2
            echo "  expected: $_expected" >&2; echo "  got: $_actual" >&2; exit 1
        fi
    fi
fi
$_sha_ok && echo -e "${B}info:${NC}  checksum verified" \
          || echo -e "${DIM}  (checksum unavailable, skipped)${NC}"

# ── Install ─────────────────────────────────────────────────────
echo -e "${B}info:${NC}  installing to ${BIN_DIR}"
chmod +x "$_tmp_bin"
mkdir -p "$BIN_DIR"
cp "$_tmp_bin" "$BIN_DIR/xphage${_ext}"

# ── PATH auto-setup ─────────────────────────────────────────────
_export_line="export PATH=\"\$PATH:$BIN_DIR\""
_path_configured=false
for _rc in "$HOME/.zshrc" "$HOME/.bashrc" "$HOME/.bash_profile" "$HOME/.profile"; do
    if [ -f "$_rc" ] && ! grep -qF "$BIN_DIR" "$_rc" 2>/dev/null; then
        { echo ""; echo "# X-Phage"; echo "$_export_line"; } >> "$_rc"
        _path_configured=true; break
    elif [ -f "$_rc" ] && grep -qF "$BIN_DIR" "$_rc" 2>/dev/null; then
        _path_configured=true; break
    fi
done

# ── Done ────────────────────────────────────────────────────────
echo ""
echo -e "${G}  xphage is installed now. Great!${NC}"
echo ""
echo -e "  To get started you may need to restart your current shell."
echo -e "  This would reload your ${C}PATH${NC} environment variable to"
echo -e "  include X-Phage's bin directory (${C}${BIN_DIR}${NC})."
echo ""
if ! [[ ":$PATH:" == *":$BIN_DIR:"* ]]; then
    echo -e "  To configure your current shell, run:"
    echo -e "  ${C}source \$HOME/.bashrc${NC}  ${DIM}or${NC}  ${C}source \$HOME/.zshrc${NC}"
    echo ""
fi
echo -e "  ${DIM}USAGE:${NC}"
echo -e "  ${C}xphage --version${NC}          Print version info and exit"
echo -e "  ${C}xphage init${NC}               Create a new project"
echo -e "  ${C}xphage build${NC}              Build current project"
echo -e "  ${C}xphage run src/main.xp0${NC}   Run directly"
echo ""
echo -e "  ${DIM}PACKAGE MANAGER (XPM):${NC}"
echo -e "  Install XPM: ${C}curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/XPM/main/scripts/install.sh | bash${NC}"
echo ""
echo -e "  ${DIM}https://github.com/${REPO}${NC}"
echo ""

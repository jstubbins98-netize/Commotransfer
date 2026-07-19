#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Commotransfer — OpenCBM setup script for macOS
#
# This script is called by the SetupWizard but can also be run manually:
#
#   bash scripts/setup_opencbm.sh
#
# It will:
#   1. Check for Homebrew
#   2. Install the opencbm formula
#   3. Print paths of installed tools
#
# Requires: macOS, Homebrew (https://brew.sh)
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
die()   { error "$*"; exit 1; }

# ── 1. Verify we are on macOS ─────────────────────────────────────────────────
if [[ "$(uname)" != "Darwin" ]]; then
    die "This script is intended for macOS only."
fi

# ── 2. Locate Homebrew ────────────────────────────────────────────────────────
BREW_CMD=""
for d in /opt/homebrew/bin /usr/local/bin; do
    if [[ -x "$d/brew" ]]; then
        BREW_CMD="$d/brew"
        break
    fi
done
command -v brew &>/dev/null && BREW_CMD="brew"

if [[ -z "$BREW_CMD" ]]; then
    die "Homebrew not found. Install it from https://brew.sh then re-run this script."
fi

info "Found Homebrew at: $BREW_CMD"

# ── 3. Check if opencbm is already installed ──────────────────────────────────
CBMCOPY=""
for d in /opt/homebrew/bin /usr/local/bin /usr/bin; do
    if [[ -x "$d/cbmcopy" ]]; then
        CBMCOPY="$d/cbmcopy"
        break
    fi
done

if [[ -n "$CBMCOPY" ]]; then
    info "OpenCBM is already installed: $CBMCOPY"
    info "Nothing to do."
    exit 0
fi

# ── 4. Install via Homebrew (no sudo required) ────────────────────────────────
info "Installing opencbm via Homebrew…"
echo ""
"$BREW_CMD" install opencbm
echo ""

# ── 5. Confirm installation ───────────────────────────────────────────────────
CBMCOPY=$(command -v cbmcopy 2>/dev/null || echo "")
CBMCTRL=$(command -v cbmctrl 2>/dev/null || echo "")

if [[ -n "$CBMCOPY" && -n "$CBMCTRL" ]]; then
    info "OpenCBM installed successfully!"
    info "  cbmcopy : $CBMCOPY"
    info "  cbmctrl : $CBMCTRL"
else
    warn "OpenCBM may not have installed correctly."
    warn "Try running manually:  brew install opencbm"
    warn "Then restart Commotransfer."
    exit 1
fi

# ── 6. Quick device probe ─────────────────────────────────────────────────────
echo ""
info "Probing IEC bus for connected drives…"
info "(Make sure your ZoomFloppy is plugged in and your drive is powered on)"
echo ""

"$CBMCTRL" detect || warn "No drives detected. You can detect them later from the Commotransfer main window."

echo ""
info "Setup complete. Launch Commotransfer and start transferring!"

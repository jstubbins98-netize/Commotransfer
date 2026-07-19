#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Commotransfer — build script for macOS
#
# Usage:
#   bash build.sh          # auto-detects Qt installation
#   bash build.sh --clean  # wipe build/ and rebuild from scratch
#   bash build.sh --run    # build then launch the app immediately
#
# Supports Qt6 and Qt5 via Homebrew (Apple Silicon and Intel).
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
JOBS=$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

info()    { echo -e "${GREEN}▶${NC}  $*"; }
step()    { echo -e "\n${CYAN}${BOLD}── $* ${NC}"; }
warn()    { echo -e "${YELLOW}⚠${NC}  $*"; }
die()     { echo -e "${RED}✗${NC}  $*" >&2; exit 1; }
success() { echo -e "${GREEN}✓${NC}  $*"; }

# ── Argument parsing ──────────────────────────────────────────────────────────
OPT_CLEAN=false
OPT_RUN=false
for arg in "$@"; do
    case "$arg" in
        --clean) OPT_CLEAN=true ;;
        --run)   OPT_RUN=true ;;
        --help|-h)
            echo "Usage: bash build.sh [--clean] [--run]"
            echo "  --clean  Remove build/ directory before building"
            echo "  --run    Launch Commotransfer.app after a successful build"
            exit 0 ;;
        *) warn "Unknown argument: $arg (ignored)" ;;
    esac
done

# ── Verify platform ───────────────────────────────────────────────────────────
[[ "$(uname)" == "Darwin" ]] || die "This build script is for macOS only."

echo ""
echo -e "${BOLD}Commotransfer — build script${NC}"
echo "────────────────────────────────────────────"

# ── Locate cmake ──────────────────────────────────────────────────────────────
step "Checking for cmake"

CMAKE_CMD=""
for d in /opt/homebrew/bin /usr/local/bin /usr/bin; do
    if [[ -x "$d/cmake" ]]; then CMAKE_CMD="$d/cmake"; break; fi
done
command -v cmake &>/dev/null && CMAKE_CMD="cmake"

if [[ -z "$CMAKE_CMD" ]]; then
    die "cmake not found. Install it with:\n    brew install cmake"
fi
info "cmake: $($CMAKE_CMD --version | head -1)"

# ── Locate Qt ─────────────────────────────────────────────────────────────────
step "Detecting Qt installation"

QT_PREFIX=""

# Preference order: Homebrew Qt6 (Apple Silicon) → Homebrew Qt6 (Intel)
#                   → Homebrew Qt6 explicit → Homebrew Qt5 fallbacks
declare -a QT_CANDIDATES=(
    "/opt/homebrew/opt/qt"       # Homebrew Qt6 Apple Silicon (default)
    "/usr/local/opt/qt"          # Homebrew Qt6 Intel
    "/opt/homebrew/opt/qt@6"     # Homebrew Qt6 explicit Apple Silicon
    "/usr/local/opt/qt@6"        # Homebrew Qt6 explicit Intel
    "/opt/homebrew/opt/qt@5"     # Homebrew Qt5 Apple Silicon
    "/usr/local/opt/qt@5"        # Homebrew Qt5 Intel
)

for candidate in "${QT_CANDIDATES[@]}"; do
    if [[ -d "$candidate" ]]; then
        QT_PREFIX="$candidate"
        break
    fi
done

if [[ -z "$QT_PREFIX" ]]; then
    die "Qt not found. Install it with:\n    brew install qt\nThen re-run this script."
fi
info "Qt prefix: $QT_PREFIX"

# ── Clean ─────────────────────────────────────────────────────────────────────
if [[ "$OPT_CLEAN" == true && -d "$BUILD_DIR" ]]; then
    step "Cleaning build directory"
    rm -rf "$BUILD_DIR"
    success "Removed $BUILD_DIR"
fi

# ── Configure ─────────────────────────────────────────────────────────────────
step "Configuring with CMake"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

"$CMAKE_CMD" "$SCRIPT_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0

# ── Build ─────────────────────────────────────────────────────────────────────
step "Building (using $JOBS parallel jobs)"
"$CMAKE_CMD" --build . --config Release -j "$JOBS"

# ── Locate the output ─────────────────────────────────────────────────────────
step "Locating build output"

APP_PATH=""
if [[ -d "$BUILD_DIR/Commotransfer.app" ]]; then
    APP_PATH="$BUILD_DIR/Commotransfer.app"
elif [[ -f "$BUILD_DIR/Commotransfer" ]]; then
    APP_PATH="$BUILD_DIR/Commotransfer"
fi

echo ""
echo "────────────────────────────────────────────"
if [[ -n "$APP_PATH" ]]; then
    success "Build succeeded!"
    info "Output: $APP_PATH"
    echo ""
    echo -e "  Run with:  ${BOLD}open \"$APP_PATH\"${NC}"
    echo ""
else
    die "Build finished but output not found in $BUILD_DIR. Check cmake output above."
fi

# ── Optional launch ───────────────────────────────────────────────────────────
if [[ "$OPT_RUN" == true ]]; then
    step "Launching Commotransfer"
    if [[ -d "$APP_PATH" ]]; then
        open "$APP_PATH"
    else
        "$APP_PATH"
    fi
fi

#!/bin/bash

set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
CONFIG_FILE="$ROOT_DIR/builder.conf"

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "ERREUR: builder.conf est introuvable dans $ROOT_DIR" >&2
    exit 1
fi

# shellcheck disable=SC1090
source "$CONFIG_FILE"

: "${PRODUCT_NAME:?PRODUCT_NAME absent de builder.conf}"
: "${PRODUCT_SLUG:?PRODUCT_SLUG absent de builder.conf}"
: "${PRODUCT_VERSION:?PRODUCT_VERSION absent de builder.conf}"
: "${SOURCE_COMMIT:?SOURCE_COMMIT absent de builder.conf}"
: "${SOURCE_DIFF_SHA256:?SOURCE_DIFF_SHA256 absent de builder.conf}"

CMAKE_VERSION="3.31.12"
CMAKE_ARCHIVE="cmake-${CMAKE_VERSION}-macos-universal.tar.gz"
CMAKE_ARCHIVE_SHA256="799af7fd545db9bf1b9cfe72f8095880e727a2d4e0df0e3dffc3bc7b95c2d3b0"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${CMAKE_ARCHIVE}"
DEPLOYMENT_TARGET="${UWDEVST_DEPLOYMENT_TARGET:-11.0}"
ARCHITECTURES="${UWDEVST_ARCHITECTURES:-arm64;x86_64}"
RUN_ID="$(date -u '+%Y%m%dT%H%M%SZ')"
OUTPUT_ROOT="$ROOT_DIR/output/$RUN_ID"
LOG_ROOT="$OUTPUT_ROOT/logs"
WORK_ROOT="$ROOT_DIR/.work/$RUN_ID"
BUILD_DIR="$WORK_ROOT/build"

mkdir -p "$LOG_ROOT" "$BUILD_DIR"
exec > >(tee -a "$LOG_ROOT/launcher.log") 2>&1

notify_user() {
    local message="$1"
    /usr/bin/osascript -e "display notification \"${message}\" with title \"${PRODUCT_NAME}\"" >/dev/null 2>&1 || true
}

on_exit() {
    local status=$?
    if [[ $status -eq 0 ]]; then
        notify_user "Build macOS termine avec succes"
    else
        echo
        echo "ECHEC: consultez $LOG_ROOT/launcher.log"
        notify_user "Echec du build macOS - voir le journal"
        /usr/bin/open -R "$LOG_ROOT/launcher.log" >/dev/null 2>&1 || true
    fi
}
trap on_exit EXIT

echo "============================================================"
echo "${PRODUCT_NAME} ${PRODUCT_VERSION} - build macOS Universal"
echo "============================================================"
echo "Dossier du paquet : $ROOT_DIR"
echo "Sortie            : $OUTPUT_ROOT"
echo "Architectures     : $ARCHITECTURES"
echo "Cible minimale    : macOS $DEPLOYMENT_TARGET"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERREUR: ce lanceur doit etre execute sur macOS." >&2
    exit 1
fi

if ! /usr/bin/xcrun --sdk macosx --show-sdk-path >/dev/null 2>&1; then
    echo "Les outils Apple de compilation ne sont pas installes."
    echo "Ouverture de l'installateur officiel Apple..."
    /usr/bin/xcode-select --install >/dev/null 2>&1 || true
    echo "Terminez l'installation Apple, puis relancez BUILD_MAC.command."
    exit 2
fi

for required_tool in /usr/bin/xcrun /usr/bin/codesign /usr/bin/lipo /usr/bin/otool /usr/bin/nm /usr/bin/ditto /usr/bin/shasum /usr/bin/curl /usr/bin/tar /usr/bin/make; do
    if [[ ! -x "$required_tool" ]]; then
        echo "ERREUR: outil Apple requis introuvable: $required_tool" >&2
        exit 1
    fi
done

for required_path in source/CMakeLists.txt JUCE/CMakeLists.txt assets cmake/enable-c.cmake PACKAGE_SHA256SUMS; do
    if [[ ! -e "$ROOT_DIR/$required_path" ]]; then
        echo "ERREUR: element du paquet introuvable: $required_path" >&2
        exit 1
    fi
done

echo
echo "Verification de l'integrite du paquet..."
(
    cd "$ROOT_DIR"
    /usr/bin/shasum -a 256 -c PACKAGE_SHA256SUMS
)

cmake_is_usable() {
    local candidate="$1"
    local version major remainder minor

    [[ -x "$candidate" ]] || return 1
    version="$($candidate --version 2>/dev/null | /usr/bin/awk 'NR == 1 { print $3 }')"
    major="${version%%.*}"
    remainder="${version#*.}"
    minor="${remainder%%.*}"
    [[ "$major" =~ ^[0-9]+$ && "$minor" =~ ^[0-9]+$ ]] || return 1
    (( major > 3 || (major == 3 && minor >= 22) ))
}

resolve_cmake() {
    local system_cmake cache_root cached_cmake temp_dir downloaded actual_hash

    system_cmake="$(command -v cmake 2>/dev/null || true)"
    if [[ -n "$system_cmake" ]] && cmake_is_usable "$system_cmake"; then
        printf '%s\n' "$system_cmake"
        return 0
    fi

    cache_root="$HOME/Library/Caches/UWdeVST-macOS-builder"
    cached_cmake="$cache_root/cmake-${CMAKE_VERSION}-macos-universal/CMake.app/Contents/bin/cmake"
    if cmake_is_usable "$cached_cmake"; then
        printf '%s\n' "$cached_cmake"
        return 0
    fi

    mkdir -p "$cache_root"
    temp_dir="$(/usr/bin/mktemp -d "$cache_root/cmake-download.XXXXXX")"
    downloaded="$temp_dir/$CMAKE_ARCHIVE"

    echo "CMake n'est pas installe : telechargement portable automatique ${CMAKE_VERSION}..." >&2
    /usr/bin/curl --fail --location --retry 3 --connect-timeout 20 "$CMAKE_URL" --output "$downloaded"
    actual_hash="$(/usr/bin/shasum -a 256 "$downloaded" | /usr/bin/awk '{ print $1 }')"
    if [[ "$actual_hash" != "$CMAKE_ARCHIVE_SHA256" ]]; then
        echo "ERREUR: empreinte CMake invalide." >&2
        echo "Attendue: $CMAKE_ARCHIVE_SHA256" >&2
        echo "Recue   : $actual_hash" >&2
        exit 1
    fi

    /usr/bin/tar -xzf "$downloaded" -C "$temp_dir"
    if [[ ! -x "$temp_dir/cmake-${CMAKE_VERSION}-macos-universal/CMake.app/Contents/bin/cmake" ]]; then
        echo "ERREUR: archive CMake incomplete." >&2
        exit 1
    fi

    if [[ -e "$cache_root/cmake-${CMAKE_VERSION}-macos-universal" ]]; then
        case "$cache_root/cmake-${CMAKE_VERSION}-macos-universal" in
            "$HOME/Library/Caches/UWdeVST-macOS-builder/"*)
                /bin/rm -rf -- "$cache_root/cmake-${CMAKE_VERSION}-macos-universal"
                ;;
            *)
                echo "ERREUR: chemin de cache CMake inattendu." >&2
                exit 1
                ;;
        esac
    fi

    /bin/mv "$temp_dir/cmake-${CMAKE_VERSION}-macos-universal" "$cache_root/"
    /bin/rm -rf -- "$temp_dir"
    printf '%s\n' "$cached_cmake"
}

CMAKE_BIN="$(resolve_cmake)"
echo "CMake            : $CMAKE_BIN"
"$CMAKE_BIN" --version | /usr/bin/head -n 1

CPU_COUNT="$(/usr/sbin/sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"
if [[ ! "$CPU_COUNT" =~ ^[0-9]+$ ]] || (( CPU_COUNT < 1 )); then
    CPU_COUNT=4
fi
if (( CPU_COUNT > 8 )); then
    CPU_COUNT=8
fi
BUILD_JOBS="${UWDEVST_JOBS:-$CPU_COUNT}"

echo "Jobs              : $BUILD_JOBS"
echo "SDK               : $(/usr/bin/xcrun --sdk macosx --show-sdk-path)"
echo
echo "Configuration CMake..."

"$CMAKE_BIN" \
    -S "$ROOT_DIR/source" \
    -B "$BUILD_DIR" \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    "-DCMAKE_OSX_ARCHITECTURES=$ARCHITECTURES" \
    "-DCMAKE_OSX_DEPLOYMENT_TARGET=$DEPLOYMENT_TARGET" \
    "-DCMAKE_PROJECT_INCLUDE=$ROOT_DIR/cmake/enable-c.cmake" \
    "-DUWDEVST_JUCE_DIR=$ROOT_DIR/JUCE" \
    "-DUWDEVST_SHARED_ASSETS_DIR=$ROOT_DIR/assets" \
    2>&1 | tee "$LOG_ROOT/configure.log"

echo
echo "Compilation Release..."
"$CMAKE_BIN" --build "$BUILD_DIR" --config Release --parallel "$BUILD_JOBS" 2>&1 | tee "$LOG_ROOT/build.log"

STANDALONE_BUNDLE="$(/usr/bin/find "$BUILD_DIR" -type d -path "*/Release/Standalone/${PRODUCT_NAME}.app" -print -quit)"
VST3_BUNDLE="$(/usr/bin/find "$BUILD_DIR" -type d -path "*/Release/VST3/${PRODUCT_NAME}.vst3" -print -quit)"

if [[ -z "$STANDALONE_BUNDLE" || -z "$VST3_BUNDLE" ]]; then
    echo "ERREUR: artefacts ${PRODUCT_NAME} Standalone/VST3 introuvables." >&2
    /usr/bin/find "$BUILD_DIR" -path '*/Release/*' -maxdepth 8 -print || true
    exit 1
fi

STANDALONE_BINARY="$STANDALONE_BUNDLE/Contents/MacOS/$PRODUCT_NAME"
VST3_BINARY="$VST3_BUNDLE/Contents/MacOS/$PRODUCT_NAME"

if [[ ! -f "$STANDALONE_BINARY" || ! -f "$VST3_BINARY" ]]; then
    echo "ERREUR: executables Mach-O introuvables dans les bundles." >&2
    exit 1
fi

echo
echo "Signature locale ad hoc..."
/usr/bin/codesign --force --deep --sign - --timestamp=none "$STANDALONE_BUNDLE"
/usr/bin/codesign --force --deep --sign - --timestamp=none "$VST3_BUNDLE"

check_architectures() {
    local binary="$1"
    local found_arches expected_arch
    found_arches="$(/usr/bin/lipo -archs "$binary")"
    for expected_arch in ${ARCHITECTURES//;/ }; do
        if [[ " $found_arches " != *" $expected_arch "* ]]; then
            echo "ERREUR: architecture $expected_arch absente de $binary (trouve: $found_arches)." >&2
            return 1
        fi
    done
    printf '%s\n' "$found_arches"
}

echo
echo "Validation Mach-O, architectures et signature..."
{
    echo "Standalone=$STANDALONE_BINARY"
    /usr/bin/file "$STANDALONE_BINARY"
    echo "StandaloneArchitectures=$(check_architectures "$STANDALONE_BINARY")"
    /usr/bin/otool -L "$STANDALONE_BINARY"
    /usr/bin/codesign --verify --deep --strict --verbose=2 "$STANDALONE_BUNDLE"
    echo
    echo "VST3=$VST3_BINARY"
    /usr/bin/file "$VST3_BINARY"
    echo "VST3Architectures=$(check_architectures "$VST3_BINARY")"
    /usr/bin/otool -L "$VST3_BINARY"
    /usr/bin/codesign --verify --deep --strict --verbose=2 "$VST3_BUNDLE"
    if ! /usr/bin/nm -gU "$VST3_BINARY" | /usr/bin/grep -q 'GetPluginFactory'; then
        echo "ERREUR: point d'entree VST3 GetPluginFactory absent." >&2
        exit 1
    fi
    echo "VST3EntryPoint=GetPluginFactory"
} 2>&1 | tee "$LOG_ROOT/validation.log"

echo
echo "Nouvelle verification de l'integrite des sources..."
(
    cd "$ROOT_DIR"
    /usr/bin/shasum -a 256 -c PACKAGE_SHA256SUMS >/dev/null
)

DELIVERY_NAME="${PRODUCT_NAME}_${PRODUCT_VERSION}_macOS_universal"
DELIVERY_DIR="$OUTPUT_ROOT/$DELIVERY_NAME"
ARCHIVE_PATH="$OUTPUT_ROOT/${DELIVERY_NAME}.zip"
mkdir -p "$DELIVERY_DIR/Standalone" "$DELIVERY_DIR/VST3"
/usr/bin/ditto "$STANDALONE_BUNDLE" "$DELIVERY_DIR/Standalone/${PRODUCT_NAME}.app"
/usr/bin/ditto "$VST3_BUNDLE" "$DELIVERY_DIR/VST3/${PRODUCT_NAME}.vst3"

{
    echo "ProductName=$PRODUCT_NAME"
    echo "ProductVersion=$PRODUCT_VERSION"
    echo "SourceCommit=$SOURCE_COMMIT"
    echo "SourceState=$SOURCE_STATE"
    echo "SourceDiffSHA256=$SOURCE_DIFF_SHA256"
    echo "JUCECommit=$JUCE_COMMIT"
    echo "BuildUTC=$RUN_ID"
    echo "macOS=$(/usr/bin/sw_vers -productVersion)"
    echo "Architecture=$(uname -m)"
    echo "Compiler=$(/usr/bin/xcrun clang --version | /usr/bin/head -n 1)"
    echo "CMake=$($CMAKE_BIN --version | /usr/bin/head -n 1)"
    echo "SDK=$(/usr/bin/xcrun --sdk macosx --show-sdk-path)"
    echo "DeploymentTarget=$DEPLOYMENT_TARGET"
    echo "RequestedArchitectures=$ARCHITECTURES"
    echo "StandaloneArchitectures=$(/usr/bin/lipo -archs "$STANDALONE_BINARY")"
    echo "VST3Architectures=$(/usr/bin/lipo -archs "$VST3_BINARY")"
    echo "Signing=ad-hoc local (non notarized)"
} > "$DELIVERY_DIR/BUILD_RECEIPT.txt"

/usr/bin/ditto -c -k --sequesterRsrc --keepParent "$DELIVERY_DIR" "$ARCHIVE_PATH"
(
    cd "$OUTPUT_ROOT"
    /usr/bin/shasum -a 256 "$(basename "$ARCHIVE_PATH")" > SHA256SUMS
)

if [[ "${UWDEVST_KEEP_BUILD:-0}" != "1" ]]; then
    case "$WORK_ROOT" in
        "$ROOT_DIR/.work/"*) /bin/rm -rf -- "$WORK_ROOT" ;;
        *) echo "ERREUR: refus de nettoyer un chemin de travail inattendu: $WORK_ROOT" >&2; exit 1 ;;
    esac
fi

echo
echo "SUCCES"
echo "Archive : $ARCHIVE_PATH"
echo "SHA-256 : $(/usr/bin/cat "$OUTPUT_ROOT/SHA256SUMS")"
/usr/bin/open "$OUTPUT_ROOT" >/dev/null 2>&1 || true


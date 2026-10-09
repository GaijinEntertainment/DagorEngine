#!/bin/bash
# Qt runtime deployment script for Linux and macOS

set -e

SCRIPT_NAME="$(basename "$0")"
ROOT_HINT="$(cd "$(dirname "$0")/../.." && pwd)"
ROOT_DIR="$ROOT_HINT"

QT_DIR="$1"
if [ -z "$QT_DIR" ] && [ -n "$QtDir" ]; then
    QT_DIR="$QtDir"
fi

if [ -z "$QT_DIR" ]; then
    echo "Usage: $SCRIPT_NAME [QtDir] [TargetExe] [--debug|--release]"
    echo ""
    echo "  QtDir      - path to the Qt installation containing the lib/ or Frameworks/ folder."
    echo "  TargetExe  - optional path to the built executable. Defaults to the latest"
    echo "               QtWidgetsSample binary under \"$ROOT_DIR/_output\"."
    echo "  --debug    - force deployment of debug Qt libraries."
    echo "  --release  - force deployment of release Qt libraries."
    exit 1
fi

TARGET_EXE="$2"
if [ -z "$TARGET_EXE" ]; then
    # Find the latest QtWidgetsSample executable
    TARGET_EXE=$(find "$ROOT_DIR/_output" -name "QtWidgetsSample*" -type f -executable 2>/dev/null | sort | tail -n 1)
fi

if [ -z "$TARGET_EXE" ] || [ ! -f "$TARGET_EXE" ]; then
    echo "$SCRIPT_NAME: could not locate a QtWidgetsSample executable. Build the target first or pass the path explicitly."
    exit 1
fi

TARGET_DIR="$(dirname "$TARGET_EXE")"
TARGET_NAME="$(basename "$TARGET_EXE")"

DEPLOY_MODE="release"
if [ "$3" = "--debug" ]; then
    DEPLOY_MODE="debug"
elif [ "$3" = "--release" ]; then
    DEPLOY_MODE="release"
elif echo "$TARGET_NAME" | grep -qi "dbg"; then
    DEPLOY_MODE="debug"
fi

echo "Deploying Qt runtime for $TARGET_EXE"
echo "  Qt root : $QT_DIR"
echo "  Output  : $TARGET_DIR"
echo "  Mode    : $DEPLOY_MODE"

MODE_SUFFIX=""
if [ "$DEPLOY_MODE" = "debug" ]; then
    MODE_SUFFIX="_debug"
fi

# Detect platform
UNAME_S="$(uname -s)"
case "$UNAME_S" in
    Darwin*)
        PLATFORM="macos"
        ;;
    Linux*)
        PLATFORM="linux"
        ;;
    *)
        echo "$SCRIPT_NAME: unsupported platform: $UNAME_S"
        exit 1
        ;;
esac

copy_file() {
    local src="$1"
    local dest="$2"
    local required="$3"
    
    if [ ! -f "$src" ]; then
        if [ "$required" = "required" ]; then
            echo "$SCRIPT_NAME: required file not found: $src"
            exit 2
        else
            echo "$SCRIPT_NAME: warning - optional file not found: $src"
            return 0
        fi
    fi
    
    mkdir -p "$(dirname "$dest")"
    cp -f "$src" "$dest"
    if [ $? -ne 0 ]; then
        echo "$SCRIPT_NAME: failed to copy $(basename "$src") to $dest"
        exit 3
    else
        echo "  copied $(basename "$src")"
    fi
}

if [ "$PLATFORM" = "macos" ]; then
    # macOS: Copy frameworks
    QT_FRAMEWORKS="$QT_DIR/lib"
    
    if [ ! -d "$QT_FRAMEWORKS/QtCore.framework" ]; then
        echo "$SCRIPT_NAME: Qt frameworks directory does not look valid: $QT_FRAMEWORKS"
        exit 1
    fi
    
    FRAMEWORKS_LIST="QtCore QtGui QtWidgets"
    
    for fw in $FRAMEWORKS_LIST; do
        FW_SRC="$QT_FRAMEWORKS/${fw}.framework"
        FW_DEST="$TARGET_DIR/../Frameworks/${fw}.framework"
        
        if [ -d "$FW_SRC" ]; then
            echo "  copying framework $fw..."
            mkdir -p "$FW_DEST/Versions/5"
            
            # Copy the main library
            if [ "$DEPLOY_MODE" = "debug" ] && [ -f "$FW_SRC/Versions/5/${fw}_debug" ]; then
                copy_file "$FW_SRC/Versions/5/${fw}_debug" "$FW_DEST/Versions/5/$fw" required
            else
                copy_file "$FW_SRC/Versions/5/$fw" "$FW_DEST/Versions/5/$fw" required
            fi
            
            # Copy Resources
            if [ -d "$FW_SRC/Versions/5/Resources" ]; then
                cp -R "$FW_SRC/Versions/5/Resources" "$FW_DEST/Versions/5/"
            fi
            
            # Create symlinks
            (cd "$FW_DEST" && ln -sf "Versions/5/$fw" "$fw" 2>/dev/null)
            (cd "$FW_DEST" && ln -sf "Versions/5/Resources" "Resources" 2>/dev/null)
        fi
    done
    
    # Copy plugins
    PLUGINS_DIR="$QT_DIR/plugins"
    if [ -d "$PLUGINS_DIR/platforms" ]; then
        mkdir -p "$TARGET_DIR/../PlugIns/platforms"
        copy_file "$PLUGINS_DIR/platforms/libqcocoa${MODE_SUFFIX}.dylib" "$TARGET_DIR/../PlugIns/platforms/libqcocoa.dylib" required
    fi
    
    if [ -d "$PLUGINS_DIR/styles" ]; then
        mkdir -p "$TARGET_DIR/../PlugIns/styles"
        copy_file "$PLUGINS_DIR/styles/libqmacstyle${MODE_SUFFIX}.dylib" "$TARGET_DIR/../PlugIns/styles/libqmacstyle.dylib" optional
    fi
    
elif [ "$PLATFORM" = "linux" ]; then
    # Linux: Copy shared libraries
    QT_LIB="$QT_DIR/lib"
    
    if [ ! -f "$QT_LIB/libQt5Core.so" ] && [ ! -f "$QT_LIB/libQt5Core.so.5" ]; then
        echo "$SCRIPT_NAME: Qt lib directory does not look valid: $QT_LIB"
        exit 1
    fi
    
    SO_LIST="libQt5Core libQt5Gui libQt5Widgets libQt5DBus libQt5XcbQpa"
    
    for lib in $SO_LIST; do
        # Find the actual .so file (could be .so.5.x.y)
        SO_FILE=$(find "$QT_LIB" -name "${lib}.so*" -type f 2>/dev/null | head -n 1)
        if [ -n "$SO_FILE" ]; then
            copy_file "$SO_FILE" "$TARGET_DIR/$(basename "$SO_FILE")" required
            
            # Create symlink if it's a versioned library
            SO_BASE="$(basename "$SO_FILE" | sed 's/\.so\..*/\.so/')"
            if [ "$SO_BASE" != "$(basename "$SO_FILE")" ]; then
                (cd "$TARGET_DIR" && ln -sf "$(basename "$SO_FILE")" "$SO_BASE" 2>/dev/null)
            fi
        fi
    done
    
    # Copy ICU libraries if present
    ICU_LIBS=$(find "$QT_LIB" -name "libicu*.so*" -type f 2>/dev/null)
    for icu in $ICU_LIBS; do
        copy_file "$icu" "$TARGET_DIR/$(basename "$icu")" optional
    done
    
    # Copy plugins
    PLUGINS_DIR="$QT_DIR/plugins"
    if [ -d "$PLUGINS_DIR/platforms" ]; then
        mkdir -p "$TARGET_DIR/platforms"
        copy_file "$PLUGINS_DIR/platforms/libqxcb.so" "$TARGET_DIR/platforms/libqxcb.so" required
    fi
    
    if [ -d "$PLUGINS_DIR/platformthemes" ]; then
        mkdir -p "$TARGET_DIR/platformthemes"
        for theme in "$PLUGINS_DIR/platformthemes"/*.so; do
            if [ -f "$theme" ]; then
                copy_file "$theme" "$TARGET_DIR/platformthemes/$(basename "$theme")" optional
            fi
        done
    fi
    
    if [ -d "$PLUGINS_DIR/xcbglintegrations" ]; then
        mkdir -p "$TARGET_DIR/xcbglintegrations"
        for xcb in "$PLUGINS_DIR/xcbglintegrations"/*.so; do
            if [ -f "$xcb" ]; then
                copy_file "$xcb" "$TARGET_DIR/xcbglintegrations/$(basename "$xcb")" optional
            fi
        done
    fi
fi

echo "$SCRIPT_NAME: deployment completed."
exit 0

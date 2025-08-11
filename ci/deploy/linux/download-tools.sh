#!/bin/bash
set -e

# Download linuxdeploy and plugins for AppImage creation

TOOLS_DIR="$PWD/ci/deploy/linux/tools"
mkdir -p $TOOLS_DIR

echo "Downloading linuxdeploy tools..."

# Download linuxdeploy
if [ ! -f "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" ]; then
    wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
         -O "$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
fi

# Download linuxdeploy Qt plugin
if [ ! -f "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
         -O "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

# Download appimagetool
if [ ! -f "$TOOLS_DIR/appimagetool-x86_64.AppImage" ]; then
    wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" \
         -O "$TOOLS_DIR/appimagetool-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/appimagetool-x86_64.AppImage"
fi

echo "Tools downloaded successfully"

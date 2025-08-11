#!/bin/bash
set -e

# Build AppImage for QML Remote Server
# This script downloads official Qt, builds the project and creates an AppImage

QT_VERSION="6.7.2"
QT_DIR="$HOME/Qt"
BUILD_DIR="build"
APPDIR="AppDir"

echo "=== Building QML Remote Server AppImage ==="

# Download and install Qt if not present
if [ ! -d "$QT_DIR/$QT_VERSION/gcc_64" ]; then
    echo "Downloading Qt $QT_VERSION..."
    python3 -m pip install --user aqtinstall
    python3 -m aqt install-qt linux desktop $QT_VERSION gcc_64 -O $QT_DIR
fi

# Set Qt environment
export PATH="$QT_DIR/$QT_VERSION/gcc_64/bin:$PATH"
export LD_LIBRARY_PATH="$QT_DIR/$QT_VERSION/gcc_64/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$QT_DIR/$QT_VERSION/gcc_64/plugins"

echo "Qt version: $(qmake -query QT_VERSION)"

# Clean and create build directory
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$QT_DIR/$QT_VERSION/gcc_64" \
      -DCMAKE_INSTALL_PREFIX=/usr \
      ..

# Build
echo "Building..."
make -j$(nproc)

# Create AppDir structure
echo "Creating AppDir structure..."
cd ..
rm -rf $APPDIR
mkdir -p $APPDIR

# Install to AppDir
DESTDIR=$PWD/$APPDIR make -C $BUILD_DIR install

# Create AppDir structure manually if needed
mkdir -p $APPDIR/usr/bin
mkdir -p $APPDIR/usr/lib
mkdir -p $APPDIR/usr/share/applications
mkdir -p $APPDIR/usr/share/icons/hicolor/256x256/apps

# Copy executable
cp $BUILD_DIR/appqml-remoteserver $APPDIR/usr/bin/

# Copy examples
mkdir -p $APPDIR/usr/share/qml-remoteserver
cp -r examples $APPDIR/usr/share/qml-remoteserver/

# Deploy Qt dependencies
echo "Deploying Qt dependencies..."
$QT_DIR/$QT_VERSION/gcc_64/bin/qmlimportscanner \
    -rootPath . \
    -importPath $QT_DIR/$QT_VERSION/gcc_64/qml \
    > qml-imports.json || true

# Use linuxdeploy with Qt plugin
echo "Running linuxdeploy..."

# Download tools if needed
./ci/deploy/linux/download-tools.sh

TOOLS_DIR="$PWD/ci/deploy/linux/tools"

# Set environment for Qt plugin
export QML_SOURCES_PATHS="$PWD"
export QMAKE="$QT_DIR/$QT_VERSION/gcc_64/bin/qmake"

# Copy desktop file and icon
cp ci/deploy/linux/qml-remoteserver.desktop $APPDIR/usr/share/applications/
# Create a simple icon if we don't have one
convert -size 256x256 xc:lightblue -pointsize 40 -fill black -gravity center \
    -annotate +0+0 "QML\nRemote\nServer" \
    $APPDIR/usr/share/icons/hicolor/256x256/apps/qml-remoteserver.png || \
    echo "ImageMagick not available, skipping icon creation"

# Run linuxdeploy with Qt plugin
$TOOLS_DIR/linuxdeploy-x86_64.AppImage \
    --appdir $APPDIR \
    --plugin qt \
    --output appimage \
    --desktop-file ci/deploy/linux/qml-remoteserver.desktop

echo "AppImage created successfully!"
ls -la *.AppImage

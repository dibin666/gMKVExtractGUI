#!/usr/bin/env bash
set -euo pipefail

VERSION="${VERSION:-2.15.0}"
BUILD_DIR="${BUILD_DIR:-/tmp/gmkvextractgui-build}"
DIST_DIR="${DIST_DIR:-/tmp/gmkvextractgui-dist}"

rm -rf "$BUILD_DIR" "$DIST_DIR"
mkdir -p "$DIST_DIR"

cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DGMKV_PACKAGE_VERSION="$VERSION"
cmake --build "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" --output-on-failure

desktop-file-validate src/gMKVExtractGUI.Cpp/resources/linux/io.github.dibin666.gmkvextractgui.desktop
appstreamcli validate --no-net "$BUILD_DIR/src/gMKVExtractGUI.Cpp/io.github.dibin666.gmkvextractgui.metainfo.xml"

cpack --config "$BUILD_DIR/CPackConfig.cmake" -G DEB -B "$DIST_DIR/deb"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -G RPM -B "$DIST_DIR/rpm"

dpkg-deb -I "$DIST_DIR"/deb/*.deb
rpm -qip "$DIST_DIR"/rpm/*.rpm
rpm -qRp "$DIST_DIR"/rpm/*.rpm

echo "Linux package validation completed in Docker."

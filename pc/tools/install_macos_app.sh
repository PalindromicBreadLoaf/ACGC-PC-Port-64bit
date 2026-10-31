#!/bin/zsh
# install_macos_app.sh - Package pc/build-macos into "Animal Crossing.app"
#
# Usage: pc/tools/install_macos_app.sh [install-dir]   (default: /Applications)
#
# Player data lives outside the app, in ~/Library/Application Support/Animal Crossing,
# so reinstalling never touches saves or settings. Files from the build folder are only
# copied there when they don't already exist.

set -euo pipefail

SCRIPT_DIR="${0:A:h}"
BIN_DIR="$SCRIPT_DIR/../build-macos/bin"
RES_DIR="$SCRIPT_DIR/../res"
INSTALL_DIR="${1:-/Applications}"
DATA_DIR="$HOME/Library/Application Support/Animal Crossing"
APP="$INSTALL_DIR/Animal Crossing.app"

GCC_LIB="$(brew --prefix gcc)/lib/gcc/current"
SDL2_LIB="$(brew --prefix sdl2)/lib/libSDL2-2.0.0.dylib"
SDL3_LIB="$(brew --prefix sdl3)/lib/libSDL3.0.dylib"

[ -x "$BIN_DIR/AnimalCrossing" ] || { echo "Build the game first: $BIN_DIR/AnimalCrossing not found"; exit 1; }

STAGE="$(mktemp -d)/Animal Crossing.app"
trap 'rm -rf "${STAGE:h}"' EXIT
MACOS="$STAGE/Contents/MacOS"
FRAMEWORKS="$STAGE/Contents/Frameworks"
RESOURCES="$STAGE/Contents/Resources"
mkdir -p "$MACOS" "$FRAMEWORKS" "$RESOURCES"

cp "$BIN_DIR/AnimalCrossing" "$MACOS/AnimalCrossing"
cp -R "$BIN_DIR/shaders" "$RESOURCES/shaders"

# Bundle the Homebrew libraries so the app survives brew upgrades. sdl2-compat finds
# SDL3 by the name libSDL3.dylib next to itself.
cp -L "$SDL2_LIB" "$FRAMEWORKS/libSDL2-2.0.0.dylib"
cp -L "$SDL3_LIB" "$FRAMEWORKS/libSDL3.dylib"
cp -L "$GCC_LIB/libstdc++.6.dylib" "$FRAMEWORKS/libstdc++.6.dylib"
cp -L "$GCC_LIB/libgcc_s.1.1.dylib" "$FRAMEWORKS/libgcc_s.1.1.dylib"
chmod u+w "$FRAMEWORKS"/*.dylib

for lib in "$FRAMEWORKS"/*.dylib; do
    install_name_tool -id "@rpath/${lib:t}" "$lib" 2>/dev/null
done
install_name_tool -change "@rpath/libgcc_s.1.1.dylib" "@loader_path/libgcc_s.1.1.dylib" \
    "$FRAMEWORKS/libstdc++.6.dylib" 2>/dev/null
install_name_tool \
    -change "$(otool -L "$MACOS/AnimalCrossing" | awk '/libSDL2-2.0.0/ {print $1}')" \
            "@executable_path/../Frameworks/libSDL2-2.0.0.dylib" \
    -change "$(otool -L "$MACOS/AnimalCrossing" | awk '/libstdc\+\+/ {print $1}')" \
            "@executable_path/../Frameworks/libstdc++.6.dylib" \
    "$MACOS/AnimalCrossing" 2>/dev/null

# The game uses paths relative to the current folder, which is / when opened from Finder.
cat > "$MACOS/Animal Crossing" <<'LAUNCHER'
#!/bin/zsh
HERE="${0:A:h}"
DATA="$HOME/Library/Application Support/Animal Crossing"
mkdir -p "$DATA/rom" "$DATA/save/card_a" "$DATA/save/card_b" "$DATA/texture_pack"
ln -sfn "$HERE/../Resources/shaders" "$DATA/shaders"
cd "$DATA" || exit 1
exec "$HERE/AnimalCrossing" "$@" > "$DATA/last-run.log" 2>&1
LAUNCHER
chmod +x "$MACOS/Animal Crossing"

cat > "$STAGE/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDisplayName</key><string>Animal Crossing</string>
    <key>CFBundleName</key><string>Animal Crossing</string>
    <key>CFBundleExecutable</key><string>Animal Crossing</string>
    <key>CFBundleIdentifier</key><string>local.acgc-pc-port.animalcrossing</string>
    <key>CFBundleIconFile</key><string>icon</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>1.0</string>
    <key>CFBundleVersion</key><string>1</string>
    <key>LSApplicationCategoryType</key><string>public.app-category.games</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST

ICONSET="${STAGE:h}/icon.iconset"
mkdir -p "$ICONSET"
for size in 16 32 128 256; do
    sips -z $size $size "$RES_DIR/icon.png" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
    sips -z $((size * 2)) $((size * 2)) "$RES_DIR/icon.png" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$RESOURCES/icon.icns"

# Changing install names invalidates signatures, and arm64 requires a valid one.
for lib in "$FRAMEWORKS"/*.dylib; do
    codesign --force --sign - "$lib" 2>/dev/null
done
codesign --force --sign - "$MACOS/AnimalCrossing" 2>/dev/null
codesign --force --sign - "$STAGE" 2>/dev/null

# -n never overwrites, so an existing save or setting always wins.
mkdir -p "$DATA_DIR"
cp -Rn "$BIN_DIR/rom" "$BIN_DIR/save" "$BIN_DIR/texture_pack" "$DATA_DIR/" 2>/dev/null || true
for file in settings.ini keybindings.ini shader_cache.bin; do
    [ -e "$BIN_DIR/$file" ] && cp -n "$BIN_DIR/$file" "$DATA_DIR/" 2>/dev/null || true
done

rm -rf "$APP"
mv "$STAGE" "$APP"

echo "Installed: $APP"
echo "Data:      $DATA_DIR"

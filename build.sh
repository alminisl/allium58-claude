#!/bin/bash
# Builds the firmware into ~/allium58/allium58_claude.uf2.
# Clones vial-qmk into ~/allium58/vial-qmk on first run, pins it to the tested
# commit, applies firmware/vial-qmk.patch and links the keyboard + keymap in.
# VIAL_BUILD_ID is pinned so flashing keeps your Vial layout. Bump it (and
# reload your .vil once) if you change layer count, tap dance or macro sizes.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
Q="$HERE/vial-qmk"
export PATH="$HOME/Library/Application Support/qmk/bin:$PATH"
export VIAL_BUILD_ID=0xA11158
if [ ! -d "$Q" ]; then
  git clone https://github.com/vial-kb/vial-qmk.git "$Q"
  git -C "$Q" checkout "$(cat "$HERE/firmware/VIAL_QMK_COMMIT")"
  git -C "$Q" submodule update --init --recursive
fi
git -C "$Q" apply --check "$HERE/firmware/vial-qmk.patch" 2>/dev/null && git -C "$Q" apply "$HERE/firmware/vial-qmk.patch"
KB="$Q/keyboards/beekeeb/allium58"
mkdir -p "$KB/keymaps"
ln -sf "$HERE/firmware/keyboard.json" "$KB/keyboard.json"
ln -sfn "$HERE/firmware/claude" "$KB/keymaps/claude"
cd "$Q"
make beekeeb/allium58:claude SKIP_VERSION=yes -j8 2>&1 | grep -E "error|warning:|Creating UF2" | grep -v lto-wrapper || true
cp beekeeb_allium58_claude.uf2 "$HERE/allium58_claude.uf2"
grep -h "BUILD_ID" .build/obj_beekeeb_allium58_claude/src/version.h

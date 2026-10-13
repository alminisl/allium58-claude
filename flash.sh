#!/bin/bash
# Flashes ~/allium58/allium58_claude.uf2 onto the left half without the reset
# button: the firmware jumps to the bootloader on raw HID 0xCF "BOOT".
set -e
H="$HOME/allium58/host"
"$H/bridge" pause >/dev/null
"$H/.venv/bin/python" -I -c '
import hid
i = [x for x in hid.enumerate(0xBEEB, 4) if x["usage_page"] == 0xFF60][0]
d = hid.device(); d.open_path(i["path"])
try: d.write(b"\x00\xcfBOOT" + bytes(27))
except OSError: pass
'
for _ in $(seq 30); do [ -f /Volumes/RPI-RP2/INFO_UF2.TXT ] && break; sleep 1; done
[ -f /Volumes/RPI-RP2/INFO_UF2.TXT ] || { echo "no bootloader drive: double-tap reset and copy the .uf2 by hand"; exit 1; }
sleep 1; cp "$HOME/allium58/allium58_claude.uf2" /Volumes/RPI-RP2/
for _ in $(seq 30); do ioreg -p IOUSB -w0 | grep -q "Allium58@" && break; sleep 1; done
sleep 2; "$H/bridge" resume >/dev/null; echo "flashed, bridge resumed"

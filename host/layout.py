#!/usr/bin/env python3
"""Reads the keyboard's whole dynamic keymap over VIA and compares/saves it.
  layout.py check   compare with layout_fingerprint.json
  layout.py save    store the current keymap as the reference"""
import hashlib, json, os, sys
import hid

REF = os.path.expanduser("~/allium58/host/layout_fingerprint.json")


def read_keymap():
    info = [i for i in hid.enumerate(0xBEEB, 4) if i["usage_page"] == 0xFF60][0]
    d = hid.device()
    d.open_path(info["path"])

    def q(*b):
        d.write(b"\x00" + bytes(b) + bytes(32 - len(b)))
        for _ in range(30):
            r = d.read(32, 100)
            if r and r[0] == b[0]:
                return r
        raise SystemExit("keyboard did not answer")

    size = q(0x11)[1] * 120
    buf = bytearray()
    for off in range(0, size, 28):
        n = min(28, size - off)
        buf += bytes(q(0x12, off >> 8, off & 0xFF, n)[4:4 + n])
    d.close()
    return buf


buf = read_keymap()
sha = hashlib.sha256(buf).hexdigest()[:16]
if sys.argv[1:] == ["save"]:
    json.dump({"sha": sha, "codes": list(buf)}, open(REF, "w"))
    print("saved", sha)
else:
    ref = json.load(open(REF))["sha"]
    print(sha, "MATCHES saved layout" if sha == ref else f"DIFFERS from saved {ref}")

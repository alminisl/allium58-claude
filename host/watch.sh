#!/bin/bash
# Watches the Allium58's USB health and writes one timeline to watch.log.
#   tail -f ~/allium58/host/watch.log
#
#   PROBE  every 10 s: bridge running? which port is the keyboard on?
#   USB    macOS kernel/IOKit lines about the keyboard: attach, detach,
#          reset, suspend, resume
#   BRIDGE the bridge's own log (keyboard connected / lost)

HOST="$HOME/allium58/host"
LOG="$HOST/watch.log"
PY="$HOST/.venv/bin/python"

stamp() { while IFS= read -r line; do printf '%s %s\n' "$(date +%T)" "$line"; done; }

echo "$(date '+%F %T') watcher started" >> "$LOG"

# macOS USB events for this device.
log stream --style compact --predicate \
  'eventMessage CONTAINS[c] "Allium58" OR eventMessage CONTAINS[c] "0xbeeb" OR (subsystem == "com.apple.iokit.IOUSBHost" AND eventMessage CONTAINS[c] "suspend")' 2>/dev/null \
  | grep --line-buffered -viE "^Filtering|^Timestamp" \
  | sed -u 's/^[0-9-]* [0-9:.]* *[A-Za-z]* *[^ ]* *//' | cut -c1-160 \
  | sed -u 's/^/USB    /' | stamp >> "$LOG" &

# The bridge's connect/lose messages.
tail -n 0 -F "$HOST/agentd.log" 2>/dev/null | grep --line-buffered -E "keyboard|listening|health|Vial|WATCHDOG|stack peak" \
  | sed -u 's/^[0-9:]* /BRIDGE /' | stamp >> "$LOG" &

trap 'kill $(jobs -p) 2>/dev/null; echo "$(date +%T) watcher stopped" >> "$LOG"' EXIT

last=""
beat=0
while true; do
  # The bridge holds the keyboard open and runs the health probe itself
  # (its "health ..." lines land here via BRIDGE); this loop tracks the port.
  state=$(pgrep -f agentd.py >/dev/null && echo "bridge up" || echo "bridge DOWN")
  # The location ID gains a hex digit per hub hop: 0x01100000 is a root port.
  loc=$(ioreg -p IOUSB -w0 | grep -o "Allium58@[0-9a-f]*" | head -1 | cut -d@ -f2)
  case "$loc" in
    "") path="-" ;;
    ???00000) path="direct" ;;
    *) path="via hub/dock" ;;
  esac
  line="PROBE  $state  [$path]"
  # Log changes immediately, and a heartbeat every 5 minutes.
  if [ "$line" != "$last" ] || [ $beat -ge 30 ]; then
    echo "$(date +%T) $line" >> "$LOG"
    last="$line"; beat=0
  fi
  beat=$((beat + 1))
  sleep 10
done

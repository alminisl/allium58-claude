#!/bin/bash
# Sets up the host bridge: Python venv with hidapi, launchd agent, running.
# Then merge host/claude-settings.example.json into ~/.claude/settings.json.
set -e
H="$HOME/allium58/host"
[ -f "$H/agentd.py" ] || { echo "clone this repo to ~/allium58 first"; exit 1; }
python3 -m venv "$H/.venv"
"$H/.venv/bin/pip" install -q hidapi
sed "s|__HOME__|$HOME|g" "$H/com.allium58.agentd.plist" > "$HOME/Library/LaunchAgents/com.allium58.agentd.plist"
"$H/bridge" resume
echo "bridge running on 127.0.0.1:8738; now add the hooks from host/claude-settings.example.json"

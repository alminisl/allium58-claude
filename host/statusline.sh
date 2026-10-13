#!/bin/bash
# Claude Code status line. Hands the payload to the Allium58 bridge, which
# records context and usage for the keyboard and replies with the line to show.
curl -s -m 1 -X POST -H 'Content-Type: application/json' --data-binary @- http://127.0.0.1:8738/statusline 2>/dev/null || printf 'claude'

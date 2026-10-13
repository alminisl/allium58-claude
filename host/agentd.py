#!/usr/bin/env python3
"""Allium58 bridge: Claude Code sessions -> the keyboard's two OLEDs.

Runs at login (launchd, see com.allium58.agentd.plist) and listens on
127.0.0.1:8738 for two things:

  POST /hook        every Claude Code hook event, piped in by curl
  POST /statusline  Claude Code's statusLine payload; the reply is the
                    status line Claude Code shows

From those it keeps a table of live sessions and drives the keyboard over
raw HID (protocol in keymaps/claude/keymap.c):

  left screen   the mascot reacts to each event, bloats with the context
                of the session it follows, and shows clock or project on top
  right screen  one row per active session: state, context %, turn timer;
                spare rows show the 5-hour usage window and today's spend

The keyboard is held open while the bridge runs (opening and closing it every
second stalled the RP2040's USB within minutes) and released whenever Vial is
running. Every 30 s the bridge asks the keyboard for its VIA protocol version
and logs when it stops answering.
"""

import json
import os
import re
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PORT = 8738
VID, PID = 0xBEEB, 0x0004
USAGE_PAGE, USAGE = 0xFF60, 0x61
HID_EVENT, HID_RIGHT, HID_STATUS, HID_CONTEXT = 0xC1, 0xC2, 0xC3, 0xC4

# Mirrors enum clawd_event / clawd_tool in clawd.h.
EV_SYNC, EV_SESSION_START, EV_PROMPT, EV_TOOL, EV_TOOL_FAILED, EV_PERMISSION, \
    EV_STOP, EV_STOP_FAILED, EV_RATE_LIMIT, EV_COMPACT, EV_SUBAGENT, EV_SESSION_END = range(12)
TOOL_OTHER, TOOL_EDIT, TOOL_BASH, TOOL_READ, TOOL_WEB = range(5)
TOOLS = [
    (r"^(Edit|Write|NotebookEdit|MultiEdit)$", TOOL_EDIT),
    (r"^Bash$", TOOL_BASH),
    (r"^(Read|Grep|Glob|LS)$", TOOL_READ),
    (r"^(WebFetch|WebSearch|mcp__.*)$", TOOL_WEB),
]

ACTIVE_S = 30 * 60      # a session silent this long drops off the screen
STALE_WORK_S = 15 * 60  # "working" with no events this long is assumed over
DONE_S = 5 * 60         # how long a finished turn reads "done" before "idle"
TICK_S = 1.0    # the turn timer ticks in seconds; the right half animates the spinner itself


def log(*args):
    print(time.strftime("%H:%M:%S"), *args, file=sys.stderr, flush=True)


def tool_category(name):
    for pattern, cat in TOOLS:
        if re.match(pattern, name or ""):
            return cat
    return TOOL_OTHER


def fmt_dur(seconds):
    s = max(0, int(seconds))
    if s >= 3600:
        return f"{s // 3600}h{s // 60 % 60:02d}"
    if s >= 600:
        return f"{s // 60}m"
    return f"{s // 60}:{s % 60:02d}"


class Session:
    def __init__(self, sid, now):
        self.id = sid
        self.first_seen = now
        self.last_seen = now
        self.last_event = 0.0
        self.cwd = ""
        self.state = "idle"     # idle | working | waiting | done | error
        self.turn_start = 0.0
        self.done_at = 0.0
        self.ctx = None
        self.cost = 0.0
        self.model = ""

    @property
    def name(self):
        return os.path.basename(self.cwd.rstrip("/")) or self.id[:8]

    def effective_state(self, now):
        if self.state in ("working", "waiting") and now - self.last_event > STALE_WORK_S:
            return "idle"
        if self.state in ("done", "error") and now - self.done_at > DONE_S:
            return "idle"
        return self.state


class State:
    def __init__(self):
        self.lock = threading.Lock()
        self.sessions = {}
        self.usage_pct = None
        self.usage_resets = None
        self.followed = None   # session id the mascot follows

    def session(self, sid, now):
        s = self.sessions.get(sid)
        if s is None:
            s = self.sessions[sid] = Session(sid, now)
        s.last_seen = now
        return s

    def hook(self, h):
        """Applies one hook payload. Returns (event, tool) for the mascot."""
        now = time.time()
        name = h.get("hook_event_name", "")
        sid = h.get("session_id") or "unknown"
        with self.lock:
            if name == "SessionEnd":
                self.sessions.pop(sid, None)
                if self.followed == sid:
                    self.followed = None
                return EV_SESSION_END, 0
            s = self.session(sid, now)
            s.last_event = now
            # A session's cwd follows every `cd` it makes; the name should be
            # the project it was started in, so keep the first one seen.
            if h.get("cwd") and not s.cwd:
                s.cwd = h["cwd"]
            self.followed = sid
            ev, tool = EV_SYNC, 0

            def work():
                if s.state not in ("working", "waiting"):
                    s.turn_start = now
                s.state = "working"

            if name == "SessionStart":
                ev = EV_SESSION_START
            elif name == "UserPromptSubmit":
                s.turn_start = now
                s.state = "working"
                ev = EV_PROMPT
            elif name == "PreToolUse":
                work()
                ev, tool = EV_TOOL, tool_category(h.get("tool_name"))
            elif name == "PostToolUse":
                work()   # also clears an answered permission prompt
            elif name == "PostToolUseFailure":
                work()
                ev = EV_TOOL_FAILED
            elif name == "PermissionRequest" or (
                    name == "Notification" and h.get("notification_type") == "permission_prompt"):
                work()
                s.state = "waiting"
                ev = EV_PERMISSION
            elif name == "Notification" and h.get("notification_type") == "idle_prompt":
                # Waiting for the next prompt: the turn is over, even an
                # interrupted one that never sent Stop.
                if s.state in ("working", "waiting"):
                    s.state = "idle"
            elif name in ("Stop", "StopFailure"):
                s.state = "done" if name == "Stop" else "error"
                s.done_at = now
                if name == "Stop":
                    ev = EV_STOP
                elif h.get("error_type") == "rate_limit":
                    ev = EV_RATE_LIMIT
                else:
                    ev = EV_STOP_FAILED
            elif name == "PreCompact":
                ev = EV_COMPACT
            elif name == "SubagentStart":
                ev = EV_SUBAGENT
            return ev, tool

    def statusline(self, p):
        """Records a statusLine payload and returns the line to display."""
        now = time.time()
        with self.lock:
            s = self.session(p.get("session_id") or "unknown", now)
            project = (p.get("workspace") or {}).get("project_dir")
            if project:
                s.cwd = project
            elif p.get("cwd") and not s.cwd:
                s.cwd = p["cwd"]
            ctx = (p.get("context_window") or {}).get("used_percentage")
            if isinstance(ctx, (int, float)):
                s.ctx = int(round(ctx))
            cost = (p.get("cost") or {}).get("total_cost_usd")
            if isinstance(cost, (int, float)):
                s.cost = float(cost)
            s.model = (p.get("model") or {}).get("display_name") or s.model
            five = (p.get("rate_limits") or {}).get("five_hour") or {}
            if isinstance(five.get("used_percentage"), (int, float)):
                self.usage_pct = int(round(five["used_percentage"]))
                self.usage_resets = five.get("resets_at")
            parts = [s.model or "claude"]
            if s.ctx is not None:
                parts.append(f"ctx {s.ctx}%")
            parts.append(f"${s.cost:.2f}")
            usage = self.usage_text(now)
            if usage:
                parts.append(usage)
            return " · ".join(parts)

    def usage_text(self, now):
        if self.usage_pct is None:
            return ""
        text = f"5h {self.usage_pct}%"
        if isinstance(self.usage_resets, (int, float)) and self.usage_resets > now:
            text += f" ({fmt_dur(self.usage_resets - now)})"
        return text

    def active(self, now):
        live = [s for s in self.sessions.values() if now - s.last_seen < ACTIVE_S]
        return sorted(live, key=lambda s: s.first_seen)

    def counts(self, now):
        with self.lock:
            states = [s.effective_state(now) for s in self.active(now)]
        return (sum(st in ("working", "waiting") for st in states), states.count("waiting"))

    def screens(self, now, tick):
        """Returns (right rows [(text, invert)] x4, left status, mascot context)."""
        with self.lock:
            sessions = self.active(now)
            rows = []
            for s in sessions:
                st = s.effective_state(now)
                icon = {"working": "|", "waiting": "!", "done": "*", "error": "x"}.get(st, " ")
                right = {"waiting": "WAIT", "done": "done", "error": "err", "idle": "idle"}.get(st)
                if right is None:
                    right = fmt_dur(now - s.turn_start)
                ctx = f"{s.ctx:>3}%" if s.ctx is not None else " --%"
                rows.append((f"{icon} {s.name[:9]:<9}{ctx} {right:>5}"[:21], st == "waiting"))

            # More sessions than rows: page through them, four at a time.
            if len(rows) > 4:
                pages = (len(rows) + 3) // 4
                page = int(now / 4) % pages
                rows = rows[page * 4:page * 4 + 4]

            extras = []
            usage = self.usage_text(now)
            if usage:
                extras.append(usage.replace("(", "resets ").replace(")", ""))
            today = time.strftime("%Y-%m-%d")
            spend = sum(s.cost for s in self.sessions.values()
                        if time.strftime("%Y-%m-%d", time.localtime(s.last_seen)) == today)
            if spend:
                extras.append(f"today ${spend:.2f}")
            if not sessions:
                rows = [("no active sessions", False)]
                extras.append(time.strftime("%a %d %b  %H:%M"))
            while len(rows) < 4 and extras:
                rows.append((extras.pop(0)[:21], False))
            rows += [("", False)] * (4 - len(rows))

            followed = self.sessions.get(self.followed)
            if followed and followed.effective_state(now) != "idle" and now - followed.last_seen < ACTIVE_S:
                status = followed.name[:13]
                ctx = followed.ctx or 0
            else:
                status = time.strftime("%H:%M %a %d")
                ctx = (followed.ctx or 0) if followed else 0
            return rows, status, ctx


class Link:
    """The keyboard's raw HID interface, held open until Vial needs it."""

    def __init__(self):
        self.lock = threading.Lock()
        self.dev = None
        self.vial_checked = 0.0
        self.vial = False
        self.present = False
        self.generation = 0   # bumps whenever the keyboard reappears, so screens resend

    def vial_running(self):
        now = time.time()
        if now - self.vial_checked > 0.5:
            self.vial_checked = now
            self.vial = subprocess.run(["pgrep", "-xq", "Vial"]).returncode == 0
        return self.vial

    def _close(self):
        if self.dev:
            try:
                self.dev.close()
            except Exception:
                pass
            self.dev = None

    def _open(self):
        if self.vial_running():
            if self.dev:
                log("Vial is open, releasing the keyboard")
                self._close()
            return False
        if self.dev:
            return True
        import hid
        for info in hid.enumerate(VID, PID):
            if info["usage_page"] == USAGE_PAGE and info["usage"] == USAGE:
                try:
                    dev = hid.device()
                    dev.open_path(info["path"])
                    dev.set_nonblocking(1)
                except OSError:
                    break
                self.dev = dev
                if not self.present:
                    self.present = True
                    self.generation += 1
                    log("keyboard connected")
                    self._report_last_hang()
                return True
        if self.present:
            self.present = False
            log("keyboard unavailable" + (" (Vial is open)" if self.vial else ""))
        return False

    STAGES = {1: "QMK core (key scan, USB, split link)", 2: "housekeeping", 3: "split RPC send",
              4: "mascot render", 5: "OLED write", 6: "raw HID handler"}

    def _report_last_hang(self):
        """Logs where the firmware hung if the hardware watchdog reset it (0xC6)."""
        try:
            self.dev.write(b"\x00\xc6" + bytes(31))
            deadline = time.time() + 1
            while time.time() < deadline:
                r = self.dev.read(32)
                if r and r[0] == 0xC6:
                    if r[1]:
                        log(f"WATCHDOG RESET: firmware hung in {self.STAGES.get(r[2], r[2])}")
                    self.stack_peak = r[3] << 8 | r[4]
                    self.stack_size = r[5] << 8 | r[6]
                    return
                time.sleep(0.02)
        except (OSError, ValueError):
            pass

    def send_many(self, packets):
        """Opens the keyboard, writes `packets`, and closes it again.
        Returns how many were written."""
        with self.lock:
            if not self._open():
                return 0
            sent = 0
            try:
                for packet in packets:
                    packet = packet[:32] + bytes(32 - min(len(packet), 32))
                    self.dev.write(b"\x00" + packet)
                    sent += 1
                    # VIA echoes every packet back; drain so nothing piles up.
                    while self.dev.read(32):
                        pass
            except (OSError, ValueError):
                self.present = False
                log("keyboard lost")
                self._close()
            return sent

    def healthy(self):
        """True if the keyboard answers a VIA protocol query, None if it is
        not open."""
        with self.lock:
            if not self._open():
                return None
            try:
                self.dev.write(b"\x00\x01" + bytes(31))
                deadline = time.time() + 1
                while time.time() < deadline:
                    r = self.dev.read(32)
                    if r and r[0] == 0x01:
                        return True
                    time.sleep(0.02)
                return False
            except (OSError, ValueError):
                return False

    def send(self, cmd, payload=b""):
        return self.send_many([bytes([cmd]) + payload]) == 1


STATE = State()
LINK = Link()


def send_event(ev, tool):
    working, waiting = STATE.counts(time.time())
    LINK.send(HID_EVENT, bytes([ev, tool, min(working, 255), min(waiting, 255)]))


def pusher():
    """Keeps both screens current, sending only what changed."""
    sent = {}
    generation = -1
    last_health, health = 0.0, None
    tick = 0
    last_full = 0.0
    while True:
        now = time.time()
        try:
            rows, status, ctx = STATE.screens(now, tick)
            if LINK.generation != generation or now - last_full > 60:
                sent.clear()
                generation = LINK.generation
                last_full = now
            pending = []
            for r, (text, invert) in enumerate(rows):
                if sent.get(("row", r)) != (text, invert):
                    pending.append((("row", r), (text, invert),
                                    bytes([HID_RIGHT, r, int(invert)]) + text.ljust(21).encode("ascii", "replace")))
            if sent.get("status") != status:
                pending.append(("status", status, bytes([HID_STATUS]) + status.encode("ascii", "replace")[:21]))
            if sent.get("ctx") != ctx:
                pending.append(("ctx", ctx, bytes([HID_CONTEXT, max(0, min(100, ctx))])))
            if pending:
                done = LINK.send_many([p for _, _, p in pending])
                for key, value, _ in pending[:done]:
                    sent[key] = value
            if now - last_health > 30:
                last_health = now
                h = LINK.healthy()
                if h is not None:
                    with LINK.lock:
                        if LINK.dev:
                            LINK._report_last_hang()
                    if getattr(LINK, "stack_peak", 0) > getattr(LINK, "stack_logged", 0):
                        LINK.stack_logged = LINK.stack_peak
                        log(f"stack peak {LINK.stack_peak} of {LINK.stack_size} bytes")
                if h is not None and h != health:
                    log("health ok" if h else "health STUCK: keyboard not answering")
                    health = h
                if h is False:
                    # A handle opened during a reset storm can stay dead while
                    # writes still "succeed"; reopen instead of sending into it.
                    with LINK.lock:
                        LINK._close()
                        LINK.present = False
                    log("reopening the keyboard")
        except Exception as e:  # never let a formatting bug kill the screens
            log("pusher:", repr(e))
        tick += 1
        time.sleep(TICK_S)


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        try:
            body = self.rfile.read(int(self.headers.get("Content-Length") or 0))
            payload = json.loads(body) if body.strip() else {}
        except (ValueError, json.JSONDecodeError):
            payload = {}
        reply = b""
        if self.path == "/hook":
            ev, tool = STATE.hook(payload)
            send_event(ev, tool)
        elif self.path == "/statusline":
            reply = STATE.statusline(payload).encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(reply)))
        self.end_headers()
        self.wfile.write(reply)

    def log_message(self, *args):
        pass


def main():
    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    threading.Thread(target=pusher, daemon=True).start()
    log(f"listening on 127.0.0.1:{PORT}")
    server.serve_forever()


if __name__ == "__main__":
    main()

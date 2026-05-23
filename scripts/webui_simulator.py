#!/usr/bin/env python3
"""Local WebUI simulator for the ESP32 dashboard.

It serves the same dashboard HTML source used by firmware and provides a
stateful subset of the ESP32 HTTP API so the UI can be inspected without
flashing or connecting to a car.
"""
from __future__ import annotations

import argparse
import json
import random
import re
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlparse


ROOT = Path(__file__).resolve().parent.parent
HTML_SRC = ROOT / "include" / "web" / "mcp2515_dashboard_ui.src.h"
VERSION_FILE = ROOT / "VERSION"


def load_dashboard_html() -> str:
    text = HTML_SRC.read_text(encoding="utf-8")
    match = re.search(r'R"HTML\((.*)\)HTML";', text, re.DOTALL)
    if not match:
        raise RuntimeError(f"HTML payload not found in {HTML_SRC}")
    return match.group(1)


class SimState:
    def __init__(self) -> None:
        self.started = time.monotonic()
        self.hw = 1
        self.sp = 1
        self.sp_auto = True
        self.can = True
        self.injection = True
        self.ad = False
        self.rx = 2480
        self.tx = 114
        self.txerr = 0
        self.follow_dist = 3
        self.gateway_ap = 1
        self.hw3_offset_slew = True
        self.hw3_slew_rate = 5
        self.hw3_offset_target = 0
        self.hw3_offset_last = 0
        self.hw3_slew_count = 0
        self.led_brightness = 120
        self.enable_print = True
        self.ap_ssid = "EV-CAN-WAVESHARE"
        self.ap_hidden = False
        self.ap_clients = 1
        self.wifi_connected = True
        self.wifi_ssid = "Home-WiFi"
        self.wifi_ip = "192.168.1.88"
        self.wifi_active = 0
        self.wifi_networks = [
            {
                "idx": 0,
                "ssid": "Home-WiFi",
                "hasPass": True,
                "static": False,
            }
        ]
        self.rec_active = False
        self.rec_count = 0
        self.rec_saved = False
        self.gateway_enabled = True
        self.gateway_nat = True
        self.gateway_mode = 1
        self.gateway_strict = False
        self.gateway_blacklist = "\n".join(
            [
                "telemetry.vn.teslamotors.com",
                "telemetry-prd.vn.cloud.tesla.cn",
                "owner-api.vn.teslamotors.com",
                "auth.tesla.cn",
            ]
        )
        self.gateway_whitelist = "pool.ntp.org\n*.github.com"
        self.gateway_blocked = [
            {"domain": "telemetry.vn.teslamotors.com", "count": 8},
            {"domain": "owner-api.vn.teslamotors.com", "count": 2},
            {"domain": "maps.example.com", "count": 3},
        ]
        self.beta = True
        self.auto_update = False
        self.log_seq = 4
        self.log_lines = [
            "[BOOT] Waveshare ESP32-S3 RS485/CAN simulator started",
            "[WIFI] AP EV-CAN-WAVESHARE up at 100.100.1.1",
            "[WIFI] STA connected to Home-WiFi at 192.168.1.88",
            "[GW] STA-AP NAT enabled, DNS filter active",
        ]


    def status(self) -> dict[str, Any]:
        self.rx += random.randint(3, 18)
        if self.injection:
            self.tx += random.randint(0, 3)
        self.ad = (self.uptime() // 12) % 2 == 1
        fps = round(7.5 + random.random() * 3.5, 1)
        return {
            "hw": self.hw,
            "sp": self.sp,
            "spAuto": self.sp_auto,
            "soff": self.hw3_offset_last,
            "gtwap": self.gateway_ap,
            "AD": self.ad,
            "eprn": self.enable_print,
            "ia": self.injection,
            "hw3OffsetSlew": self.hw3_offset_slew,
            "hw3SlewRate": self.hw3_slew_rate,
            "hw3OffsetTarget": self.hw3_offset_target,
            "hw3OffsetLast": self.hw3_offset_last,
            "hw3SlewCount": self.hw3_slew_count,
            "ledB": self.led_brightness,
            "can": self.can,
            "ci": self.injection,
            "rx": self.rx,
            "tx": self.tx,
            "txerr": self.txerr,
            "fd": self.follow_dist,
            "fps": fps,
            "eflg": 0,
            "up": self.uptime(),
            "probe": {
                "active": False,
                "state": 0,
                "id": 1021,
                "mux": 0,
                "txa": 0,
                "rxa": 0,
                "txdlc": 8,
                "rxdlc": 8,
                "hasrx": True,
                "tx": [0, 0, 0, 60, 32, 0, 4, 0],
                "rx": [0, 0, 0, 60, 32, 0, 0, 0],
            },
            "mux": [
                {"rx": self.rx // 3, "tx": self.tx // 3, "err": 0},
                {"rx": self.rx // 4, "tx": self.tx // 4, "err": 0},
                {"rx": self.rx // 5, "tx": self.tx // 5, "err": 0},
            ],
        }

    def frames(self) -> dict[str, Any]:
        now = self.uptime() * 1000
        base = [
            (1021, "DI_state", [0, 0, 0, 60, 32, 0, 0, 0]),
            (1016, "AP_status", [1, 0, 0, 0, 0, 0, 0, 0]),
            (921, "ISA_chime", [0, 0, 0, 0, 0, 0, 0, 0]),
            (257, "Gateway_heartbeat", [self.gateway_ap, 0, 0, 0, 0, 0, 0, 0]),
            (0x678, "GTW_gearControl", [0, 0, 0, 0, 4, 0, 0, 0]),
            (0x233, "UI_stalklessControl", [0, 0, 2, 0, 0, 0, 0, 0]),
            (0x118, "DI_systemStatus", [0, 4, 3, 0, 0, 0, 0, 0]),
        ]
        frames = []
        for i, (can_id, name, data) in enumerate(base):
            d = list(data)
            d[0] = (d[0] + self.uptime() + i) & 0xFF
            frames.append({"ts": now - i * 87, "id": can_id, "dlc": 8, "data": d, "name": name})
        return {"frames": frames}


HTML = load_dashboard_html()
VERSION = VERSION_FILE.read_text(encoding="utf-8").strip() if VERSION_FILE.exists() else "3.0.0-beta.5"
STATE = SimState()


def _dns_rules(text):
    return [x.strip().lower().removeprefix("*.").rstrip(".") for x in re.split(r"[\s,;]+", text or "") if x.strip()]


def _domain_in_list(domain, rules):
    return _domain_rule_match_len(domain, rules) > 0


def _domain_rule_match_len(domain, rules):
    domain = (domain or "").strip().lower().removeprefix("*.").rstrip(".")
    best = 0
    for rule in rules:
        if domain == rule or domain.endswith("." + rule):
            best = max(best, len(rule))
    return best


def _dns_decision(domain):
    domain = (domain or "").strip().lower().removeprefix("*.").rstrip(".")
    blacklist = _dns_rules(STATE.gateway_blacklist)
    whitelist = _dns_rules(STATE.gateway_whitelist)
    blacklisted = _domain_in_list(domain, blacklist)
    whitelisted = _domain_in_list(domain, whitelist)
    block_len = _domain_rule_match_len(domain, blacklist)
    allow_len = _domain_rule_match_len(domain, whitelist)
    if not STATE.gateway_enabled:
        allowed, reason = True, "gateway disabled"
    elif not domain:
        allowed, reason = False, "empty domain"
    elif allow_len > 0 and allow_len >= block_len:
        allowed, reason = True, "matched whitelist"
    elif block_len > 0:
        allowed, reason = False, "matched blacklist"
    elif STATE.gateway_mode == 0:
        allowed, reason = True, "not in blacklist"
    else:
        allowed, reason = False, "not in whitelist"
    return {
        "domain": domain,
        "enabled": STATE.gateway_enabled,
        "mode": STATE.gateway_mode,
        "allowed": allowed,
        "blocked": (not allowed) and STATE.gateway_enabled and bool(domain),
        "blacklisted": blacklisted,
        "whitelisted": whitelisted,
        "reason": reason,
    }


def _read_bits_le(data, start, length):
    value = 0
    for i in range(length):
        bit = start + i
        if bit // 8 >= len(data):
            break
        if data[bit // 8] & (1 << (bit % 8)):
            value |= 1 << i
    return value


def _gear_label(value, short=False):
    if value == 1:
        return "P" if short else "PARK"
    if value == 2:
        return "R" if short else "REVERSE"
    if value == 3:
        return "N" if short else "NEUTRAL"
    if value == 4:
        return "D" if short else "DRIVE"
    return "IDLE_SNA" if value == 0 else "UNKNOWN"


def _gear_frame(frame_id, name, signal, data, count, short=False):
    cands = []
    for bit in range(0, max(0, len(data) * 8 - 2)):
        v = _read_bits_le(data, bit, 3)
        if v in (2, 4):
            cands.append({"bit": bit, "value": v, "label": _gear_label(v, short)})
    return {
        "seen": True,
        "id": frame_id,
        "name": name,
        "signal": signal,
        "ts": int(STATE.uptime() * 1000),
        "age": 80,
        "count": count,
        "dlc": len(data),
        "data": data,
        "candidates": cands,
    }


class Handler(BaseHTTPRequestHandler):
    server_version = "EVWebUISim/1.0"

    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"{self.address_string()} - {fmt % args}")

    def send_obj(self, obj: Any, status: int = 200) -> None:
        data = json.dumps(obj, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def send_text(self, text: str, content_type: str = "text/plain", status: int = 200) -> None:
        data = text.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        self.send_header("Pragma", "no-cache")
        self.end_headers()
        self.wfile.write(data)

    def read_form(self) -> dict[str, str]:
        length = int(self.headers.get("Content-Length", "0") or "0")
        raw = self.rfile.read(length).decode("utf-8", errors="replace") if length else ""
        if self.headers.get("Content-Type", "").startswith("application/json"):
            try:
                return {"plain": raw, **json.loads(raw)}
            except Exception:
                return {"plain": raw}
        parsed = parse_qs(raw, keep_blank_values=True)
        return {k: v[-1] if v else "" for k, v in parsed.items()}

    def do_GET(self) -> None:
        parsed_url = urlparse(self.path)
        path = parsed_url.path
        qs = parse_qs(parsed_url.query, keep_blank_values=True)
        if path == "/":
            self.send_text(HTML, "text/html; charset=utf-8")
        elif path == "/status":
            self.send_obj(STATE.status())
        elif path == "/frames":
            self.send_obj(STATE.frames())
        elif path == "/gear_status":
            self.send_obj(
                {
                    "source": "mcu3",
                    "readOnly": True,
                    "note": "MCU3 JSON provides enum values but no bit offsets; candidates are 3-bit raw D/R windows.",
                    "gtw": _gear_frame(0x678, "GTW_gearControl", "GTW_gearShiftRequest", [0, 0, 0, 0, 4, 0, 0, 0], STATE.rx // 7 + 1),
                    "ui": _gear_frame(0x233, "UI_stalklessControl", "UI_gearRequest", [0, 0, 2, 0, 0, 0, 0, 0], STATE.rx // 9 + 1),
                    "di": _gear_frame(0x118, "DI_systemStatus", "DI_gear / DI_shiftRequestType", [0, 4, 3, 0, 0, 0, 0, 0], STATE.rx // 8 + 1, True),
                }
            )
        elif path == "/log":
            self.send_obj({"seq": STATE.log_seq, "lines": STATE.log_lines[-20:]})
        elif path == "/rec_status":
            self.send_obj({"active": STATE.rec_active, "count": STATE.rec_count, "cap": 512, "saved": STATE.rec_saved})
        elif path == "/rec_download":
            self.send_text("ts,id,dlc,data\n0,1021,8,00 00 00 3c 20 00 00 00\n", "text/csv")
        elif path == "/ap_status":
            self.send_obj(
                {
                    "ssid": STATE.ap_ssid,
                    "ip": "100.100.1.1",
                    "clients": STATE.ap_clients,
                    "stored": True,
                    "hidden": STATE.ap_hidden,
                }
            )
        elif path == "/wifi_status":
            self.send_obj(
                {
                    "connected": STATE.wifi_connected,
                    "ssid": STATE.wifi_ssid,
                    "stored": bool(STATE.wifi_networks),
                    "count": len(STATE.wifi_networks),
                    "active": STATE.wifi_active,
                    "ip": STATE.wifi_ip,
                    "static": False,
                }
            )
        elif path == "/system_status":
            uptime = STATE.uptime()
            heap_total = 327680
            heap_free = 214000 - (uptime % 9000)
            self.send_obj(
                {
                    "chip": "ESP32-S3",
                    "target": "esp32s3",
                    "cores": 2,
                    "revision": 2,
                    "cpu_mhz": 160,
                    "idf": "6.0.1",
                    "firmware": VERSION,
                    "mac": "A4:CB:8F:DA:D5:44",
                    "reset": "software",
                    "uptime": uptime,
                    "tasks": 18,
                    "core": uptime % 2,
                    "heap_total": heap_total,
                    "heap_free": heap_free,
                    "heap_min": 188000,
                    "heap_largest": 128000,
                    "internal_free": heap_free,
                    "psram_total": 0,
                    "psram_free": 0,
                    "flash_size": 16 * 1024 * 1024,
                    "flash_speed": 80_000_000,
                    "app_addr": 0x10000,
                    "app_size": 4 * 1024 * 1024,
                    "app_used": 1_025_343,
                    "app_label": "app0",
                    "spiffs_ok": True,
                    "spiffs_total": 0x7E0000,
                    "spiffs_used": 128 * 1024,
                    "wifi_connected": STATE.wifi_connected,
                    "wifi_rssi": -43 if STATE.wifi_connected else None,
                    "ap_clients": STATE.ap_clients,
                    "temp_c": 41.6,
                }
            )
        elif path == "/wifi_networks":
            self.send_obj(
                {
                    "count": len(STATE.wifi_networks),
                    "max": 4,
                    "active": STATE.wifi_active,
                    "networks": STATE.wifi_networks,
                }
            )
        elif path == "/wifi_scan":
            self.send_obj(
                {
                    "networks": [
                        {"ssid": "Home-WiFi", "rssi": -42, "ch": 6, "enc": True},
                        {"ssid": "Tesla-Service", "rssi": -67, "ch": 11, "enc": True},
                        {"ssid": "EV-CAN-WAVESHARE", "rssi": -31, "ch": 1, "enc": True},
                    ]
                }
            )
        elif path == "/gateway_status":
            self.send_obj(
                {
                    "enabled": STATE.gateway_enabled,
                    "nat": STATE.gateway_nat and STATE.wifi_connected,
                    "mode": STATE.gateway_mode,
                    "strict": STATE.gateway_strict,
                    "blocked": sum(x["count"] for x in STATE.gateway_blocked),
                    "upstream": "8.8.8.8",
                }
            )
        elif path == "/gateway_dns":
            self.send_obj(
                {
                    "enabled": STATE.gateway_enabled,
                    "mode": STATE.gateway_mode,
                    "strict": STATE.gateway_strict,
                    "blacklist": STATE.gateway_blacklist,
                    "whitelist": STATE.gateway_whitelist,
                }
            )
        elif path == "/gateway_dns_test":
            domain = qs.get("domain", [""])[0]
            self.send_obj(_dns_decision(domain))
        elif path == "/gateway_blocked":
            rows = []
            for item in STATE.gateway_blocked:
                dom = item["domain"].lower()
                decision = _dns_decision(dom)
                blacklisted = decision["blacklisted"]
                whitelisted = decision["whitelisted"]
                rows.append({**item, "blacklisted": blacklisted, "whitelisted": whitelisted, "canWhitelist": not blacklisted})
            self.send_obj(rows)
        elif path == "/can_pins":
            self.send_obj({"tx": 15, "rx": 16, "customized": False})
        elif path == "/settings_export":
            self.send_obj(
                {
                    "version": VERSION,
                    "ap": {"ssid": STATE.ap_ssid, "pass": "", "hidden": STATE.ap_hidden},
                    "wifi": {"ssid": STATE.wifi_ssid, "pass": "", "static": False, "ip": "", "gw": "", "mask": "", "dns": ""},
                    "hw3": {"offsetSlew": STATE.hw3_offset_slew, "slewRate": STATE.hw3_slew_rate},
                    "can": {"tx": 15, "rx": 16},
                    "beta": STATE.beta,
                }
            )
        elif path == "/update_check":
            self.send_obj(
                {
                    "ok": True,
                    "current": VERSION,
                    "latest": VERSION,
                    "update": False,
                    "prerelease": True,
                    "artifact": "waveshare_ESP32_S3_RS485_CAN",
                    "tag": "local-sim",
                    "url": "",
                }
            )
        elif path == "/update_beta":
            self.send_obj({"beta": STATE.beta})
        elif path == "/auto_update":
            self.send_obj({"enabled": STATE.auto_update})
        elif path == "/sim/state":
            self.send_obj(
                {
                    "can": STATE.can,
                    "injection": STATE.injection,
                    "wifi_connected": STATE.wifi_connected,
                    "gateway_enabled": STATE.gateway_enabled,
                    "gateway_nat": STATE.gateway_nat,
                }
            )
        else:
            self.send_text("not found", status=404)

    def do_POST(self) -> None:
        path = urlparse(self.path).path
        form = self.read_form()
        if path == "/config":
            if "hw" in form:
                STATE.hw = max(0, min(2, int(form["hw"] or 0)))
            if "sp" in form:
                STATE.sp = max(0, min(4, int(form["sp"] or 0)))
            if "spa" in form:
                STATE.sp_auto = form["spa"] == "1"
            if "can" in form:
                STATE.injection = form["can"] == "1"
            if "hw3OffsetSlew" in form:
                STATE.hw3_offset_slew = form["hw3OffsetSlew"] == "1"
            if "hw3SlewRate" in form:
                STATE.hw3_slew_rate = max(1, min(25, int(form["hw3SlewRate"] or 5)))
            self.send_obj({"ok": True})
        elif path == "/led_brightness":
            STATE.led_brightness = max(0, min(255, int(form.get("b", STATE.led_brightness) or 0)))
            self.send_obj({"ok": True})
        elif path == "/logging":
            if "eprn" in form:
                STATE.enable_print = form["eprn"] == "1"
            self.send_obj({"ok": True})
        elif path == "/disable":
            STATE.injection = False
            self.send_text("Injection stopped.")
        elif path == "/reset_stats":
            STATE.rx = 0
            STATE.tx = 0
            STATE.txerr = 0
            self.send_obj({"ok": True})
        elif path == "/rec_start":
            STATE.rec_active = True
            STATE.rec_saved = False
            STATE.rec_count = 0
            self.send_obj({"ok": True})
        elif path == "/rec_stop":
            STATE.rec_active = False
            STATE.rec_saved = True
            STATE.rec_count = 24
            self.send_obj({"ok": True})
        elif path == "/ap_config":
            if form.get("ssid"):
                STATE.ap_ssid = form["ssid"][:32]
            STATE.ap_hidden = form.get("hidden") in ("1", "true")
            self.send_obj({"ok": True, "msg": "Saved in simulator"})
        elif path == "/wifi_config":
            ssid = form.get("ssid", "Sim-WiFi")[:32]
            idx = int(form.get("idx", -1) or -1)
            item = {"idx": idx if idx >= 0 else len(STATE.wifi_networks), "ssid": ssid, "hasPass": bool(form.get("pass")), "static": form.get("static") == "1"}
            if idx >= 0 and idx < len(STATE.wifi_networks):
                STATE.wifi_networks[idx] = item
            else:
                STATE.wifi_networks.append(item)
            STATE.wifi_ssid = ssid
            STATE.wifi_connected = True
            STATE.wifi_active = item["idx"]
            self.send_obj({"ok": True})
        elif path == "/wifi_delete":
            idx = int(form.get("idx", -1) or -1)
            STATE.wifi_networks = [n for n in STATE.wifi_networks if n.get("idx") != idx]
            STATE.wifi_active = STATE.wifi_networks[0]["idx"] if STATE.wifi_networks else -1
            self.send_obj({"ok": True})
        elif path == "/gateway_dns":
            STATE.gateway_enabled = form.get("enabled", "0") in ("1", "true")
            STATE.gateway_mode = int(form.get("mode", STATE.gateway_mode) or 0)
            STATE.gateway_strict = form.get("strict", "0") in ("1", "true")
            STATE.gateway_blacklist = form.get("blacklist", STATE.gateway_blacklist)
            STATE.gateway_whitelist = form.get("whitelist", STATE.gateway_whitelist)
            self.send_obj({"ok": True})
        elif path == "/gateway_blocked_clear":
            STATE.gateway_blocked = []
            self.send_obj({"ok": True})
        elif path == "/gateway_whitelist_add":
            domain = form.get("domain", "").strip().lower()
            blacklist = _dns_rules(STATE.gateway_blacklist)
            if _domain_rule_match_len(domain, blacklist) > 0:
                self.send_obj({"ok": False, "error": "domain is blacklisted"}, status=409)
                return
            whitelist = _dns_rules(STATE.gateway_whitelist)
            if _domain_rule_match_len(domain, whitelist) > 0:
                self.send_obj({"ok": True, "already": True})
                return
            STATE.gateway_whitelist = (STATE.gateway_whitelist.rstrip() + "\n" + domain).strip()
            self.send_obj({"ok": True})
        elif path == "/can_pins":
            self.send_obj({"ok": True, "reboot": True})
        elif path in ("/settings_import", "/update_install", "/reboot"):
            self.send_obj({"ok": True, "reboot": path in ("/settings_import", "/reboot")})
        elif path == "/update_beta":
            if "beta" in form:
                STATE.beta = form["beta"] == "1"
            self.send_obj({"beta": STATE.beta})
        elif path == "/auto_update":
            if "enabled" in form:
                STATE.auto_update = form["enabled"] == "1"
            self.send_obj({"enabled": STATE.auto_update})
        else:
            self.send_text("not found", status=404)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the local WebUI simulator.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8088)
    args = parser.parse_args()
    httpd = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"EV WebUI simulator: http://{args.host}:{args.port}")
    print("Press Ctrl+C to stop.")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

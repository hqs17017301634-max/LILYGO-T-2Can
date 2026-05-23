#!/usr/bin/env python3
"""Refresh the OTA build timestamp shown in the embedded dashboard UI."""
from __future__ import annotations

import re
import subprocess
import sys
from datetime import datetime, timezone, timedelta
from pathlib import Path

ROOT = Path(globals().get("__file__", ".")).resolve()
if ROOT.is_file():
    ROOT = ROOT.parent.parent
else:
    ROOT = Path.cwd().resolve()
SRC = ROOT / "include" / "web" / "mcp2515_dashboard_ui.src.h"
MINIFY = ROOT / "scripts" / "minify_dashboard.py"
VERSION_FILE = ROOT / "VERSION"

TZ_SHANGHAI = timezone(timedelta(hours=8))


def js_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace("'", "\\'").replace("\n", "\\n")


def unicode_escape(value: str) -> str:
    return value.encode("unicode_escape").decode("ascii")


def main() -> int:
    version = VERSION_FILE.read_text(encoding="utf-8").strip() if VERSION_FILE.exists() else "unknown"
    timestamp = datetime.now(TZ_SHANGHAI).strftime("%Y-%m-%d %H:%M:%S +08:00")
    stamp_text = f"Version: {version}\nOTA timestamp: {timestamp}"
    stamp_zh = f"版本：{version}\nOTA 时间：{timestamp}"

    text = SRC.read_text(encoding="utf-8")

    text = re.sub(
        r"Version: [^\r\n<]+\r?\nOTA(?: test)? timestamp: [^<]+(?=</div>)",
        stamp_text,
        text,
        count=1,
    )
    text = re.sub(
        r'(<div class="modal-msg" id="ota-test-msg">)Version: [^\r\n<]+\r?\nOTA(?: test)? timestamp: [^<]+(</div>)',
        lambda m: m.group(1) + stamp_text + m.group(2),
        text,
        count=1,
    )
    i18n_entry = f"'{js_string(stamp_text)}':'{unicode_escape(stamp_zh)}',"
    text = re.sub(
        r"'Version: [^']+?\\nOTA(?: test)? timestamp: [^']+?':'[^']*?',",
        lambda _m: i18n_entry,
        text,
        count=1,
    )

    SRC.write_text(text, encoding="utf-8")
    print(f"OTA timestamp: {timestamp}")

    proc = subprocess.run([sys.executable, str(MINIFY)], cwd=str(ROOT))
    return proc.returncode


if __name__ == "__main__":
    raise SystemExit(main())

_result = main()
if _result:
    raise SystemExit(_result)

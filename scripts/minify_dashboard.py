#!/usr/bin/env python3
"""Minify + gzip embedded dashboard HTML in mcp2515_dashboard_ui.h.

Reads source HTML from index.html in the repo root, minifies HTML/CSS/JS,
gzips the result, and writes mcp2515_dashboard_ui.h with both:
  - DASH_HTML[]   (raw minified HTML, for native builds and debugging)
  - DASH_HTML_GZ[] / DASH_HTML_GZ_LEN  (gzip-compressed for HTTP send)
"""
import gzip
import re
import shutil
import subprocess
import sys
from pathlib import Path

import rjsmin
import csscompressor
import htmlmin

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "include" / "web" / "mcp2515_dashboard_ui.src.h"
DST = ROOT / "include" / "web" / "mcp2515_dashboard_ui.h"


def terser_minify(code: str) -> str:
    terser = shutil.which("terser")
    if not terser:
        return rjsmin.jsmin(code)
    proc = subprocess.run(
        [terser, "--compress", "--ecma", "2020"],
        input=code,
        text=True,
        capture_output=True,
    )
    if proc.returncode != 0:
        print(f"warn: terser failed: {proc.stderr}", file=sys.stderr)
        return rjsmin.jsmin(code)
    return proc.stdout


def minify_blocks(html: str, tag: str, fn) -> str:
    pattern = re.compile(rf"(<{tag}\b[^>]*>)(.*?)(</{tag}>)", re.DOTALL | re.IGNORECASE)

    def repl(match):
        try:
            return match.group(1) + fn(match.group(2)) + match.group(3)
        except Exception as exc:
            print(f"warn: {tag} minify failed: {exc}", file=sys.stderr)
            return match.group(0)

    return pattern.sub(repl, html)


text = SRC.read_text(encoding="utf-8") if SRC.exists() else DST.read_text(encoding="utf-8")
m = re.search(r'R"HTML\((.*)\)HTML";', text, re.DOTALL)
if not m:
    print("no HTML payload found", file=sys.stderr)
    sys.exit(1)

html = m.group(1)
before = len(html)

html = minify_blocks(html, "style", csscompressor.compress)
html = minify_blocks(html, "script", terser_minify)
html = htmlmin.minify(
    html,
    remove_comments=True,
    remove_empty_space=True,
    remove_all_empty_space=True,
    reduce_boolean_attributes=True,
    keep_pre=True,
)

raw_len = len(html)
gz = gzip.compress(html.encode("utf-8"), compresslevel=9, mtime=0)
gz_len = len(gz)


def hex_array(data: bytes, width: int = 16) -> str:
    out = []
    for i in range(0, len(data), width):
        chunk = data[i : i + width]
        out.append(",".join(f"0x{b:02x}" for b in chunk))
    return ",\n    ".join(out)


body = []
body.append("#pragma once")
body.append("#ifdef ESP_PLATFORM")
body.append('#include "platform/espidf_runtime.h"')
body.append("#else")
body.append("#include <Arduino.h>")
body.append("#endif")
body.append("#include <stddef.h>")
body.append("#include <stdint.h>")
body.append("")
body.append("#ifndef ESP_PLATFORM")
body.append('static const char DASH_HTML[] PROGMEM = R"HTML(' + html + ')HTML";')
body.append("#endif")
body.append("")
body.append("static const uint8_t DASH_HTML_GZ[] PROGMEM = {")
body.append("    " + hex_array(gz))
body.append("};")
body.append(f"static constexpr size_t DASH_HTML_GZ_LEN = {gz_len};")
body.append("")

DST.write_text("\n".join(body), encoding="utf-8")

print(
    f"html: {before} -> {raw_len} bytes minified ({100 * raw_len / before:.1f}%)"
)
print(
    f"gzip: {raw_len} -> {gz_len} bytes ({100 * gz_len / raw_len:.1f}% of minified, "
    f"{100 * gz_len / before:.1f}% of original)"
)

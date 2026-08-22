#!/usr/bin/env python3
"""Generate a LuaLS/EmmyLua library stub for the firmware's ``rf`` API.

The public function names are read from LuaEngine.cpp. Signatures and prose live
here because the C API registration itself does not contain that information.
Generation fails when either side gets out of sync.
"""

from __future__ import annotations

import argparse
import difflib
import re
from dataclasses import dataclass
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / "src/services/LuaEngine.cpp"
DEFAULT_OUTPUT = ROOT / "tools/generated/rfsuite.lua"


@dataclass(frozen=True)
class Function:
    name: str
    params: tuple[tuple[str, str, str | None], ...] = ()
    returns: tuple[tuple[str, str | None], ...] = ()
    description: str = ""


FUNCTIONS = (
    Function("millis", returns=(("integer", "Milliseconds since boot."),), description="Return the device uptime."),
    Function("peak_channel", returns=(("RfChannel", "Current peak channel."),), description="Return the strongest channel in the latest scan."),
    Function("level", (("channel", "RfChannel", None),), (("integer", "Relative activity from 0 to 100."),), "Read the latest activity for one channel."),
    Function("log", (("message", "string", None),), description="Append a timestamped message to the Lua log on the SD card."),
    Function("spectrum", returns=(("integer[]", "126 values; index 1 represents RF channel 0."),), description="Return a snapshot of all channel activity values."),
    Function("status", returns=(("RfStatus", None),), description="Return current analyzer and recorder status."),
    Function("set_cursor", (("channel", "RfChannel", None),), description="Move the analyzer cursor."),
    Function("freeze", (("frozen", "boolean", None),), description="Freeze or resume analyzer acquisition."),
    Function("set_band", (("band", "RfBand", None),), description="Select the analyzer scan band."),
    Function("set_trace", (("trace", "RfTrace", None),), description="Select the analyzer trace mode."),
    Function("capture_baseline", description="Capture the current levels as the DELTA baseline."),
    Function("clear_max", description="Clear the maximum trace history."),
    Function("toggle_watch", (("channel", "RfChannel", None),), description="Toggle a watched-channel marker."),
    Function("recording", (("start", "boolean", None),), (("boolean", "True when the requested operation succeeded."),), "Start a new recording session or stop recording."),
    Function("environment", (("start", "boolean", None),), (("boolean", "True when the requested operation succeeded."),), "Start or stop passive occupancy analysis."),
    Function("open_screen", (("screen", "RfScreen", None),), description="Choose the TFT screen shown after the script exits."),
    Function("gui_begin", (("title", "string", "\"LUA GUI\""),), description="Open and clear the protected 152 x 86 Lua canvas."),
    Function("gui_footer", (("left", "string", "\"\""), ("middle", "string", "\"\""), ("right", "string", "\"\"")), description="Set the three firmware footer labels."),
    Function("gui_clear", description="Clear the Lua canvas without overwriting its firmware frame."),
    Function("gui_text", (("x", "integer", None), ("y", "integer", None), ("text", "string", None), ("color", "RfColor", "\"white\"")), description="Draw clipped single-line text."),
    Function("gui_pixel", (("x", "integer", None), ("y", "integer", None), ("color", "RfColor", "\"white\"")), description="Draw one clipped pixel."),
    Function("gui_line", (("x0", "integer", None), ("y0", "integer", None), ("x1", "integer", None), ("y1", "integer", None), ("color", "RfColor", "\"white\"")), description="Draw a clipped line."),
    Function("gui_rect", (("x", "integer", None), ("y", "integer", None), ("width", "integer", None), ("height", "integer", None), ("color", "RfColor", "\"white\""), ("filled", "boolean", "false")), description="Draw an outline or filled rectangle."),
    Function("gui_circle", (("x", "integer", None), ("y", "integer", None), ("radius", "integer", None), ("color", "RfColor", "\"white\""), ("filled", "boolean", "false")), description="Draw an outline or filled circle."),
    Function("button", (("button", "RfButton", None),), (("boolean", "True while the active-low button is pressed."),), "Read a hardware button during a script."),
    Function("delay", (("milliseconds", "integer", None),), description="Wait 0 to 1000 ms while feeding the watchdog."),
    Function("lab_start", (("target", "string", None),), (("boolean", "True when transmission started."),), "Start an authorized-lab RF target; unavailable in analyzer builds."),
    Function("lab_stop", description="Stop all controlled-lab RF activity."),
)


def registered_functions(source: str) -> set[str]:
    pattern = re.compile(
        r"lua_pushcfunction\(state,\s*\w+\);\s*"
        r'lua_setfield\(state,\s*-2,\s*"([a-z0-9_]+)"\);'
    )
    return set(pattern.findall(source))


def validate(source_path: Path) -> None:
    actual = registered_functions(source_path.read_text(encoding="utf-8"))
    documented = {function.name for function in FUNCTIONS}
    missing = sorted(actual - documented)
    stale = sorted(documented - actual)
    if missing or stale:
        details = []
        if missing:
            details.append("missing metadata: " + ", ".join(missing))
        if stale:
            details.append("not registered by firmware: " + ", ".join(stale))
        raise ValueError("Lua API metadata is out of sync (" + "; ".join(details) + ")")


def generate() -> str:
    lines = [
        "---@meta RFSuite",
        "-- Generated by tools/generate_lua_library.py; do not edit manually.",
        "-- This file is for editor completion/type checking and is not copied to the SD card.",
        "",
        "---@alias RfChannel integer # RF24 channel 0..125.",
        '---@alias RfBand \"all\"|\"wifi\"|\"bt\"',
        '---@alias RfTrace \"live\"|\"avg\"|\"max\"|\"delta\"',
        '---@alias RfScreen \"spectrum\"|\"waterfall\"|\"inspect\"|\"survey\"|\"events\"|\"logging\"|\"status\"|\"menu\"',
        '---@alias RfButton \"up\"|\"down\"|\"a\"|\"b\"|\"right\"|\"left\"',
        '---@alias RfColor \"white\"|\"black\"|\"gray\"|\"accent\"|\"cyan\"|\"green\"|\"yellow\"|\"orange\"|\"red\"',
        "",
        "---@class RfStatus",
        "---@field peak_channel RfChannel",
        "---@field peak_level integer",
        "---@field confidence integer",
        "---@field cursor RfChannel",
        "---@field sweeps integer",
        "---@field radios integer",
        "---@field frozen boolean",
        "---@field logging boolean",
        "---@field environment_running boolean",
        "",
        "---@class RfApi",
        "---@type RfApi",
        "rf = {}",
    ]
    for function in FUNCTIONS:
        lines.extend(("", "---" + function.description))
        for name, lua_type, default in function.params:
            optional = "?" if default is not None else ""
            suffix = f" Default: `{default}`." if default is not None else ""
            lines.append(f"---@param {name}{optional} {lua_type}{suffix}")
        for lua_type, description in function.returns:
            suffix = f" {description}" if description else ""
            lines.append(f"---@return {lua_type}{suffix}")
        arguments = ", ".join(name for name, _, _ in function.params)
        lines.extend((f"function rf.{function.name}({arguments}) end",))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE, help="path to LuaEngine.cpp")
    parser.add_argument("--output", "-o", type=Path, default=DEFAULT_OUTPUT, help="generated Lua library path")
    parser.add_argument("--check", action="store_true", help="fail if the output is absent or stale")
    args = parser.parse_args()

    try:
        validate(args.source)
    except (OSError, ValueError) as error:
        parser.error(str(error))

    content = generate()
    if args.check:
        current = args.output.read_text(encoding="utf-8") if args.output.exists() else ""
        if current != content:
            diff = difflib.unified_diff(
                current.splitlines(), content.splitlines(),
                fromfile=str(args.output), tofile="generated", lineterm=""
            )
            print("\n".join(diff), file=sys.stderr)
            print("Lua library is stale; run tools/generate_lua_library.py", file=sys.stderr)
            return 1
        print(f"Lua library is up to date: {args.output}")
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(content, encoding="utf-8")
    print(f"Generated {args.output} ({len(FUNCTIONS)} functions)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

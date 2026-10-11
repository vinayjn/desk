#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
arduino_cli="${ARDUINO_CLI:-arduino-cli}"
if ! command -v "$arduino_cli" >/dev/null 2>&1; then
  arduino_cli="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
fi
if [[ ! -x "$arduino_cli" ]] && ! command -v "$arduino_cli" >/dev/null 2>&1; then
  echo "Install arduino-cli or set ARDUINO_CLI to its path." >&2
  exit 1
fi

fqbn="${FQBN:-esp32:esp32:esp32c3:CDCOnBoot=cdc}"
build_dir="${DESK_BUILD_DIR:-${TMPDIR:-/tmp}/desk-arduino-build}"
"$arduino_cli" compile --fqbn "$fqbn" --build-path "$build_dir" "$project_root"

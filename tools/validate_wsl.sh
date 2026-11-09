#!/usr/bin/env bash
# Use the dedicated local Linux toolchain cache; compile/test only, never flash.
set -euo pipefail
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_dir"
export PATH="$repo_dir/build/toolchain/bin:$PATH"
export ARDUINO_DIRECTORIES_DATA="$HOME/.cache/apex-embedded-toolchain/data"
export ARDUINO_DIRECTORIES_DOWNLOADS="$repo_dir/build/toolchain/downloads"
export ARDUINO_DIRECTORIES_USER="$HOME/.cache/apex-embedded-toolchain/user"
python3 tools/validate_software.py --firmware

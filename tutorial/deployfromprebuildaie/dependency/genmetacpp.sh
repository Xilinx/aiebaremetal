#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

usage() {
  echo "Usage: $0 [path/to/aie_control_config.json]"
  echo "If not provided, defaults to:"
  echo "  \$SCRIPT_DIR/../../../example/example_oob/Work/ps/c_rts/aie_control_config.json"
  exit 1
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DEFAULT_JSON="$SCRIPT_DIR/../../../example/example_oob/Work/ps/c_rts/aie_control_config.json"
JSON_PATH="${1:-$DEFAULT_JSON}"

PY_SCRIPT="$SCRIPT_DIR/../../../script/aiemetacpp.py"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Error: python3 not found in PATH." >&2
  exit 1
fi

if [[ ! -f "$PY_SCRIPT" ]]; then
  echo "Error: Python script not found at: $PY_SCRIPT" >&2
  exit 1
fi

if [[ ! -f "$JSON_PATH" ]]; then
  echo "Error: JSON config not found at: $JSON_PATH" >&2
  echo "Tip: pass a path explicitly or verify the default path exists." >&2
  exit 1
fi

python3 "$PY_SCRIPT" "$JSON_PATH"

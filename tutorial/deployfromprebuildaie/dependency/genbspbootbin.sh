#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

usage() {
  echo "Usage: $0 [work_path] [bif_file]"
  echo "  work_path: Path to link as ./Work (default: /a/b/c/Work)"
  echo "  bif_file : BIF filename in script dir OR absolute/relative path to a .bif"
  echo "             (default: Palmyra.bif)"
  echo "Examples:"
  echo "  $0                                  # uses /a/b/c/Work and Palmyra.bif"
  echo "  $0 /proj/my/work                    # uses provided work path and Palmyra.bif"
  echo "  $0 /proj/my/work MyBoard.bif        # uses MyBoard.bif in script dir"
  echo "  $0 /proj/my/work /path/to/file.bif  # uses explicit bif path"
  exit 1
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_WORK="$SCRIPT_DIR/../../../example/example_oob/Work/"

WORK_TARGET="${1:-$DEFAULT_WORK}"
#BIF_ARG="${2:-Palmyra.bif}"
BIF_ARG="${2:-vek385_allgmio.bif}"


# Resolve BIF path:
# - If BIF_ARG contains '/', treat it as a path.
# - Otherwise, look for it in the script directory.
if [[ "$BIF_ARG" == *"/"* ]]; then
  BIF="$BIF_ARG"
else
  BIF="$SCRIPT_DIR/$BIF_ARG"
fi

ARCH="$SCRIPT_DIR/../../../thirdparty/arch/"

PALMYRABIF="$SCRIPT_DIR/../../../script/Palmyra.bif"
VEK385BIF="$SCRIPT_DIR/../../../script/vek385_allgmio.bif"

pushd "$SCRIPT_DIR" >/dev/null

# Create/refresh symlink: ./arch -> thirdparty/arch
ln -sfn "$ARCH" ./arch
# create bif link
ln -sf "$PALMYRABIF" ./Palmyra.bif
ln -sf "$VEK385BIF" ./vek385_allgmio.bif
echo $PALMYRABIF

if [[ ! -f "$BIF" ]]; then
  echo "Error: BIF file not found at: $BIF"
  return
fi
# Create/refresh symlink: ./Work -> WORK_TARGET
# If a non-symlink directory/file named Work exists, fail with a clear message.
if [[ -e "./Work" && ! -L "./Work" ]]; then
  echo "Error: './Work' exists and is not a symlink. Please move or remove it."
  exit 1
fi
ln -sfn "$WORK_TARGET" ./Work
echo "Linked ./Work -> $WORK_TARGET"

# Run bootgen with selected BIF
echo "Running bootgen with BIF: $BIF"
bootgen -arch versal_2ve_2vm -image "$BIF" -o boot.bin -w

popd >/dev/null

echo "Done: boot.bin generated."

#!/usr/bin/env bash
#SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
#BIF=$SCRIPT_DIR/vek280_boot_image.bif
#ARCH=$SCRIPT_DIR/../../../thirdparty/arch/
#pushd $SCRIPT_DIR
#ln -s $ARCH ./arch
#bootgen -arch versal -image ${BIF} -o testkernel.BIN -w
#popd

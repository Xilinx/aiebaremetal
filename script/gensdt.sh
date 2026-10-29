#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
version="${VERSION:-2026.2}"
source "${VITIS_SETTINGS:-/proj/xbuilds/${version}_daily_latest/installs/lin64/${version}/Vitis/settings64.sh}"

set -x

usage() {
	cat <<'EOF'
Usage: gensdt.sh [OPTIONS]

  --chroot PATH   Target chroot/work directory (cleared and recreated)
  --xsa PATH      XSA hardware description file
  --branch NAME   Git branch to checkout (default: master)
  --repo PATH     Path to system-device-tree (or other) git repository
  --origin NAME   Git remote name for checkout (default: origin)
  --board NAME    Board name for sdtgen (e.g. vck190)

  --help, -h      Show this help
EOF
}

CHROOT=""
XSA=""
BRANCH="master"
REPO="/mnt/raid5/botruns/sdtgen-dts/system-device-tree-xlnx"
ORIGIN="origin"
BOARD=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	--chroot)
		if [[ $# -lt 2 ]]; then
			echo "error: --chroot requires a value" >&2
			exit 1
		fi
		CHROOT="$2"
		shift 2
		;;
	--chroot=*)
		CHROOT="${1#*=}"
		shift
		;;
	--xsa)
		if [[ $# -lt 2 ]]; then
			echo "error: --xsa requires a value" >&2
			exit 1
		fi
		XSA="$2"
		shift 2
		;;
	--xsa=*)
		XSA="${1#*=}"
		shift
		;;
	--branch)
		if [[ $# -lt 2 ]]; then
			echo "error: --branch requires a value" >&2
			exit 1
		fi
		BRANCH="$2"
		shift 2
		;;
	--branch=*)
		BRANCH="${1#*=}"
		shift
		;;
	--repo)
		if [[ $# -lt 2 ]]; then
			echo "error: --repo requires a value" >&2
			exit 1
		fi
		REPO="$2"
		shift 2
		;;
	--repo=*)
		REPO="${1#*=}"
		shift
		;;
	--origin)
		if [[ $# -lt 2 ]]; then
			echo "error: --origin requires a value" >&2
			exit 1
		fi
		ORIGIN="$2"
		shift 2
		;;
	--origin=*)
		ORIGIN="${1#*=}"
		shift
		;;
	--board)
		if [[ $# -lt 2 ]]; then
			echo "error: --board requires a value" >&2
			exit 1
		fi
		BOARD="$2"
		shift 2
		;;
	--board=*)
		BOARD="${1#*=}"
		shift
		;;
	--help|-h)
		usage
		exit 0
		;;
	*)
		echo "error: unknown option: $1" >&2
		exit 1
		;;
	esac
done

missing=()
[[ -z "$CHROOT" ]] && missing+=(--chroot)
[[ -z "$XSA" ]] && missing+=(--xsa)
[[ -z "$BRANCH" ]] && missing+=(--branch)
[[ -z "$REPO" ]] && missing+=(--repo)
[[ -z "$ORIGIN" ]] && missing+=(--origin)
[[ -z "$BOARD" ]] && missing+=(--board)
if ((${#missing[@]})); then
	echo "error: required option(s) missing: ${missing[*]}" >&2
	echo "Try $(basename "$0") --help" >&2
	exit 1
fi

CHROOT=$(realpath "$CHROOT")
XSA=$(realpath "$XSA")
REPO=$(realpath "$REPO")

mkdir -p "$CHROOT"

pushd "$REPO"
git remote update
git checkout "$ORIGIN/$BRANCH"
pushd # $REPO

pushd "$CHROOT"
DIRNAME="$(basename "$CHROOT")"
echo DIRNAME="$DIRNAME"

TODAY=$(date +%Y-%m-%d)
TARGET_DIR="${DIRNAME}_${TODAY}"
if [ -d "$TARGET_DIR" ]; then
	rm -rf "$TARGET_DIR"
	echo "Deleted existing folder: $TARGET_DIR"
fi
mkdir -p "$TARGET_DIR"
echo "Created folder: $TARGET_DIR"
TARGET_DIR=$(realpath "$TARGET_DIR")
FOLDER_COUNT=$(ls -d $DIRNAME_* 2>/dev/null | wc -l)
if [ "$FOLDER_COUNT" -gt 365 ]; then
	ls -d $DIRNAME_* 2>/dev/null | sort -V | head -n -3 | while read dir; do
	rm -rf "$dir"
	echo "Deleted old folder: $dir"
done
fi

pushd "$TARGET_DIR"
#xsct -eval "sdtgen set_dt_param -xsa $XSA; sdtgen set_dt_param -dir $TARGET_DIR; sdtgen set_dt_param -repo $REPO; sdtgen set_dt_param -board_dts $BOARD; sdtgen generate_sdt"
rm sdtgen.tcl

echo "sdtgen set_dt_param -xsa $XSA" >> sdtgen.tcl
echo "sdtgen set_dt_param -dir $TARGET_DIR" >> sdtgen.tcl
#echo "sdtgen set_dt_param -repo $REPO" >> sdtgen.tcl
echo "sdtgen set_dt_param -board_dts $BOARD" >> sdtgen.tcl
echo "sdtgen generate_sdt" >> sdtgen.tcl

sdtgen sdtgen.tcl

gcc -I . -E -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o system-top.dts.post system-top.dts
dtc -I dts -O dtb -o system-top.post.dtb system-top.dts.post
popd # $TARGET_DIR

rm latest -f
ln -s ${TARGET_DIR} latest
popd # $CHROOT

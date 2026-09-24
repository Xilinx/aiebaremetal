#!/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

SETTINGS_SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")

source "${SETTINGS_SCRIPT_DIR}/envaie2pst50.sh"


: "${VERSION2:=0}"
if [ "${VERSION2}" -eq 1 ]; then
	VERSION="hub"
	WEBSITE=""
else
	VERSION="enterprise."
	WEBSITE="xilinx"
fi
export GITBASE="${GITBASE:-https://git${VERSION}${WEBSITE}.com}"
export CERT_REPO="${CERT_REPO:-TurinAI/cert}"
export XTF_REPO="${XTF_REPO:-regressions/xtf}"
export EMBEDDEDSW_REPO="${EMBEDDEDSW_REPO:-embeddedsw/embeddedsw}"
export AIERT_REPO="${AIERT_REPO:-ai-engine/aie-rt}"
export AIERT_COMMIT="${AIERT_COMMIT:-7e331051439034265511eb442956bb1b082d7494}"

COMPILE_TYPE=0
: "${CERT_GIT_REPO:=}"
: "${XTF_GIT_REPO:=}"
: "${EMBEDDEDSW_GIT_REPO:=}"
: "${AIERT_GIT_REPO:=}"
export CERT_GIT_REPO XTF_GIT_REPO EMBEDDEDSW_GIT_REPO AIERT_GIT_REPO
if [ $# -gt 0 ]; then
	COMPILE_TYPE=$1
fi

if [ -n "${GITBASE:-}" ]; then
	_gitbase="${GITBASE%/}"
	if [ -z "$CERT_GIT_REPO" ] && [ -n "${CERT_REPO:-}" ]; then
		CERT_GIT_REPO="${_gitbase}/${CERT_REPO#/}"
	fi
	if [ -z "$XTF_GIT_REPO" ] && [ -n "${XTF_REPO:-}" ]; then
		XTF_GIT_REPO="${_gitbase}/${XTF_REPO#/}"
	fi
	if [ -z "$EMBEDDEDSW_GIT_REPO" ] && [ -n "${EMBEDDEDSW_REPO:-}" ]; then
		EMBEDDEDSW_GIT_REPO="${_gitbase}/${EMBEDDEDSW_REPO#/}"
	fi
	if [ -z "$AIERT_GIT_REPO" ] && [ -n "${AIERT_REPO:-}" ]; then
		AIERT_GIT_REPO="${_gitbase}/${AIERT_REPO#/}"
	fi
	unset _gitbase
	_clone_if_missing_or_empty() {
		local repo="$1"
		local dest="$2"
		if [ ! -d "$dest" ] || [ -z "$(ls -A "$dest" 2>/dev/null)" ]; then
			git clone --recursive "$repo" "$dest" || true
		fi
	}
	_pin_repo_commit_if_set() {
		local commit="$1"
		local dest="$2"
		if [ -n "$commit" ] && [ -d "$dest/.git" ]; then
			git -C "$dest" fetch origin "$commit" || true
			git -C "$dest" checkout "$commit" || true
		fi
	}
	pushd $SETTINGS_SCRIPT_DIR/../thirdparty
	_clone_if_missing_or_empty "$CERT_GIT_REPO" cert
	_clone_if_missing_or_empty "$XTF_GIT_REPO" xtf
	_clone_if_missing_or_empty "$EMBEDDEDSW_GIT_REPO" embeddedsw
	pushd aielib
	_clone_if_missing_or_empty "$AIERT_GIT_REPO" aie-rt
	# _pin_repo_commit_if_set "$AIERT_COMMIT" aie-rt
	popd
	popd
fi

echo ${COMPILE_TYPE}


echo "The settings script is located in: $SETTINGS_SCRIPT_DIR"

export PATH=$PATH:$SETTINGS_SCRIPT_DIR
export SCRIPT_DIR="$SETTINGS_SCRIPT_DIR"

if [ ${COMPILE_TYPE} -eq  0 ]; then
	pushd .
	if test -d "$TMPDIR"; then
  	  :
	elif test -d "$TMP"; then
  	  TMPDIR=$TMP
	elif test -d "$TEMP"; then
  	  TMPDIR=$TEMP
	elif test -d /var/tmp; then
  	  TMPDIR=/var/tmp
	else
  	  TMPDIR=/tmp
	fi

	foldername=baremetal_runtime_$(date +%Y%m%d%H%M%S)
	cd $TMPDIR
	mkdir $foldername
	cd $foldername
	git clone --recursive https://${GITBASE}.${DOMAIN}/ai-engine/aiebaremetal.git

	export BAREMETAL_DIR=$(pwd)/aiebaremetal
	export PATH=$BAREMETAL_DIR/script/:$PATH
	popd

else
	export BAREMETAL_DIR=$SETTINGS_SCRIPT_DIR/../
fi

export LM_LICENSE_FILE=1757@xsjlicsrvip
export RDI_INTERNAL_ALLOW_PARTIAL_DATA=yes
#source /proj/xbuilds/2024.1_INT_qualified_latest/installs/lin64/Vitis/2024.1/settings64.sh
# For external buffer latest compiler is needed
#source /proj/xbuilds/HEAD_daily_latest/installs/lin64/Vitis/HEAD/settings64.sh
source "${SETTINGS_SCRIPT_DIR}/adftoaeg_setup.sh"

if [[ -n "${XILINX_VITIS:-}" ]]; then
	export XILINX_VITIS_AIETOOLS="${XILINX_VITIS}/aietools"
fi


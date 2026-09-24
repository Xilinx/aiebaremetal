#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
EXAMPLE_KERNELPLIO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

fail() {
	echo "ERROR: $*" >&2
	if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
		return 1
	fi
	exit 1
}

AIE_GEN=1
FORCEBUILDLIB=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ]; then
	FORCEBUILDLIB=$2
fi

echo "${AIE_GEN}"
(
	cd "${EXAMPLE_KERNELPLIO_DIR}" || exit 1
	bash -c "source ../../script/envaie2pst50.sh; make compile CFLAGS='-rdynamic' AIE_GEN=${AIE_GEN} -j32"
) || fail "make compile failed"

pushd "${EXAMPLE_KERNELPLIO_DIR}" || fail "cannot cd to ${EXAMPLE_KERNELPLIO_DIR}"
cd ../
#default rebuild libaiebaremetal.so
if [[ ${FORCEBUILDLIB} -eq 1 ]]; then
	make targetforce "AIE_KERNEL_LOC=${EXAMPLE_KERNELPLIO_DIR}" "AIE_GEN=${AIE_GEN}" -j32
fi

popd


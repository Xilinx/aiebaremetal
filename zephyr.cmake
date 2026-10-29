# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

function(add_aiebaremetal_library TARGET_NAME)
  if(NOT DEFINED AIEBAREMETAL_SRC_DIR OR "${AIEBAREMETAL_SRC_DIR}" STREQUAL "")
    message(FATAL_ERROR "AIEBAREMETAL_SRC_DIR is not set. Please set AIEBAREMETAL_SRC_DIR to the path of the aie-rt source directory.")
  endif()

  target_sources(${TARGET_NAME} PRIVATE
    ${AIEBAREMETAL_SRC_DIR}/src/aeg_aiebaremetal.cc
    ${AIEBAREMETAL_SRC_DIR}/src/aeg_configure.cc
    ${AIEBAREMETAL_SRC_DIR}/src/cert_elf.cc
    ${AIEBAREMETAL_SRC_DIR}/src/cert_elf_loader.cc
    ${AIEBAREMETAL_SRC_DIR}/src/cert_host.cc
    ${AIEBAREMETAL_SRC_DIR}/src/ctrl_code_elf.cc
    ${AIEBAREMETAL_SRC_DIR}/src/common_layer/aeg_api_message.cpp
    ${AIEBAREMETAL_SRC_DIR}/src/common_layer/aeg_runtime_api.cpp
    ${AIEBAREMETAL_SRC_DIR}/src/common_layer/aegothers/aeg_profiling_api.cpp
    ${AIEBAREMETAL_SRC_DIR}/src/common_layer/aegothers/fal_util.cpp
    ${AIEBAREMETAL_SRC_DIR}/example/build/baremetal_metadata.cpp
  )

  target_include_directories(${TARGET_NAME} PUBLIC
    ${AIEBAREMETAL_SRC_DIR}/src/
    ${AIEBAREMETAL_SRC_DIR}/src/include/
    ${AIEBAREMETAL_SRC_DIR}/src/common_layer/
    ${AIEBAREMETAL_SRC_DIR}/src/common/
    ${AIEBAREMETAL_SRC_DIR}/src/include/common/
    ${AIEBAREMETAL_SRC_DIR}/src/include/common_layer/
    ${AIEBAREMETAL_SRC_DIR}/src/include/common_layer/aegothers/
    ${AIEBAREMETAL_SRC_DIR}/src/include/elfio/
    #    ${AIEBAREMETAL_SRC_DIR}/src/include/log/
    #${AIEBAREMETAL_SRC_DIR}/src/include/vitis_aie_compat/
    #${AIEBAREMETAL_SRC_DIR}/src/include/vitis_aie_compat/xaiengine/
    ${AIEBAREMETAL_SRC_DIR}/thirdparty/aielib/include/xaiefal/
    ${AIEBAREMETAL_SRC_DIR}/thirdparty/aielib/include/

  )
endfunction()

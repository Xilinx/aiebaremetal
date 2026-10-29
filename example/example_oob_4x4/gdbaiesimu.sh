# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
sudo gdb attach $(pgrep -fl simmsm_dbg | grep -v loader | awk '{print $1}')

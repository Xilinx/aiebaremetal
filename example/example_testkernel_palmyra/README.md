<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->


"""

This is a test case for Palmyra.
Palmyra pdi (with GMIO enabled from col 0 to 11) is located in  thirdparty/arch/platform/pdi/palmyra/


BIF file combines HW PDI and CDOs to generate final pdi, which be loaded in the Palmyra HW.
copy arch folder to test folder and run below command to generate Palmyra.pdi that contains HW pdi and CDOs.

bootgen -arch versal_2ve_2vm -image Palmyra.bif -w -o Palmyra.pdi

Use Palmyra.pdi and palmyra.elf to run the test on hardware.
For detailed instruction please refer https://confluence.amd.com/display/XPS/How+to+run+Baremetal+test+on+Palmyra

"""

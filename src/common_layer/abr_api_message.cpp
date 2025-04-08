/**
* Copyright (C) 2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
// Original code by AMD Xilinx
// This copy is maintained by the SSW AIE Team
// Modifications by SSW AIE Team on 2023-12-30
// Description: Remove the error.h header file

/* 
 * Changes made:
 * -SSW AIE Team, 2023-12-30
 *   - Remove the error.h header file.
 */
#include "abr_api_message.h"
#include <iostream>
//#include "core/common/error.h"

abr::err_code errorMsg(abr::err_code code, std::string msg)
{
    //throw xrt_core::error(-((int)code), msg);
    return code;
}

void debugMsg(std::string msg)
{
}

void infoMsg(std::string msg)
{
	std::cout << msg << std::endl;
}

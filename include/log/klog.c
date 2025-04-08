/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
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

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#define _AIE_LOG_LOCAL_TEST_
#include "klog.h"
#include "klogr.h"
int main() {
	char log[256];
	void *address = (void *)log;
	int run_round = 0;
	for(int i = 0; i< 1024; i++) {
		wlog(0,"2******mmstart 1 round = %d \n", ++run_round);
	}
	//rlog(0,0,0);
	wlog_reset();
	wlog(0,"0----This is a test %d", 2);
	wlog(0,"0----This is a test %f", 3.14);

	char * s = rlog(0,0,0);
	printf("%s\n",s);

	return 0;
}

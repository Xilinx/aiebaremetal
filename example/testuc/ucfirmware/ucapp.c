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
#ifdef _UCFW_
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_io.h"

#include <mb_interface.h>

#include <stdlib.h>
#include "ucheader.h"

#define ENABLE_UART
#ifdef ENABLE_UART
#   define TRACE(...)   xil_printf(__VA_ARGS__)
#else
#   define TRACE(...)
#endif


void (*fptr)(void) = 0;


int main()
{
    init_platform();

    TRACE( "c- Hello World from Microblaze/UC !\n" );
    //the share or module data mem
    volatile int *mem = (volatile int*)0x20000;
    *mem = MAGIC_DATA;
    for ( int i = 0; i < 10; i ++ ) {
    //  TRACE( "Read mem address 0x%x data=0x%x\n", (int)(mem+i), *(mem+i) );
    }

		uint col = 12, row = 3;
		uint addr = 0x80000000 | col << 25 | row << 20 | 0x00038000;
		uint* ptr = (uint *)addr;
		//*ptr = 1;   // Set Enable bit to 1


    TRACE( "Microblaze enter sleep\n" );    

/*
    uint32_t result = 0;

    for (int i = 0; i < 5; ++i) {
        uint32_t stream_word;
        getfsl(stream_word, 0);
    	result += stream_word;
    }

    TRACE("stream sum = %u\n", result);

    putfsl(result, 0);

    // Put once more with TLAST
    cputfsl(result, 0);

    volatile uint32_t *array_addr = 0x84000000;
    *array_addr = 0x3F;

    TRACE("array read = %u\n", *array_addr);
    cputfsl(*array_addr, 0);
*/

    cleanup_platform();

    mb_sleep();

        TRACE( "Wake up again!\n" );

	fptr();
    return 0;
}
#else
#endif

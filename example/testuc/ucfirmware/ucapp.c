/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * ucapp.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
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

		int col = 12, row = 3;
		int addr = 0x80000000 | col << 25 | row << 20 | 0x00038000;
		int* ptr = (int *)addr;
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

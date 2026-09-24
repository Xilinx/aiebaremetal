<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

"""
    Main goal of this application is to read radar data and produce range and velocity. Kernel sequence looks like this - FFT kernel with point size 2048 produces range, then AIE custom kernel to convert cint16 data to cint32, transpose the output 4 dimensional matrix and then run FFT again with point size 64(i.e group size). There are couple of things to notice here. 
    
    range_fft_output [i] [j] [k] = FFT ( chirp [i] [j] [k] )
    
    Where 
    for ( i = 1 to G )   // G: No of groups i.e. no of chirps transmitted by each TX antenna in a radar cycle
    for ( j = 1 to TX )   // TX: No of TX antennas
 
    for ( k = 1 to RX )   // RX: No of RX antennas
    Now if there are L groups of output then FFT output looks like  
    range_fft_output [G] [TX] [RX] [L]
    
    The output of Range FFT from a radar cycle is then transposed over “Groups” and “Samples” dimensions and then another FFT is performed. The output of this FFT represents the velocity of the objects detected by radar in form of peaks in the output plot. Hence, this FFT is called “Velocity” or “Doppler” FFT and its point size is same as number of groups.
    
    For this design the specification used is as follows
    
    
    
    Memory requirement is too huge to fit into AIE array and also range FFT's output needs to be transposed before feeding it to velocity FFT. To solve these issues, they are using Shared Memory available in Mem tiles in AIE-ML array on Versal AI Edge series devices. Basically, they divide the transpose operation of 4D cube into multiple intermediate transposes done using smaller buffer implemented using Mem tiles. They use two Mem tiles, one between Range FFT and DDR, and second between DDR and Velocity FFT
    
    To support this kind of memory requirement and meet the throughput demand, they store and move data in a certain way and therefore mm2s PL kernel is not simply copying the data linearly.
    The mm2s data mover comprises three loops, first one to control iteration of the mm2s kernel , second to loop over the groups dimension, third for the coverage of rx channels and tx channels. To reduce input data storage in DDR, they are only storing unique chirps for 64 Groups i.e. input data buffer in DDR is [64][2048]. mm2s data mover takes these unique chirp sequence for each Group and generate chirps for TX and RX channel iterations by rotating the chirp samples left by 4 samples.
    
    Based on some experiments done in baremetal runtime systemc support, there are few things that makes it difficult to port this example as it is at this point of time 
    
        a. Custom PL kernel support.
        b. Systemc is not able to handle a FFT kernel with point size 2048. Input data is accordingly organized.
        c. Number of memtiles available in systemc is not same as actual hardware.
        
    This example should be ported as it is when we have fully functional baremetal runtime support. As of now, we will have two simple DSP use cases. They are 
        a. Range_FFT with reduced point size followed by cint16_2_cint32 kernel. Verify adf runtime and baremetal runtime output matches.
        b. Another example using two dsp filters.

 Steps to run the application

    source /proj/xbuilds/2024.1_INT_qualified_latest/installs/lin64/Vitis/2024.1/settings64.sh
    # Inside example_auto_radar_2dfft directory
    source download_dsplib.sh
    source build.sh 2
    source modify_libadf
    cd script
    source builda72.sh

Confluence page : https://confluence.amd.com/pages/viewpage.action?spaceKey=~sangejha&title=Auto+Radar+2d+FFT+application+on+Baremetal+runtime


"""

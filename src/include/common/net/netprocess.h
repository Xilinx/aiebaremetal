/* <!--
 * Copyright (C) 2009 - 2022 Xilinx, Inc.
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 * -->
 */
#ifndef __NET_PROCESS__
#define __NET_PROCESS__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

#include "xparameters.h"

#include "netif/xadapter.h"

#include "platform.h"
#include "platform_config.h"
#if defined(__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif

#include "lwip/tcp.h"
#include "xil_cache.h"

#if LWIP_IPV6 == 1
#include "lwip/ip.h"
#else
#if LWIP_DHCP == 1
#include "lwip/dhcp.h"
#endif
#endif
#include "filerecv.h"
/* defined by each RAW mode application */
void tcp_fasttmr(void);
void tcp_slowtmr(void);

/* missing declaration in lwIP */
void lwip_init();

#if LWIP_IPV6 == 0
#if LWIP_DHCP == 1
extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
#endif
#endif

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
static struct netif server_netif;
struct netif *process_netif;
struct netif *echo_netif;

#if LWIP_IPV6 == 1
void print_ip6_v2(char *msg, ip_addr_t *ip) {
    print(msg);
    xil_printf(" %x:%x:%x:%x:%x:%x:%x:%x\n\r", IP6_ADDR_BLOCK1(&ip->u_addr.ip6), IP6_ADDR_BLOCK2(&ip->u_addr.ip6),
               IP6_ADDR_BLOCK3(&ip->u_addr.ip6), IP6_ADDR_BLOCK4(&ip->u_addr.ip6), IP6_ADDR_BLOCK5(&ip->u_addr.ip6),
               IP6_ADDR_BLOCK6(&ip->u_addr.ip6), IP6_ADDR_BLOCK7(&ip->u_addr.ip6), IP6_ADDR_BLOCK8(&ip->u_addr.ip6));
}
#else
void print_ip_v2(char *msg, ip_addr_t *ip) {
    print(msg);
    xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings_v2(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw) {

    print_ip_v2("Board IP: ", ip);
    print_ip_v2("Netmask : ", mask);
    print_ip_v2("Gateway : ", gw);
}
#endif

#if defined(__arm__) && !defined(ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
int ProgramSi5324(void);
int ProgramSfpPhy(void);
#endif
#endif

#ifdef XPS_BOARD_ZCU102
#if defined(XPAR_XIICPS_0_DEVICE_ID) || defined(XPAR_XIICPS_0_BASEADDR)
int IicPhyReset(void);
#endif
#endif

int net_process_main(void *callback) {
#if LWIP_IPV6 == 0
    ip_addr_t ipaddr, netmask, gw;

#endif
    /* the mac address of the board. this should be unique per board */
    unsigned char mac_ethernet_address[] = {0x00, 0x0a, 0x35, 0x00, 0x01, 0x02};

    process_netif = &server_netif;
#if defined(__arm__) && !defined(ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
    ProgramSi5324();
    ProgramSfpPhy();
#endif
#endif

    /* Define this board specific macro in order perform PHY reset on ZCU102 */
#ifdef XPS_BOARD_ZCU102
    if (IicPhyReset()) {
        xil_printf("Error performing PHY reset \n\r");
        return -1;
    }
#endif

    init_platform();

#if LWIP_IPV6 == 0
#if LWIP_DHCP == 1
    ipaddr.addr = 0;
    gw.addr = 0;
    netmask.addr = 0;
#else
    /* initialize IP addresses to be used */
    IP4_ADDR(&ipaddr, 192, 168, 1, 10);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 1, 1);
#endif
#endif

    lwip_init();

#if (LWIP_IPV6 == 0)
    /* Add network interface to the netif_list, and set it as default */
    if (!xemac_add(process_netif, &ipaddr, &netmask, &gw, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
        xil_printf("Error adding N/W interface\n\r");
        return -1;
    }
#else
    /* Add network interface to the netif_list, and set it as default */
    if (!xemac_add(process_netif, NULL, NULL, NULL, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
        xil_printf("Error adding N/W interface\n\r");
        return -1;
    }
    process_netif->ip6_autoconfig_enabled = 1;

    netif_create_ip6_linklocal_address(process_netif, 1);
    netif_ip6_addr_set_state(process_netif, 0, IP6_ADDR_VALID);

    print_ip6_v2("\n\rBoard IPv6 address ", &process_netif->ip6_addr[0].u_addr.ip6);

#endif
    netif_set_default(process_netif);

#ifndef SDT
    /* now enable interrupts */
    platform_enable_interrupts();
#endif

    /* specify that the network if is up */
    netif_set_up(process_netif);

#if (LWIP_IPV6 == 0)
#if (LWIP_DHCP == 1)
    /* Create a new DHCP client for this interface.
     * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
     * the predefined regular intervals after starting the client.
     */
    dhcp_start(process_netif);
    dhcp_timoutcntr = 240;

    while (((process_netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0)) {
        xemacif_input(process_netif);
    }

    if (dhcp_timoutcntr <= 0) {
        if ((process_netif->ip_addr.addr) == 0) {
            xil_printf("DHCP Timeout\r\n");
            xil_printf("Configuring default IP of 192.168.1.10\r\n");
            IP4_ADDR(&(process_netif->ip_addr), 192, 168, 1, 10);
            IP4_ADDR(&(process_netif->netmask), 255, 255, 255, 0);
            IP4_ADDR(&(process_netif->gw), 192, 168, 1, 1);
        }
    }

    ipaddr.addr = process_netif->ip_addr.addr;
    gw.addr = process_netif->gw.addr;
    netmask.addr = process_netif->netmask.addr;
#endif

    print_ip_settings_v2(&ipaddr, &netmask, &gw);

#endif
    /* start the application (web server, rxtest, txtest, etc..) */
    start_network_process((void *)callback);

    /* receive and process packets */
    while (1) {
        if (TcpFastTmrFlag) {
            tcp_fasttmr();
            TcpFastTmrFlag = 0;
        }
        if (TcpSlowTmrFlag) {
            tcp_slowtmr();
            TcpSlowTmrFlag = 0;
        }
        xemacif_input(process_netif);
    }

    /* never reached */
    cleanup_platform();

    return 0;
}
#ifdef __cplusplus
}
#endif
#endif

// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef __FILE_RECV__
#define __FILE_RECV__
#include "lwip/def.h"
#include "lwip/tcp.h"
#include "aeg_log.h"
#include "netcommon.h"
#include <stdint.h>
#include <string.h>
static char *imgdata = NULL;
static int imglen = 0;
typedef struct _file_recv_arg {
    void *state_arg;
    DataCallback func;
} file_recv_arg;
static file_recv_arg frecv_arg = {0, 0};
static err_t file_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    file_transfer_state_t *state = (file_transfer_state_t *)arg;
    if (p == NULL) {
        // Remote side closed connection.
        if (state && (state->bytes_received == state->file_size) && state->file_size) {
            // Finalize MD5 if file transfer completed.
            // Optionally: send MD5 result back to client.
            char result[6];
            *((uint32_t *)result) = crc32_bitwise((const unsigned char *)imgdata, state->file_size);
            tcp_write(tpcb, result, 6, TCP_WRITE_FLAG_COPY);
            if (frecv_arg.func) {
                int16_t type = frecv_arg.func(imgdata, state->file_size);
                *((uint16_t *)&result[4]) = type;
                AEG_LOG("[DEBUG] user callback returned type: %d\n", type);
            }
        }
        tcp_close(tpcb);
        if (state) {
            mem_free(state);
        }
        memset(state, 0, sizeof(file_transfer_state_t));
        return ERR_OK;
    }

    tcp_recved(tpcb, p->len);
    uint8_t *payload = (uint8_t *)p->payload;
    uint16_t payload_len = p->len;
    uint16_t offset = 0;

    while (payload_len > 0) {
        // First, if header is not yet complete, process header bytes.
        if (state->header_received < HEADER_SIZE) {
            uint16_t needed = HEADER_SIZE - state->header_received;
            uint16_t to_copy = (payload_len < needed) ? payload_len : needed;
            memcpy(state->header_buf + state->header_received, payload + offset, to_copy);
            state->header_received += to_copy;
            offset += to_copy;
            payload_len -= to_copy;

            // If header is now complete, parse it.
            if (state->header_received == HEADER_SIZE) {
                uint32_t magic;
                memcpy(&magic, state->header_buf, 4);
                magic = ntohl(magic);
                if (magic != MAGIC_NUMBER) {
                    // Invalid magic number: abort.
                    pbuf_free(p);
                    tcp_close(tpcb);
                    mem_free(state);
                    return ERR_ABRT;
                }
                // read 32 bit big endian halves and convert to host byte order
                uint32_t hi_be, lo_be;
                memcpy(&hi_be, state->header_buf + 4, 4);
                memcpy(&lo_be, state->header_buf + 8, 4);
                uint64_t hi = ntohl(hi_be);
                uint64_t lo = ntohl(lo_be);

                // rebuild the 64-bit length
                state->file_size = (hi << 32) | lo;
                AEG_LOG("[DEBUG] state->file_size = %d\n", (int)state->file_size);
                state->bytes_received = 0;
                if (state->file_size > imglen) {
                    imglen = ((state->file_size + 3) / 4) * 4;
                    if (imgdata) {
                        free(imgdata);
                    }
                    imgdata = (char *)malloc(imglen);
                }
            }
        } else {
            // Process file data.
            uint64_t remaining = state->file_size - state->bytes_received;
            uint16_t to_process = (payload_len < remaining) ? payload_len : remaining;
            memcpy(imgdata + state->bytes_received, payload + offset, to_process);

            state->bytes_received += to_process;
            offset += to_process;
            payload_len -= to_process;

            if (state->bytes_received == state->file_size) {
                // File completely received.
                offset = 0;
                int16_t type = 0;
                char result[6];
                if (frecv_arg.func) {
                    type = frecv_arg.func(imgdata, state->file_size);
                }
                *((uint32_t *)result) = crc32_bitwise((const unsigned char *)imgdata, state->file_size);
                *((uint16_t *)&result[4]) = type;
                tcp_write(tpcb, result, 6, TCP_WRITE_FLAG_COPY);
                tcp_close(tpcb);
                mem_free(state);
                pbuf_free(p);
                memset(state, 0, sizeof(file_transfer_state_t));
                return ERR_OK;
            }
        }
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t accept_file_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    static int connection = 1;
    file_transfer_state_t *state = (file_transfer_state_t *)mem_malloc(sizeof(file_transfer_state_t));
    if (!state) {
        return ERR_MEM;
    }
    memset(state, 0, sizeof(file_transfer_state_t));
    tcp_arg(newpcb, state);

    /* set the receive callback for this connection */
    tcp_recv(newpcb, file_recv_callback);

    connection++;

    return ERR_OK;
}
static int start_network_process(void *callback) {
    struct tcp_pcb *pcb;
    err_t err;
    unsigned port = 7;

    /* create new TCP PCB structure */
    pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        xil_printf("Error creating PCB. Out of Memory\n\r");
        return -1;
    }

    /* bind to specified @port */
    err = tcp_bind(pcb, IP_ANY_TYPE, port);
    if (err != ERR_OK) {
        xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
        return -2;
    }

    /* we do not need any arguments to callback functions */
    tcp_arg(pcb, NULL);

    /* listen for connections */
    pcb = tcp_listen(pcb);
    if (!pcb) {
        xil_printf("Out of memory while tcp_listen\n\r");
        return -3;
    }

    /* specify callback to use for incoming connections */
    frecv_arg.func = (DataCallback)callback;
    tcp_accept(pcb, accept_file_callback);

    xil_printf("TCP echo server started @ port %d\n\r", port);

    return 0;
}
#endif

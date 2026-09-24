// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    int sock;
    struct sockaddr_in server_addr;
    char send_buf[] = "Hello from client!";
    char recv_buf[1024];
    ssize_t bytes_sent, bytes_recv;

    // 1. Create a TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 2. Fill in the server's address (10.1.1.1:7)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345); // echo port
    if (inet_pton(AF_INET, "172.19.243.31", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    // 3. Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // 4. Send data
    bytes_sent = send(sock, send_buf, strlen(send_buf), 0);
    if (bytes_sent < 0) {
        perror("send");
        close(sock);
        return 1;
    }
    printf("Sent %zd bytes: %s", bytes_sent, send_buf);

    // 5. Receive data (echoed back by the server)
    //    We'll do one read here, but you could loop if you expect more data
    bytes_recv = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
    if (bytes_recv < 0) {
        perror("recv");
        close(sock);
        return 1;
    }
    // Null-terminate if we want to print as a string
    recv_buf[bytes_recv] = '\0';
    printf("Received %zd bytes: %s\n", bytes_recv, recv_buf);

    // 6. Close the socket
    close(sock);
    return 0;
}

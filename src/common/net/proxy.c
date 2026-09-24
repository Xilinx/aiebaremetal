// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
 * Simple TCP proxy: listens on a local port and forwards all traffic
 * to 10.10.70.1:7.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTEN_PORT 12345 // Local port to listen on
#define FORWARD_IP "10.10.70.1"
#define FORWARD_PORT 7
#define BUFFER_SIZE 4096

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

int local_ip_list(void) {
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    printf("Local IP addresses:\n");
    // Loop over the linked list returned by getifaddrs()
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;
        // We want to display IPv4 and IPv6 addresses.
        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host,
                            NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }
            printf("%s: %s\n", ifa->ifa_name, host);
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}

// This function handles the forwarding between the client and the remote server.
void handle_connection(int client_sock) {
    int forward_sock;
    struct sockaddr_in forward_addr;
    char buffer[BUFFER_SIZE];
    int n;

    // Create a socket to connect to the remote server
    if ((forward_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket (forward)");
        return;
    }

    memset(&forward_addr, 0, sizeof(forward_addr));
    forward_addr.sin_family = AF_INET;
    forward_addr.sin_port = htons(FORWARD_PORT);
    if (inet_pton(AF_INET, FORWARD_IP, &forward_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(forward_sock);
        return;
    }

    if (connect(forward_sock, (struct sockaddr *)&forward_addr, sizeof(forward_addr)) < 0) {
        perror("connect");
        close(forward_sock);
        return;
    }

    // Forward data between client_sock and forward_sock
    while (1) {
        fd_set readfds;
        int maxfd = (client_sock > forward_sock) ? client_sock : forward_sock;

        FD_ZERO(&readfds);
        FD_SET(client_sock, &readfds);
        FD_SET(forward_sock, &readfds);

        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            break;
        }

        // Data from client -> remote server
        if (FD_ISSET(client_sock, &readfds)) {
            n = read(client_sock, buffer, BUFFER_SIZE);
            if (n <= 0) { // Connection closed or error
                break;
            }
            if (write(forward_sock, buffer, n) != n) {
                perror("write to forward_sock");
                break;
            }
        }

        // Data from remote server -> client
        if (FD_ISSET(forward_sock, &readfds)) {
            n = read(forward_sock, buffer, BUFFER_SIZE);
            if (n <= 0) { // Connection closed or error
                break;
            }
            if (write(client_sock, buffer, n) != n) {
                perror("write to client_sock");
                break;
            }
        }
    }

    close(forward_sock);
}

int main() {
    int listen_sock, client_sock;
    struct sockaddr_in listen_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Ignore SIGCHLD to prevent zombie processes from forked children
    signal(SIGCHLD, SIG_IGN);

    // Create a listening socket
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket (listen)");
        exit(EXIT_FAILURE);
    }

    // Allow immediate reuse of the port after the proxy is closed
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    listen_addr.sin_port = htons(LISTEN_PORT);

    if (bind(listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock, 10) < 0) {
        perror("listen");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    printf("Proxy listening on port %d, forwarding to %s:%d\n", LISTEN_PORT, FORWARD_IP, FORWARD_PORT);
    local_ip_list();
    // Main loop: accept connections and fork a child to handle each one.
    while (1) {
        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_sock);
        } else if (pid == 0) {
            // Child process
            close(listen_sock); // Child doesn't need the listening socket
            handle_connection(client_sock);
            close(client_sock);
            exit(0);
        } else {
            // Parent process
            close(client_sock);
        }
    }

    close(listen_sock);
    return 0;
}

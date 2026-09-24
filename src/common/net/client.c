// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "netcommon.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define SERVER_IP "172.19.243.31" // Replace with the actual server IP
#define SERVER_PORT 12345         // Server listening port

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_to_send>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *filename = argv[1];

    // Open file for reading in binary mode.
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Determine file size.
    fseek(fp, 0, SEEK_END);
    uint64_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Create and connect socket.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Construct the header.
    uint8_t header[HEADER_SIZE];
    uint32_t magic = htonl(MAGIC_NUMBER);
    memcpy(header, &magic, 4);
    uint64_t net_file_size = file_size; // htobe64(file_size);
    memcpy(header + 4, &net_file_size, 8);

    // Send header.
    ssize_t sent = send(sockfd, header, HEADER_SIZE, 0);
    if (sent != HEADER_SIZE) {
        perror("send header");
        fclose(fp);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send file data in chunks.
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        size_t total_sent = 0;
        while (total_sent < bytes_read) {
            ssize_t n = send(sockfd, buffer + total_sent, bytes_read - total_sent, 0);
            if (n <= 0) {
                perror("send file data");
                fclose(fp);
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            total_sent += n;
        }
    }
    fclose(fp);

    // Optionally, wait for the server to send back the MD5 hash.
    unsigned char crc32_with_type_response[6];
    ssize_t r = recv(sockfd, crc32_with_type_response, 6, 0);
    if (r == 6) {
        printf("Received CRC: ");
        for (int i = 3; i >= 0; i--) {
            printf("%02x", crc32_with_type_response[i]);
        }
        uint16_t type = *((uint16_t *)(&crc32_with_type_response[4]));
        printf("\n");
        printf("type is %d\n", type);
    } else {
        fprintf(stderr, "Did not receive complete MD5 response (r=%zd)\n", r);
    }
    close(sockfd);
    return 0;
}

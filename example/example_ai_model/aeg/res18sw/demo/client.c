// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "netcommon.h"

#include <sys/socket.h>
#include <endian.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#define SERVER_IP "172.19.241.8"  // Replace with the actual server IP
#define SERVER_PORT 12345        // Server listening port

#define MAX_LINE_LENGTH 1024
#define SYNSET "./synset.txt"
char *get_name( int n) {
    const char *filename = SYNSET;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int current_line = 1;

    while ((read = getline(&line, &len, file)) != -1) {
        if (current_line == n) {
            // Remove trailing newline character, if present
            if (read > 0 && line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }

            char *result = strdup(line); //allocate new memory, so line can be freed.
            free(line);
            fclose(file);
            return result;
        }
        current_line++;
    }

    free(line);
    fclose(file);
    return NULL; // Line not found
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file_to_send> <dst IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argc = 3) {
        
    }
    const char *filename = argv[1];
    const char *SERVER_IP = argv[2];

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
    if (sockfd < 0) { perror("socket"); exit(EXIT_FAILURE); }

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
    // split size to hi and lo 32-bit and conver to big-endian byte order
    uint64_t x  = file_size;
    uint32_t hi = htonl((uint32_t)(x >> 32));
    uint32_t lo = htonl((uint32_t)(x & 0xFFFFFFFFu));
    memcpy(header + 4, &hi, sizeof(hi));
    memcpy(header + 8, &lo, sizeof(lo));

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
        for (int i = 3; i >=0; i--) {
            printf("%02x", crc32_with_type_response[i]);
        }
	uint16_t type = *((uint16_t *)(&crc32_with_type_response[4]));
        printf("\n");
	printf("type is idx %d type name is %s\n", type, get_name(type + 1));
    } else {
        fprintf(stderr, "Did not receive complete MD5 response (r=%zd)\n", r);
    }
    close(sockfd);
    return 0;
}

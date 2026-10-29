# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# python3 client.py <filename> <ip address>

import socket  
import struct  
import sys  
import os  
  
SERVER_PORT = 12345    
MAGIC_NUMBER = 0x46494C45  # 'FILE' in ASCII  
HEADER_FMT    = '!IQ'     # 4-byte uint32 + 8-byte uint64, big-endian  
HEADER_SIZE   = struct.calcsize(HEADER_FMT)  
SYNSET = "./synset.txt"  
  
def get_name(n):  
    """Get the name from the synset file."""  
    try:  
        with open(SYNSET, 'r') as file:  
            for i, line in enumerate(file, 1):  
                if i == n:  
                    return line.strip()  
        return None  # Line not found  
    except Exception as e:  
        print(f"Error opening file: {e}")  
        return None  
  
def recvall(sock, n): 
    data = b""  
    while len(data) < n:  
        packet = sock.recv(n - len(data))  
        if not packet:  
            raise Exception("Connection closed before we received all data")  
        data += packet  
    return data  
  
def main():  
    if len(sys.argv) < 3:  
        print(f"Usage: {sys.argv[0]} <file_to_send> <dst IP>")  
        sys.exit(1)  
      
    filename = sys.argv[1]  
    server_ip = sys.argv[2]  
      
    try:  
        file_size = os.path.getsize(filename)  
    except Exception as e:  
        print(f"Error getting file size: {e}")  
        sys.exit(1)  
      
    # Create the TCP socket  and connect
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:  
        try:  
            sock.connect((server_ip, SERVER_PORT))  
        except socket.error as e:  
            print(f"Socket connection error: {e}")  
            sys.exit(1)  
          
        # Construct header  
        header = struct.pack(HEADER_FMT, MAGIC_NUMBER, file_size)

        try:  
            sock.sendall(header)  
        except socket.error as e:  
            print(f"Error sending header: {e}")  
            sys.exit(1)  
          
        try:  
            with open(filename, 'rb') as file:  
                while True:  
                    buffer = file.read(4096)  
                    if not buffer:  
                        break  
                    sock.sendall(buffer)  
        except Exception as e:  
            print(f"Error sending file: {e}")  
            sys.exit(1)  
          
        # Receive response 
        try:  
            crc32_with_type_response = recvall(sock, 6)  
            crc32_bytes = crc32_with_type_response[:4]  
            crc_hex = "".join(f"{b:02x}" for b in reversed(crc32_bytes))  
            print(f"Received CRC: {crc_hex}")  

            # extract from little endian  
            type_value = struct.unpack_from("<H", crc32_with_type_response, 4)[0]  
            type_name = get_name(type_value + 1)  
            print(f"Type is idx {type_value} and type name is {type_name}")  
        except Exception as e:  
            print(f"Error receiving response: {e}")  
  
if __name__ == "__main__":  
    main()  

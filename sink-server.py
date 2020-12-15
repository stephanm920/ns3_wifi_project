#!/usr/bin/python3

# Sink server program. Gets data from TCP bulk sender and
# collects total number of bytes received
import socket
import sys

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 54321              # Arbitrary non-privileged port
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen(1)
    print('Waiting for connection...')
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        num_bytes = 0
        while True:
            data = conn.recv(512)
            if not data:
                break
            num_bytes += 512
        print('Total Bytes Received: ', num_bytes)
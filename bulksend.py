#!/usr/bin/python3

# TCP bulk sender. Sends random 512-byte TCP packets
# for 10 seconds straight

import socket
import time
import os

HOST = '192.168.0.13' #sink server address
PORT = 54321

start_time = time.time()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print('Connected to: ', HOST)
    while time.time() - start_time < 10:
        send_me = os.urandom(512)
        s.sendall(send_me)
    s.close()
    print('Done!')

import time
import socket

client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
while True:
    client_socket.sendto(b'test', ("127.0.0.1", 12000))
    time.sleep(1.)

#! /bin/python3
# status-server.py
# Request status from Contiki nodes
import socket
import threading
import sys

# Configuration
UDP_SERVER_IP = "::1"  # This computer
UDP_SERVER_PORT = 3000
UDP_CLIENT_PORT = 3000

# IP addresses
UDP_CLIENT_IP = ("aaaa::0212:4b00:0dc0:9b14%tun0",  # Node 0
                 "aaaa::0212:4b00:0dc0:776f%tun0")  # Node 1

# Function for setting up a UDP socket
def setup_socket(client_ip):
    (family, socktype, proto, canonname, sockaddr) = socket.getaddrinfo(client_ip, 3000, socket.AF_INET6, socket.SOCK_DGRAM)[0]
    print(socket.getaddrinfo(client_ip, UDP_CLIENT_PORT, socket.AF_INET6, socket.SOCK_DGRAM)[0])
    sock = socket.socket(family, socktype, proto)
    return sock

# Function for sending a message to the target IP
def send_message(sock, message):
    # Send the message
    try:
        sock.sendto(message.encode(), sockaddr)
        print("Sent message:", message)
    except OSError as err:
        print(format(err))
        pass

# Main program
while True:
    # Request which node to request input from
    print("Request status from which node?")
    node_number = input()
    # Check if that node exists
    client_ip = ""
    try:
        client_ip = UDP_CLIENT_IP[int(node_number)]
    except:
        print(sys.exc_info()[0])
        pass
    # Request status from Contiki node
    sock = setup_socket(client_ip)
    send_message(sock, "STATUS")
    # Wait for data to be received
    data, addr = sock.recvfrom(1024) # Buffer size is 1024 bytes
    print("Received message:", data.decode())
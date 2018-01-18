#! /bin/python3
import socket
import threading

# Configuration
UDP_SERVER_IP = "::1"  # This computer
UDP_SERVER_PORT = 3000
#UDP_CLIENT_IP = "aaaa::0212:4b00:0dc0:9b14%tun0"  # Node 1
UDP_CLIENT_IP = "aaaa::0212:4b00:0dc0:776f%tun0"  # Node 2
UDP_CLIENT_PORT = 3000
MESSAGE = "hello"
MESSAGE_INTERVAL = 1  # seconds

# Print configuration
print("Client IPv6 address:", UDP_CLIENT_IP)
print("Client port:", UDP_CLIENT_PORT)
print("Message:", MESSAGE)

# Setup the UDP socket
(family, socktype, proto, canonname, sockaddr) = socket.getaddrinfo(UDP_CLIENT_IP, 3000, socket.AF_INET6, socket.SOCK_DGRAM)[0]
print(socket.getaddrinfo(UDP_CLIENT_IP, 3000, socket.AF_INET6, socket.SOCK_DGRAM)[0])
sock = socket.socket(family, socktype, proto)
#sock.bind((UDP_CLIENT_IP, UDP_CLIENT_PORT, 0, 16))

# Function for sending a message to the target IP
def send_message():
    threading.Timer(MESSAGE_INTERVAL, send_message).start()
    try:
        sock.sendto(MESSAGE.encode(), sockaddr)
        print("Sent message:", MESSAGE)
    except OSError as err:
        print(format(err))
        pass

# Start sending messages every x seconds
send_message()

# Listen for any messages and print them in the terminal
while True:
    data, addr = sock.recvfrom(1024) # Buffer size is 1024 bytes
    print("Received message:", data.decode())
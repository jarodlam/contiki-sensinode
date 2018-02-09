#! /bin/python3
import ubidots
import socket
import threading
import sys

# Configuration
UDP_SERVER_IP = "::1"  # This computer
UDP_SERVER_PORT = 3000
#UDP_CLIENT_IP = "aaaa::0212:4b00:0dc0:9b14%tun0"  # Node 1
UDP_CLIENT_IP = "aaaa::0212:4b00:0dc0:776f%tun0"  # Node 2
UDP_CLIENT_PORT = 3000
MESSAGE = "light"
MESSAGE_INTERVAL = 1  # seconds

# Setup ubidots
api = ubidots.ApiClient(token='A1E-eL7aqNBNH7PXng1ukgLr3grsyzROxD')
ubidots_variable = api.get_variable('5a7a88c6c03f973cd73271f4')

# Print configuration
print("Client IPv6 address:", UDP_CLIENT_IP)
print("Client port:", UDP_CLIENT_PORT)
print("Message:", MESSAGE)

# Function for sending a message to the target IP
def send_message():
    #threading.Timer(MESSAGE_INTERVAL, send_message).start()
    try:
        sock.sendto(MESSAGE.encode(), sockaddr)
        print("Sent message:", MESSAGE)
    except OSError as err:
        print(format(err))
        pass

# Setup the UDP socket
while True:
    (family, socktype, proto, canonname, sockaddr) = socket.getaddrinfo(UDP_CLIENT_IP, 3000, socket.AF_INET6, socket.SOCK_DGRAM)[0]
    #print(socket.getaddrinfo(UDP_CLIENT_IP, 3000, socket.AF_INET6, socket.SOCK_DGRAM)[0])
    sock = socket.socket(family, socktype, proto)

    # Start sending messages every x seconds
    send_message()

    # Listen for any messages and print them in the terminal
    data, addr = sock.recvfrom(1024)  # Buffer size is 1024 bytes
    value = int.from_bytes(data, byteorder=sys.byteorder)
    print("Received message:", value)

    percent = -(value / 32764 * 100) + 100
    
    try:
        ubidots_variable.save_value({'value': percent})
    except ValueError as err:
        print(format(err))
        pass
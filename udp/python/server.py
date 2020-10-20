import socket

size = 8192

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

msg_count = 0

try:
    while True:
        data, address = sock.recvfrom(size)
        sock.sendto("%d %s" % (msg_count, data), address)
        msg_count += 1
finally:
    sock.close()

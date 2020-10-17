import socket
import utils
import re


class FTPClient(object):
    BUFFER_SIZE = 2048

    def __init__(self):
        self.connect_socket = None
        self.data_socket = None
        self.data_mode = 'PASV'

        self.ip = ''
        self.port = -1

    def connect_socket_receive(self):
        lines = []
        buffer = ''
        while True:
            buffer += self.connect_socket.recv(
                self.BUFFER_SIZE).decode("utf-8")
            if '\r\n' in buffer:
                lines += buffer.split('\r\n')[:-1]
                buffer = buffer.split('\r\n')[-1]
                if lines[-1][3] == ' ':
                    break
        return '\n'.join(lines)

    def data_socket_receive(self):
        if self.data_mode == 'PORT':
            sock,_ = self.data_socket.accept()
        else:
            sock = self.data_socket
        
        data = b''
        while True:
            buffer = sock.recv(self.BUFFER_SIZE)
            data += buffer
            if not buffer:
                break
        return data

    def open_data_socket(self):
        if self.data_mode == 'PASV':
            self.PASV()
        elif self.data_mode == 'PORT':
            self.PORT()
        else:
            raise NotImplementedError

    def close_data_socket(self):
        self.data_socket.close()
        self.data_socket = None

    def open_connect_socket(self, host, port=21):
        if self.connect_socket is None:
            self.connect_socket = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM)
            self.connect_socket.connect((host, port))
            res = self.connect_socket_receive()
            return res
        else:
            return ''

    def USER(self, username):
        req = ('USER %s\r\n' % username).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def PASS(self, password):
        req = ('PASS %s\r\n' % password).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def QUIT(self):
        req = 'QUIT\r\n'.encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()

        self.connect_socket.close()
        self.connect_socket = None

        return res

    def SYST(self):
        req = 'SYST\r\n'.encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()

        return res

    def TYPE(self, mode='I'):
        req = ('TYPE %s\r\n' % mode).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()

        return res

    def PORT(self):
        ip = utils.get_host_ip()
        self.data_socket.bind(('', 0))
        _, port = self.data_socket.getsockname()
        self.data_socket.listen(1)
        req = ('PORT %s,%d,%d\r\n' %
               (ip.replace('.', ','), port//256, port % 256)).encode()
        print(req)
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def PASV(self):
        req = 'PASV\r\n'.encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()

        matched = re.search('(\d*),(\d*),(\d*),(\d*),(\d*),(\d*)', res).group()
        matched = matched.split(',')
        ip = '.'.join(matched[:4])

        port = int(matched[4]) * 256 + int(matched[5])
        if self.data_socket is None:
            self.data_socket = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM)
            self.data_socket.connect((ip, port))
            print(ip, port)
        return res

    def PWD(self):
        req = 'PWD\r\n'.encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def CWD(self, newpath):
        req = ('CWD %s\r\n' % newpath).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def MKD(self, newpath):
        req = ('MKD %s\r\n' % newpath).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def RMD(self, path):
        req = ('RMD %s\r\n' % path).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def RNFR(self, path):
        req = ('RNFR %s\r\n' % path).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res
    
    def RNTO(self, path):
        req = ('RNTO %s\r\n' % path).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        return res

    def LIST(self, path):
        self.open_data_socket()

        req = ('LIST %s\r\n' % path).encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()

        data = self.data_socket_receive()
        self.close_data_socket()

        res += '\n' + self.connect_socket_receive()

        return res, data.decode('utf-8').split('\r\n')[:-1]

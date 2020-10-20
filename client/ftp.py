import socket
import utils
import re
from PyQt5.QtCore import *


class FTPClient(QObject):
    BUFFER_SIZE = 8192
    responseGet = pyqtSignal(str)
    dataProgress = pyqtSignal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
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

    def data_socket_receive(self, output_pipe=None):
        if self.data_mode == 'PORT':
            sock, _ = self.data_socket.accept()
        else:
            sock = self.data_socket

        if output_pipe is None:
            data = b''
            current_size = 0
            while True:
                buffer = sock.recv(self.BUFFER_SIZE)
                # print(buffer)
                data += buffer
                current_size += len(buffer)
                self.dataProgress.emit(current_size)
                if not buffer:
                    break
            return data
        else:
            current_size = 0
            while True:
                buffer = sock.recv(self.BUFFER_SIZE)
                current_size += len(buffer)
                output_pipe.write(buffer)
                self.dataProgress.emit(current_size)
                if not buffer:
                    break

    def data_socket_send(self, input_pipe=None):
        if self.data_mode == 'PORT':
            sock, _ = self.data_socket.accept()
        else:
            sock = self.data_socket

        current_size = 0
        while True:
            buffer = input_pipe.read(self.BUFFER_SIZE)
            sock.send(buffer)
            current_size += len(buffer)
            self.dataProgress.emit(current_size)
            if not buffer:
                break

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

    def close_connect_socket(self):
        if self.connect_socket is not None:
            self.connect_socket.close()
            self.connect_socket = None

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

        if self.data_socket is None:
            self.data_socket = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM)

        self.data_socket.bind(('', 0))
        _, port = self.data_socket.getsockname()
        self.data_socket.listen(1)
        req = ('PORT %s,%d,%d\r\n' %
               (ip.replace('.', ','), port//256, port % 256)).encode()
        # print(req)
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        # self.responseGet.emit(res)
        # print(res)
        return res

    def PASV(self):
        req = 'PASV\r\n'.encode()
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        # print(res)

        matched = re.search('(\d*),(\d*),(\d*),(\d*),(\d*),(\d*)', res).group()
        matched = matched.split(',')
        ip = '.'.join(matched[:4])

        port = int(matched[4]) * 256 + int(matched[5])
        if self.data_socket is None:
            self.data_socket = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM)
            self.data_socket.connect((ip, port))
            # print(ip, port)
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

    def DELE(self, path):
        req = ('DELE %s\r\n' % path).encode()
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

        # last one after split is ''
        return res, data.decode('utf-8').split('\r\n')[:-1]

    def RETR(self, remote_file, local_file, cont=False):
        # print(remote_file, local_file)
        self.open_data_socket()

        req = ('RETR %s\r\n' % remote_file).encode()
        # print(req)
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        self.responseGet.emit(res)

        with open(local_file, 'wb' if not cont else 'ab') as f:
            self.data_socket_receive(f)

        self.close_data_socket()

        new_res = self.connect_socket_receive()
        res += '\n' + new_res
        self.responseGet.emit(new_res)

        return res

    def STOR(self, remote_file, local_file, cont=False, rest=0):
        # print(remote_file, local_file)
        self.open_data_socket()

        req = ('STOR %s\r\n' % remote_file).encode()
        # print(req)
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        self.responseGet.emit(res)

        with open(local_file, 'rb') as f:
            if cont:
                f.seek(rest)
            self.data_socket_send(f)

        self.close_data_socket()

        new_res = self.connect_socket_receive()
        res += '\n' + new_res
        self.responseGet.emit(new_res)

        return res

    def REST(self, rest):
        req = ('REST %d\r\n' % rest).encode()
        # print(req)
        self.connect_socket.sendall(req)
        res = self.connect_socket_receive()
        self.responseGet.emit(res)

        return res

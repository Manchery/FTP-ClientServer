from requests import get
import grp
import pwd
import stat
from ns import NS
import os
import time

# [standard library - Getting a machine's external IP address with Python - Stack Overflow](https://stackoverflow.com/questions/2311510/getting-a-machines-external-ip-address-with-python)


def get_host_ip():
    ip = get('https://api.ipify.org').text
    # print 'My public IP address is:', ip
    return ip


# [get open TCP port in Python - Stack Overflow](https://stackoverflow.com/questions/2838244/get-open-tcp-port-in-python/2838309#2838309)
def get_open_port():
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("", 0))
    # s.listen(1)
    port = s.getsockname()[1]
    s.close()
    return port

def get_file_info_from_str(s):
    x = s.split()
    info = NS()
    info.mode = x[0]
    info.mode_num = x[1]
    info.owner = x[2]
    info.group = x[3]
    info.size = x[4]
    info.date = ' '.join(x[5:-1])
    info.filename = x[-1]
    return info

def get_file_info(filepath):
    st = os.stat(filepath)

    def mode_to_letters():
        mode = st.st_mode
        letters = list('----------')

        if os.path.isdir(filepath):
            letters[0] = 'd'

        if bool(mode & stat.S_IRUSR):
            letters[1] = 'r'
        if bool(mode & stat.S_IWUSR):
            letters[2] = 'w'
        if bool(mode & stat.S_IXUSR):
            letters[3] = 'x'

        if bool(mode & stat.S_IRGRP):
            letters[4] = 'r'
        if bool(mode & stat.S_IWGRP):
            letters[5] = 'w'
        if bool(mode & stat.S_IXGRP):
            letters[6] = 'x'

        if bool(mode & stat.S_IROTH):
            letters[7] = 'r'
        if bool(mode & stat.S_IWOTH):
            letters[8] = 'w'
        if bool(mode & stat.S_IXOTH):
            letters[9] = 'x'

        return ''.join(letters)

    info = NS()
    info.mode = mode_to_letters()
    info.mode_num = str(st.st_nlink)
    info.owner = pwd.getpwuid(st.st_uid).pw_name
    info.group = grp.getgrgid(st.st_gid).gr_name
    info.size = str(st.st_size)
    info.date = time.strftime('%b %d %H:%M', time.gmtime(st.st_mtime))
    info.filename = os.path.basename(filepath)

    return info

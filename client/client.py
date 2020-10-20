import os
import sys
from PyQt5 import QtGui, QtWidgets
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
from ftp import FTPClient
from dialog import inputDialog, confirmDialog
from threading import Thread
from copy import deepcopy
import re
import time

import utils
from functools import partial


# parent class for DownloadThread and UploadThread
class AbstractDataThread(QThread):
    responseGet = pyqtSignal(str)
    dataProgress = pyqtSignal(int)
    finish = pyqtSignal()
    failed = pyqtSignal()

    def __init__(self, ftp, remote_file, local_file, parent=None, rest=0):
        super().__init__(parent)
        self.parent = parent
        self.ftp = ftp
        self.remote_file = remote_file
        self.local_file = local_file
        self.rest = rest

        self.ftp.responseGet.connect(self.responseGet)
        self.ftp.dataProgress.connect(self.dataProgress)


class DownloadThread(AbstractDataThread):
    def __init__(self, ftp, remote_file, local_file, parent=None, rest=0):
        super(DownloadThread, self).__init__(
            ftp, remote_file, local_file, parent, rest)

    def run(self):
        if self.rest != 0:
            self.ftp.REST(self.rest)

        retr_res = self.ftp.RETR(self.remote_file, self.local_file, self.rest != 0)
        self.ftp.QUIT()
        self.ftp.close_connect_socket()
        if '226' in retr_res:
            self.finish.emit()
        else:
            self.failed.emit()


class UploadThread(AbstractDataThread):
    def __init__(self, ftp, remote_file, local_file, parent=None, rest=0):
        super(UploadThread, self).__init__(
            ftp, remote_file, local_file, parent, rest)

    def run(self):
        if self.rest != 0:
            self.ftp.REST(self.rest)

        stor_res = self.ftp.STOR(self.remote_file, self.local_file,
                      self.rest != 0, self.rest)
        self.ftp.QUIT()
        self.ftp.close_connect_socket()
        if '226' in stor_res:
            self.finish.emit()
        else:
            self.failed.emit()


# parent class for LocalFileWidget and RemoteFileWidget
class AbstractFileWidget(QWidget):
    selectedFileChanged = pyqtSignal()

    def __init__(self, parent=None):
        super(AbstractFileWidget, self).__init__(parent)
        self.parent = parent

        self.mainLayout = QVBoxLayout()

        pathLayout = QHBoxLayout()
        self.pathLabel = QLabel()
        self.pathEdit = QLineEdit()
        # completer for path edit
        completer = QtWidgets.QCompleter()
        self.completerModel = QStringListModel()
        completer.setModel(self.completerModel)
        self.completeWordList = []
        self.pathEdit.setCompleter(completer)
        pathLayout.addWidget(self.pathLabel)
        pathLayout.addWidget(self.pathEdit)
        self.mainLayout.addLayout(pathLayout)

        self.files = QTreeWidget()
        self.files.setIconSize(QSize(20, 20))
        self.files.setRootIsDecorated(False)
        # self.files.header().setStretchLastSection(False)

        self.mainLayout.addWidget(self.files)

        selectedLayout = QHBoxLayout()
        selectedLayout.addWidget(QLabel('Selected: '))
        self.selected = QLabel()
        selectedLayout.addWidget(self.selected, stretch=1)
        self.mainLayout.addLayout(selectedLayout)

        self.buttonLayout = QHBoxLayout()
        self.buttonLayout.setAlignment(Qt.AlignRight)
        self.mainLayout.addLayout(self.buttonLayout)

        self.setLayout(self.mainLayout)

        self.files.currentItemChanged.connect(self.currentFileChanged)
        self.files.currentItemChanged.connect(
            self.selectedFileChanged)  # signal

        self.cwd = ''

    def currentFileChanged(self):
        if self.files.currentItem() is not None:
            filename = self.files.currentItem().text(0)
            path = os.path.abspath(os.path.join(self.cwd, filename))
            self.selected.setHidden(True)
            self.selected.setText(path)
            self.selected.setHidden(False)

    def addItem(self, item):
        self.files.addTopLevelItem(item)
        if not self.files.currentItem():
            self.files.setCurrentItem(self.files.topLevelItem(0))
            self.files.setEnabled(True)

    def pathEditSetText(self, t):
        self.pathEdit.setHidden(True)
        self.pathEdit.setText(t)
        self.pathEdit.setHidden(False)

    def selectedIsFile(self):
        if self.files.currentItem() is not None:
            if self.files.currentItem().text(0) == '..':
                return False
            return self.files.currentItem().text(2)[0] != 'd'
        else:
            return False

    def selectedIsDir(self):
        if self.files.currentItem() is not None:
            if self.files.currentItem().text(0) == '..':
                return True
            return self.files.currentItem().text(2)[0] == 'd'
        else:
            return False


class LocalFileWidget(AbstractFileWidget):
    uploadClicked = pyqtSignal()
    contUploadClicked = pyqtSignal()

    def __init__(self, cwd=os.path.expanduser('~'), parent=None):
        super(LocalFileWidget, self).__init__(parent)

        self.pathLabel.setText('Local Path')
        self.files.setHeaderLabels(['Name', 'Size', 'Mode', 'Last Time'])

        self.upload_button = QPushButton(text='Upload')
        self.buttonLayout.addWidget(self.upload_button)
        self.cont_upload_button = QPushButton(text='Continue To Upload')
        self.buttonLayout.addWidget(self.cont_upload_button)

        self.upload_button.clicked.connect(self.uploadButtonClicked)
        self.cont_upload_button.clicked.connect(self.contUploadClicked)

        self.cwd = cwd
        self.updateLocalPath()

        self.files.itemDoubleClicked.connect(self.fileDoubleClicked)
        self.pathEdit.returnPressed.connect(self.pathEditReturnPressed)

    # SLOTS
    def pathEditReturnPressed(self):
        self.updateLocalPath(os.path.abspath(self.pathEdit.text()))

    def fileDoubleClicked(self, item, column):
        path = os.path.abspath(os.path.join(self.cwd, item.text(0)))
        if os.path.isdir(path):
            self.updateLocalPath(path)

    def uploadButtonClicked(self):
        mode = self.files.currentItem().text(2)
        if not mode.startswith('d'):
            self.uploadClicked.emit()
    # SLOTS END

    def updateLocalPath(self, newCwd=None):
        if newCwd is not None:
            if not os.path.isdir(newCwd):
                return
            self.cwd = newCwd
        self.pathEditSetText(self.cwd)

        self.files.clear()

        self.completeWordList = []

        # back to parent dir
        if self.cwd != '/':
            item = QTreeWidgetItem()
            icon = QIcon('icons/folder.png')
            item.setIcon(0, icon)
            item.setText(0, '..')
            for i in range(1, 4):
                item.setText(i, '')
            self.addItem(item)

        for f in os.listdir(self.cwd):
            # hidden files
            if f.startswith('.'):
                continue

            path = os.path.join(self.cwd, f)
            info = utils.get_file_info(path)

            if info.mode.startswith('d'):
                self.completeWordList.append(path)
                icon = QIcon('icons/folder.png')
            else:
                icon = QIcon('icons/file.png')

            item = QTreeWidgetItem()
            item.setIcon(0, icon)
            item.setText(0, info.filename)
            item.setText(1, info.size)
            item.setText(2, info.mode)
            item.setText(3, info.date)
            self.addItem(item)

        self.completerModel.setStringList(self.completeWordList)


class RemoteFileWidget(AbstractFileWidget):
    downloadClicked = pyqtSignal()
    contDownloadClicked = pyqtSignal()

    def __init__(self, ftp, parent=None):
        super(RemoteFileWidget, self).__init__(parent)

        self.ftp = ftp

        self.pathLabel.setText('Remote Path')
        self.files.setHeaderLabels(
            ['Name', 'Size', 'Mode', 'Last Time', 'Owner/Group'])

        self.download_button = QPushButton(text='Download')
        self.buttonLayout.addWidget(self.download_button)
        self.cont_download_button = QPushButton(text='Continue To Download')
        self.buttonLayout.addWidget(self.cont_download_button)
        self.mkd_button = QPushButton(text='Make Dir')
        self.buttonLayout.addWidget(self.mkd_button)
        self.rmd_button = QPushButton(text='Remove')
        self.buttonLayout.addWidget(self.rmd_button)
        self.rename_button = QPushButton(text='Rename')
        self.buttonLayout.addWidget(self.rename_button)

        self.mkd_button.clicked.connect(self.mkdButtonClicked)
        self.rmd_button.clicked.connect(self.rmdButtonClicked)
        self.rename_button.clicked.connect(self.renameButtonClicked)

        self.download_button.clicked.connect(self.downloadButtonClicked)
        self.cont_download_button.clicked.connect(self.contDownloadClicked)

        self.files.itemDoubleClicked.connect(self.fileDoubleClicked)
        self.pathEdit.returnPressed.connect(self.pathEditReturnPressed)

    # SLOTS
    def fileDoubleClicked(self, item, column):
        path = os.path.abspath(os.path.join(self.cwd, item.text(0)))
        if item.text(0) == '..' or item.text(2).startswith('d'):
            self.updateRemotePath(path)

    def pathEditReturnPressed(self):
        self.updateRemotePath(os.path.abspath(self.pathEdit.text()))

    def mkdButtonClicked(self):
        new_dir = inputDialog('Make Dir', 'Dir name', parent=self)
        if new_dir:
            mkd_res = self.ftp.MKD(new_dir)
            self.parent.log(mkd_res)
            self.updateRemotePath(self.cwd)

    def renameButtonClicked(self):
        if self.files.currentItem() is None:
            return

        old_name = self.files.currentItem().text(0)
        new_name = inputDialog('Rename', 'New name',
                               default=old_name, parent=self)
        if new_name and new_name != old_name:
            rnfr_res = self.ftp.RNFR(old_name)
            if rnfr_res.startswith('350'):
                self.parent.log(rnfr_res)
                rnto_res = self.ftp.RNTO(new_name)
                self.parent.log(rnto_res)
                self.updateRemotePath(self.cwd)

    def rmdButtonClicked(self):
        if self.files.currentItem() is None:
            return

        if confirmDialog('Remove', 'Sure?', parent=self):
            if self.files.currentItem().text(2).startswith('d'):
                rmd_res = self.ftp.RMD(self.files.currentItem().text(0))
                self.parent.log(rmd_res)
                self.updateRemotePath(self.cwd)
            else:
                rmd_res = self.ftp.DELE(self.files.currentItem().text(0))
                self.parent.log(rmd_res)
                self.updateRemotePath(self.cwd)

    def downloadButtonClicked(self):
        mode = self.files.currentItem().text(2)
        if not mode.startswith('d'):
            self.downloadClicked.emit()
    # SLOTS END

    def updateRemotePath(self, newCwd):
        if self.cwd != newCwd:
            cwd_res = self.ftp.CWD(newCwd)
            self.parent.log(cwd_res)
            if cwd_res.startswith('250'):
                self.cwd = newCwd

        self.pathEditSetText(self.cwd)

        list_res, list_data = self.ftp.LIST(self.cwd)
        self.parent.log(list_res)
        self.file_infos = [utils.get_file_info_from_str(x) for x in list_data]

        self.files.clear()
        self.completeWordList = []

        # back to parent dir
        if self.cwd != '/':
            item = QTreeWidgetItem()
            icon = QIcon('icons/folder.png')
            item.setIcon(0, icon)
            item.setText(0, '..')
            for i in range(1, 5):
                item.setText(i, '')
            self.addItem(item)

        for info in self.file_infos:
            f = info.filename
            if f.startswith('.'):
                continue

            path = os.path.join(self.cwd, f)

            if info.mode.startswith('d'):
                self.completeWordList.append(path)
                icon = QIcon('icons/folder.png')
            else:
                icon = QIcon('icons/file.png')

            item = QTreeWidgetItem()
            item.setIcon(0, icon)
            item.setText(0, info.filename)
            item.setText(1, info.size)
            item.setText(2, info.mode)
            item.setText(3, info.date)
            item.setText(4, info.owner + ' ' + info.group)
            self.addItem(item)

        self.completerModel.setStringList(self.completeWordList)


class MainWindow(QWidget):
    COLORS = {
        '1': 'darkGreen',
        '2': 'green',
        '3': 'blue',
        '4': 'orange',
        '5': 'red',
    }

    def __init__(self, app, parent=None):
        super(MainWindow, self).__init__(parent)

        self.connected = False

        self.app = app

        self.ftp = FTPClient()
        self.ftp.responseGet.connect(lambda x: self.log(x))

        self.resize(1200, 900)
        self.setWindowTitle('FTP Client')

        self.createGui()

        self.all_buttons = [
            self.local.upload_button,
            self.local.cont_upload_button,
            self.remote.download_button,
            self.remote.cont_download_button,
            self.remote.mkd_button,
            self.remote.rename_button,
            self.remote.rmd_button
        ]
        for b in self.all_buttons:
            b.setDisabled(True)

        # debug
        # self.hostEdit.setText('59.66.136.21')
        # self.userEdit.setText('ssast')
        # self.passwordEdit.setText('ssast')

        # self.hostEdit.setText('192.168.98.132')
        # self.userEdit.setText('anonymous')
        # self.passwordEdit.setText('ssast')
        # debug end

        self.connectButton.clicked.connect(self.connectSwitch)
        self.dataModeButton.clicked.connect(self.dataModeSwitch)

        self.remote.downloadClicked.connect(self.download)
        self.remote.contDownloadClicked.connect(self.cont_download)
        self.local.uploadClicked.connect(self.upload)
        self.local.contUploadClicked.connect(self.cont_upload)

        self.local.selectedFileChanged.connect(self.selectedFileChanged)
        self.remote.selectedFileChanged.connect(self.selectedFileChanged)

    def log(self, log_str):
        self.messages.setHidden(True)

        for l in log_str.strip().split('\n'):
            if len(l) == 0:
                continue
            if l.startswith('[Client Error]'):
                color = 'darkRed'
            else:
                color = self.COLORS.get(l[0], 'black')
            s = '%s <font color="%s">%s</font>' % (time.strftime(
                "%Y-%m-%d %H:%M:%S", time.localtime()), color, l)
            self.messages.append(s)

        self.messages.setHidden(False)

    def dataModeSwitch(self):
        if self.ftp.data_mode == 'PORT':
            self.ftp.data_mode = 'PASV'
        elif self.ftp.data_mode == 'PASV':
            self.ftp.data_mode = 'PORT'
        else:
            raise NotImplementedError

        self.dataModeButton.setText(
            'Use PORT Mode' if self.ftp.data_mode == 'PASV' else 'Use PASV Mode')

    def connectSwitch(self):
        if not self.connected:
            host = self.hostEdit.text()
            username = self.userEdit.text()
            password = self.passwordEdit.text()
            port = int(self.portEdit.text())

            try:
                connect_res = self.ftp.open_connect_socket(host, port)
                self.log(connect_res)
            except:
                self.ftp.close_connect_socket()
                self.log("[Client Error] Network is unreachable.")
            else:
                if connect_res.startswith('220'):
                    user_res = self.ftp.USER(username)
                    self.log(user_res)

                    if user_res.startswith('331'):
                        pass_res = self.ftp.PASS(password)
                        self.log(pass_res)

                        if pass_res.startswith('230'):
                            syst_res = self.ftp.SYST()
                            self.log(syst_res)
                            type_res = self.ftp.TYPE()
                            self.log(type_res)

                            if type_res.startswith('200'):
                                pwd_res = self.ftp.PWD()
                                self.log(pwd_res)
                                cwd = re.search(
                                    '\".*\"', pwd_res).group()[1:-1]

                                self.connected = True
                                self.remote.cwd = cwd
                                self.remote.updateRemotePath(cwd)

                                self.hostEdit.setDisabled(True)
                                self.userEdit.setDisabled(True)
                                self.passwordEdit.setDisabled(True)
                                self.portEdit.setDisabled(True)

                    if not self.connected:
                        self.log(self.ftp.QUIT())
        else:
            quit_res = self.ftp.QUIT()
            self.log(quit_res)
            if quit_res.startswith('221'):
                self.ftp.close_connect_socket()
                self.connected = False
                self.remote.pathEditSetText('')
                self.remote.files.clear()

                self.hostEdit.setDisabled(False)
                self.userEdit.setDisabled(False)
                self.passwordEdit.setDisabled(False)
                self.portEdit.setDisabled(False)
                for b in self.all_buttons:
                    b.setHidden(True)
                    b.setDisabled(True)
                    b.setHidden(False)

        self.connectButton.setText(
            'Connect' if not self.connected else 'Disconnect')

    def selectedFileChanged(self):

        def setButtonDisabled(b, disable):
            b.setHidden(True)
            b.setDisabled(disable)
            b.setHidden(False)

        if self.connected and self.local.selectedIsFile():
            setButtonDisabled(self.local.upload_button, False)
        else:
            setButtonDisabled(self.local.upload_button, True)

        if self.connected and self.remote.selectedIsFile():
            setButtonDisabled(self.remote.download_button, False)
        else:
            setButtonDisabled(self.remote.download_button, True)

        if self.connected and self.local.selectedIsFile() and self.remote.selectedIsFile():
            setButtonDisabled(self.local.cont_upload_button, False)
            setButtonDisabled(self.remote.cont_download_button, False)
        else:
            setButtonDisabled(self.local.cont_upload_button, True)
            setButtonDisabled(self.remote.cont_download_button, True)

        if self.connected:
            setButtonDisabled(self.remote.mkd_button, False)
        else:
            setButtonDisabled(self.remote.mkd_button, True)

        if self.connected and self.remote.files.currentItem() is not None and self.remote.files.currentItem().text(0) != '..':
            setButtonDisabled(self.remote.rename_button, False)
            setButtonDisabled(self.remote.rmd_button, False)
        else:
            setButtonDisabled(self.remote.rename_button, True)
            setButtonDisabled(self.remote.rmd_button, True)

    # -------------- Data Transimission ---------------

    def clone_ftp_connection(self):
        ftp = FTPClient()
        ftp.data_mode = self.ftp.data_mode
        host = self.hostEdit.text()
        port = int(self.portEdit.text())
        username = self.userEdit.text()
        password = self.passwordEdit.text()
        try:
            ftp.open_connect_socket(host, port)
        except:
            self.log("[Client Error] Network is unreachable.")
            return None
        else:
            ftp.USER(username)
            ftp.PASS(password)
            ftp.TYPE()
            return ftp

    def download(self):
        if self.remote.files.currentItem() is None:
            return

        filename = self.remote.files.currentItem().text(0)
        filesize = int(self.remote.files.currentItem().text(1))
        remote_dir = self.remote.cwd
        local_dir = self.local.cwd

        remote_file = os.path.join(remote_dir, filename)
        local_file = os.path.join(local_dir, filename)

        d_ftp = self.clone_ftp_connection()
        if d_ftp is None:
            return

        item = QTreeWidgetItem()
        item.setText(0, 'remote -> local')
        item.setText(1, local_file)
        item.setText(2, remote_file)
        item.setText(3, str(filesize))
        item.setText(4, '0%')
        item.setText(5, 'Pending')
        self.queue.addTopLevelItem(item)

        downloadThread = DownloadThread(d_ftp, remote_file, local_file, self)
        downloadThread.responseGet.connect(lambda x: self.log(x))

        def modify_item(x):
            if filesize != 0:
                item.setText(4, ('%.2f' % (x/filesize*100)) + '%')
            else:
                item.setText(4, '100%')
            item.setText(5, 'Downloading')
        downloadThread.dataProgress.connect(modify_item)

        def downloadDone():
            item.setText(5, 'Done')
            self.local.updateLocalPath(self.local.cwd)
        downloadThread.finish.connect(downloadDone)
        def downloadFailed():
            item.setText(5, 'Failed')
            self.local.updateLocalPath(self.local.cwd)
        downloadThread.failed.connect(downloadFailed)

        downloadThread.start()

    def cont_download(self):
        if self.remote.files.currentItem() is None or self.local.files.currentItem() is None:
            return

        if self.remote.files.currentItem().text(0) != self.local.files.currentItem().text(0):
            if not confirmDialog('Continue to Download', 'Filename not Same. Sure?', parent=self):
                return

        filename = self.remote.files.currentItem().text(0)
        filesize = int(self.remote.files.currentItem().text(1))
        rest = int(self.local.files.currentItem().text(1))
        remote_dir = self.remote.cwd
        local_dir = self.local.cwd

        remote_file = os.path.join(remote_dir, filename)
        local_file = os.path.join(
            local_dir, self.local.files.currentItem().text(0))

        d_ftp = self.clone_ftp_connection()
        if d_ftp is None:
            return

        item = QTreeWidgetItem()
        item.setText(0, 'remote -> local')
        item.setText(1, local_file)
        item.setText(2, remote_file)
        item.setText(3, str(filesize))
        item.setText(4, ('%.2f' % (rest/filesize*100)) + '%')
        item.setText(5, 'Pending')
        self.queue.addTopLevelItem(item)

        downloadThread = DownloadThread(
            d_ftp, remote_file, local_file, self, rest)
        downloadThread.responseGet.connect(lambda x: self.log(x))

        def modify_item(x):
            if filesize != 0:
                item.setText(4, ('%.2f' % ((x+rest)/filesize*100)) + '%')
            else:
                item.setText(4, '100%')
            item.setText(5, 'Downloading')
        downloadThread.dataProgress.connect(modify_item)

        def downloadDone():
            item.setText(5, 'Done')
            self.local.updateLocalPath(self.local.cwd)
        downloadThread.finish.connect(downloadDone)
        def downloadFailed():
            item.setText(5, 'Failed')
            self.local.updateLocalPath(self.local.cwd)
        downloadThread.failed.connect(downloadFailed)

        downloadThread.start()

    def upload(self):
        if self.local.files.currentItem() is None:
            return

        filename = self.local.files.currentItem().text(0)
        filesize = int(self.local.files.currentItem().text(1))
        remote_dir = self.remote.cwd
        local_dir = self.local.cwd

        remote_file = os.path.join(remote_dir, filename)
        local_file = os.path.join(local_dir, filename)

        u_ftp = self.clone_ftp_connection()
        if u_ftp is None:
            return

        item = QTreeWidgetItem()
        item.setText(0, 'local -> remote')
        item.setText(1, local_file)
        item.setText(2, remote_file)
        item.setText(3, str(filesize))
        item.setText(4, '0%')
        item.setText(5, 'Pending')
        self.queue.addTopLevelItem(item)

        uploadThread = UploadThread(u_ftp, remote_file, local_file, self)
        uploadThread.responseGet.connect(lambda x: self.log(x))

        def modify_item(x):
            if filesize != 0:
                item.setText(4, ('%.2f' % (x/filesize*100)) + '%')
            else:
                item.setText(4, '100%')
            item.setText(5, 'Uploading')
        uploadThread.dataProgress.connect(modify_item)

        def uploadDone():
            item.setText(5, 'Done')
            self.remote.updateRemotePath(self.remote.cwd)
        uploadThread.finish.connect(uploadDone)
        def uploadFailed():
            item.setText(5, 'Failed')
            self.remote.updateRemotePath(self.remote.cwd)
        uploadThread.failed.connect(uploadFailed)

        uploadThread.start()

    def cont_upload(self):
        if self.remote.files.currentItem() is None or self.local.files.currentItem() is None:
            return

        if self.remote.files.currentItem().text(0) != self.local.files.currentItem().text(0):
            if not confirmDialog('Continue to Upload', 'Filename not Same. Sure?', parent=self):
                return

        filename = self.local.files.currentItem().text(0)
        filesize = int(self.local.files.currentItem().text(1))
        rest = int(self.remote.files.currentItem().text(1))
        remote_dir = self.remote.cwd
        local_dir = self.local.cwd

        remote_file = os.path.join(
            remote_dir, self.remote.files.currentItem().text(0))
        local_file = os.path.join(local_dir, filename)

        u_ftp = self.clone_ftp_connection()
        if u_ftp is None:
            return

        item = QTreeWidgetItem()
        item.setText(0, 'local -> remote')
        item.setText(1, local_file)
        item.setText(2, remote_file)
        item.setText(3, str(filesize))
        item.setText(4, ('%.2f' % (rest/filesize*100)) + '%')
        item.setText(5, 'Pending')
        self.queue.addTopLevelItem(item)

        uploadThread = UploadThread(u_ftp, remote_file, local_file, self, rest)
        uploadThread.responseGet.connect(lambda x: self.log(x))

        def modify_item(x):
            if filesize != 0:
                item.setText(4, ('%.2f' % ((x+rest)/filesize*100)) + '%')
            else:
                item.setText(4, '100%')
            item.setText(5, 'Uploading')
        uploadThread.dataProgress.connect(modify_item)

        def uploadDone():
            item.setText(5, 'Done')
            self.remote.updateRemotePath(self.remote.cwd)
        uploadThread.finish.connect(uploadDone)
        def uploadFailed():
            item.setText(5, 'Failed')
            self.remote.updateRemotePath(self.remote.cwd)
        uploadThread.failed.connect(uploadFailed)
        
        uploadThread.start()

    # --------------------- GUI -----------------------

    def createGui(self):
        self.mainLayout = QtWidgets.QVBoxLayout()

        self.mainLayout.addLayout(self.createLoginLayout())
        self.mainLayout.addLayout(self.createMessageLayout(), stretch=1)
        self.mainLayout.addLayout(self.createFileLayout(), stretch=4)
        self.mainLayout.addLayout(self.createQueueLayout(), stretch=1)

        self.setLayout(self.mainLayout)

    def createLoginLayout(self):
        loginLayout = QHBoxLayout()

        hostLabel = QLabel('Host')
        self.hostEdit = QLineEdit()
        loginLayout.addWidget(hostLabel)
        loginLayout.addWidget(self.hostEdit)

        userLabel = QLabel('User')
        self.userEdit = QLineEdit()
        # self.userEdit.setText('anonymous')
        # self.userEdit.setDisabled(True)
        loginLayout.addWidget(userLabel)
        loginLayout.addWidget(self.userEdit)

        passwordLabel = QLabel('Password')
        self.passwordEdit = QLineEdit()
        self.passwordEdit.setEchoMode(QLineEdit.Password)
        loginLayout.addWidget(passwordLabel)
        loginLayout.addWidget(self.passwordEdit)

        portLabel = QLabel('Port')
        self.portEdit = QLineEdit('21')
        loginLayout.addWidget(portLabel)
        loginLayout.addWidget(self.portEdit)

        self.connectButton = QPushButton()
        self.connectButton.setText(
            'Connect' if not self.connected else 'Disconnect')
        loginLayout.addWidget(self.connectButton)

        self.dataModeButton = QPushButton()
        self.dataModeButton.setText(
            'Use PORT Mode' if self.ftp.data_mode == 'PASV' else 'Use PASV Mode')
        loginLayout.addWidget(self.dataModeButton)

        return loginLayout

    def createMessageLayout(self):
        messageLayout = QVBoxLayout()
        self.messages = QTextBrowser()
        messageLayout.addWidget(self.messages)
        return messageLayout

    def createFileLayout(self):
        fileLayout = QHBoxLayout()

        self.local = LocalFileWidget(parent=self)
        self.remote = RemoteFileWidget(self.ftp, parent=self)
        fileLayout.addWidget(self.local)
        fileLayout.addWidget(self.remote)

        return fileLayout

    def createQueueLayout(self):
        queueLayout = QHBoxLayout()
        self.queue = QTreeWidget()
        self.queue.setRootIsDecorated(False)
        self.queue.setHeaderLabels(
            ['Direction', 'Local File', 'Remote File', 'Size', 'Progress', 'Status'])
        queueLayout.addWidget(self.queue)
        return queueLayout


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    client = MainWindow(app)
    client.show()
    app.exec_()

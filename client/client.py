import os
import sys
from PyQt5 import QtGui, QtWidgets
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
from ftp import FTPClient

from log import Window
import utils
from functools import partial


class AbstractFileWidget(QWidget):
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

        self.setLayout(self.mainLayout)

        self.files.currentItemChanged.connect(self.selectionChanged)

        self.cwd = ''

    def selectionChanged(self):
        filename = self.files.currentItem().text(0)
        path = os.path.abspath(os.path.join(self.cwd, filename))
        self.selected.setText(path)

    def addItem(self, item):
        self.files.addTopLevelItem(item)
        print(item.text(0))
        if not self.files.currentItem():
            print('aaa')
            self.files.setCurrentItem(self.files.topLevelItem(0))
            self.files.setEnabled(True)

    def pathEditSetText(self, t):
        self.pathEdit.setHidden(True)
        self.pathEdit.setText(t)
        self.pathEdit.setHidden(False)


class LocalFileWidget(AbstractFileWidget):
    def __init__(self, cwd=os.path.expanduser('~'), parent=None):
        super(LocalFileWidget, self).__init__(parent)

        self.pathLabel.setText('Local Path')
        self.files.setHeaderLabels(['Name', 'Size', 'Mode', 'Modified Time'])

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
    # SLOTS END

    def updateLocalPath(self, newCwd=None):
        if newCwd is not None:
            self.cwd = newCwd
        self.pathEditSetText(self.cwd)

        self.files.clear()

        self.completeWordList = []

        if self.cwd != '/':
            item = QTreeWidgetItem()
            icon = QIcon('icons/folder.png')
            item.setIcon(0, icon)
            item.setText(0, '..')
            for i in range(1, 4):
                item.setText(i, '')
            self.addItem(item)

        for f in os.listdir(self.cwd):
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
    def __init__(self, ftp, parent=None):
        super(RemoteFileWidget, self).__init__(parent)

        self.ftp = ftp

        self.pathLabel.setText('Remote Path')
        self.files.setHeaderLabels(
            ['Name', 'Size', 'Mode', 'Modified Time', 'Owner/Group'])

        self.files.itemDoubleClicked.connect(self.fileDoubleClicked)
        self.pathEdit.returnPressed.connect(self.pathEditReturnPressed)

    # SLOTS
    def fileDoubleClicked(self, item, column):
        path = os.path.abspath(os.path.join(self.cwd, item.text(0)))
        if item.text(0) == '..' or item.text(2).startswith('d'):
            self.updateRemotePath(path)

    def pathEditReturnPressed(self):
        self.updateRemotePath(os.path.abspath(self.pathEdit.text()))
    # SLOTS END

    def updateRemotePath(self, newCwd):
        if self.cwd != newCwd:
            self.parent.log(self.ftp.CWD(newCwd))
            self.cwd = newCwd

        self.pathEditSetText(self.cwd)

        list_req, list_data = self.ftp.LIST(self.cwd)
        self.parent.log(list_req)
        self.file_infos = [utils.get_file_info_from_str(x) for x in list_data]

        self.files.clear()
        self.completeWordList = []

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
    def __init__(self, app, parent=None):
        super(MainWindow, self).__init__(parent)

        self.connected = False

        self.app = app

        self.ftp = FTPClient()

        self.resize(1200, 900)
        self.setWindowTitle('FTP Client')

        self.mainLayout = QtWidgets.QVBoxLayout()

        self.mainLayout.addLayout(self.createLoginLayout())
        self.mainLayout.addLayout(self.createMessageLayout(), stretch=1)
        self.mainLayout.addLayout(self.createFileLayout(), stretch=4)
        self.mainLayout.addLayout(self.createQueueLayout(), stretch=1)

        self.setLayout(self.mainLayout)

        # debug
        self.hostEdit.setText('59.66.136.21')
        self.userEdit.setText('ssast')
        self.passwordEdit.setText('ssast')
        # debug end

        self.connectButton.clicked.connect(self.connectSwitch)

    def log(self, log_str):
        self.messages.setHidden(True)
        self.messages.append(log_str.strip())
        self.messages.setHidden(False)

    def connectSwitch(self):
        if not self.connected:
            host = self.hostEdit.text()
            username = self.userEdit.text()
            password = self.passwordEdit.text()

            self.log(self.ftp.open_connect_socket(host))
            self.log(self.ftp.USER(username))
            self.log(self.ftp.PASS(password))
            self.log(self.ftp.SYST())
            self.log(self.ftp.TYPE())

            pwd_res = self.ftp.PWD()
            self.log(pwd_res)
            self.remote.cwd = pwd_res[5:-1]
            self.remote.updateRemotePath(pwd_res[5:-1])
        else:
            self.log(self.ftp.QUIT())
            self.remote.pathEditSetText('')
            self.remote.files.clear()
        
        self.connected = not self.connected
        self.connectButton.setText('Connect' if not self.connected else 'Disconnect')

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

        self.connectButton = QPushButton()
        self.connectButton.setText('Connect' if not self.connected else 'Disconnect')
        loginLayout.addWidget(self.connectButton)

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
        self.queue = QListWidget()
        queueLayout.addWidget(self.queue)
        return queueLayout


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    client = MainWindow(app)
    client.show()
    app.exec_()

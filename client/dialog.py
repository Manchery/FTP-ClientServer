from PyQt5.QtWidgets import *
from PyQt5.QtCore import Qt

class InputDialog(QDialog):
    def __init__(self, title, label, edit=True, default='', parent=None):
        super(InputDialog, self).__init__(parent)

        self.label = QLabel(label)
        if edit:
            self.edit = QLineEdit(default)

        self.mainLayout = QVBoxLayout()
        self.mainLayout.addWidget(self.label, stretch=2)
        if edit:
            self.mainLayout.addWidget(self.edit)

        self.buttonBox = QDialogButtonBox()
        self.buttonBox.setStandardButtons(
            QDialogButtonBox.Cancel | QDialogButtonBox.Ok)
        self.mainLayout.addWidget(self.buttonBox, stretch=2)

        self.setLayout(self.mainLayout)

        self.setWindowTitle(title)

        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        self.setWindowModality(Qt.ApplicationModal)
        self.show()
        self.accepted = self.exec_()


def inputDialog(title, label, default='', parent=None):
    input = InputDialog(title, label, True, default=default, parent=parent)
    if not input.accepted:
        return False
    else:
        return input.edit.text()

def confirmDialog(title, label, parent=None):
    input = InputDialog(title, label, False, parent=parent)
    if not input.accepted:
        return False
    else:
        return True

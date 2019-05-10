import sys

from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow, QLabel
from PyQt5.QtGui import QPainter, QColor, QPen, QPixmap
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import Qt, QObject, QRunnable, QMetaObject, QSocketNotifier, pyqtSlot, pyqtSignal

from . import bagl

BUTTON_LEFT  = 1
BUTTON_RIGHT = 2

class PaintWidget(QWidget):
    def __init__(self,parent):
        super(PaintWidget, self).__init__(parent)
        self.mPixmap = QPixmap()
        self.pixels = {}

    def paintEvent(self, event):
        if self.pixels:
            pixmap = QPixmap(self.size())
            pixmap.fill(Qt.white)
            painter = QPainter(pixmap)
            painter.drawPixmap(0, 0, self.mPixmap)
            self._redraw(painter)
            self.mPixmap = pixmap
            self.pixels = {}

        qp = QPainter(self)
        qp.drawPixmap(0, 0, self.mPixmap)

    def _redraw(self, qp):
        for (x, y), color in self.pixels.items():
            qp.setPen(QColor.fromRgb(color))
            qp.drawPoint(x, y)

    def draw_point(self, x, y, color):
        self.pixels[(x, y)] = color

class Screen(QMainWindow):
    COLORS = {
        'LAGOON_BLUE': 0x7ebab5,
        'JADE_GREEN': 0xb9ceac,
        'FLAMINGO_PINK': 0xd8a0a6,
        'SAFFRON_YELLOW': 0xf6a950,
        'MATTE_BLACK': 0x111111,

        'CHARLOTTE_PINK': 0xff5555,
        'ARNAUD_GREEN': 0x79ff79,
        'SYLVE_CYAN': 0x29f3f3,
    }

    def __init__(self, apdu, seph, color, width=128, height=32):
        self.apdu = apdu
        self.seph = seph

        self.width = width
        self.height = height
        
        super().__init__()

        self._init_notifiers([ apdu, seph ])

        self.setWindowTitle('Nano Emulator')
        box_size_x, box_size_y = 100, 26
        self.setGeometry(10, 10, self.width + box_size_x, self.height + box_size_y)
        self.setWindowFlags(Qt.FramelessWindowHint)
        
        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor.fromRgb(self.COLORS[color]))
        self.setPalette(p)

        #painter.drawEllipse(QPointF(x,y), radius, radius);

        # Add paint widget and paint
        self.m = PaintWidget(self)
        self.m.move(20, box_size_y // 2)
        self.m.resize(self.width, self.height)

        self.setWindowIcon(QIcon('mcu/icon.png'))

        self.show()

        self.bagl = bagl.Bagl(self.m, width, height, color)

    def add_notifier(self, klass):
        n = QSocketNotifier(klass.s.fileno(), QSocketNotifier.Read, self)
        n.activated.connect(lambda s: klass.can_read(s, self))

        assert klass.s.fileno() not in self.notifiers
        self.notifiers[klass.s.fileno()] = n

    def _init_notifiers(self, classes):
        self.notifiers = {}
        for klass in classes:
            self.add_notifier(klass)

    def enable_notifier(self, fd, enabled=True):
        n = self.notifiers[fd]
        n.setEnabled(enabled)

    def remove_notifier(self, fd):
        # just in case
        self.enable_notifier(fd, False)

        n = self.notifiers.pop(fd)
        n.disconnect()
        del n

    def forward_to_app(self, packet):
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet):
        self.apdu.forward_to_client(packet)

    def _key_event(self, event, pressed):
        key = event.key()
        if key in [ Qt.Key_Left, Qt.Key_Right ]:
            buttons = { Qt.Key_Left: BUTTON_LEFT, Qt.Key_Right: BUTTON_RIGHT }
            # forward this event to seph
            self.seph.handle_button(buttons[key], pressed)

    def keyPressEvent(self, event):
        self._key_event(event, True)

    def keyReleaseEvent(self, event):
        self._key_event(event, False)

    def display_status(self, data):
        self.bagl.display_status(data)

    def screen_update(self):
        self.bagl.refresh()

    def mousePressEvent(self, event):
        '''Get the mouse location.'''

        self.mouse_offset = event.pos()
        QApplication.setOverrideCursor(Qt.DragMoveCursor)

    def mouseReleaseEvent(self, event):
        QApplication.restoreOverrideCursor()

    def mouseMoveEvent(self, event):
        '''Move the window.'''

        x = event.globalX()
        y = event.globalY()
        x_w = self.mouse_offset.x()
        y_w = self.mouse_offset.y()
        self.move(x - x_w, y - y_w)

def display(apdu, seph, color='MATTE_BLACK'):
    app = QApplication(sys.argv)
    display = Screen(apdu, seph, color)
    app.exec_()
    app.quit()

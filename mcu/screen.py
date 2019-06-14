import sys

from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow, QLabel
from PyQt5.QtGui import QPainter, QColor, QPen, QPixmap
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import Qt, QObject, QRunnable, QMetaObject, QSocketNotifier, pyqtSlot, pyqtSignal

from . import bagl
from .display import Display, COLORS, MODELS

BUTTON_LEFT  = 1
BUTTON_RIGHT = 2


class PaintWidget(QWidget):
    def __init__(self, parent, model):
        super(PaintWidget, self).__init__(parent)
        self.mPixmap = QPixmap()
        self.pixels = {}
        self.model = model

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
        # There are only 2 colors on Nano S but the one passed in argument isn't
        # always valid. Fix it here.
        if self.model == 'nanos' and color != 0x000000:
            color = 0x00fffb
        self.pixels[(x, y)] = color

class Screen(QMainWindow, Display):
    def __init__(self, apdu, seph, color, model, ontop):
        self.apdu = apdu
        self.seph = seph

        self.width, self.height = MODELS[model].screen_size
        self.box_position_x, self.box_position_y = MODELS[model].box_position
        box_size_x, box_size_y = MODELS[model].box_size

        super().__init__()

        self._init_notifiers([ apdu, seph ])

        self.setWindowTitle('Ledger %s Emulator' % MODELS[model].name)
        self.setGeometry(10, 10, self.width + box_size_x, self.height + box_size_y)

        flags = Qt.FramelessWindowHint
        if ontop:
            flags |= Qt.CustomizeWindowHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(flags)

        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor.fromRgb(COLORS[color]))
        self.setPalette(p)

        #painter.drawEllipse(QPointF(x,y), radius, radius);

        # Add paint widget and paint
        self.m = PaintWidget(self, model)
        self.m.move(self.box_position_x, self.box_position_y)
        self.m.resize(self.width, self.height)

        self.setWindowIcon(QIcon('mcu/icon.png'))

        self.show()

        self.bagl = bagl.Bagl(self.m, self.width, self.height, color)

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

    def display_raw_status(self, data):
        self.bagl.display_raw_status(data)

    def screen_update(self):
        self.bagl.refresh()

    def mousePressEvent(self, event):
        '''Get the mouse location.'''

        self.mouse_offset = event.pos()

        self.seph.handle_finger(self.mouse_offset.x(), self.mouse_offset.y(), True)
        QApplication.setOverrideCursor(Qt.DragMoveCursor)

    def mouseReleaseEvent(self, event):
        x = self.mouse_offset.x() - (self.box_position_x + 1)
        y = self.mouse_offset.y() - (self.box_position_y + 1)
        if x >= 0 and x < self.width and y >= 0 and y < self.height:
            self.seph.handle_finger(x, y, False)
        QApplication.restoreOverrideCursor()

    def mouseMoveEvent(self, event):
        '''Move the window.'''

        x = event.globalX()
        y = event.globalY()
        x_w = self.mouse_offset.x()
        y_w = self.mouse_offset.y()
        self.move(x - x_w, y - y_w)

def display(apdu, seph, color='MATTE_BLACK', model='nanos', ontop=False):
    app = QApplication(sys.argv)
    display = Screen(apdu, seph, color, model, ontop)
    app.exec_()
    app.quit()

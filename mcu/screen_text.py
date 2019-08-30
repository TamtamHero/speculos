import select 
from . import bagl
from .display import Display, MODELS 

BUTTON_LEFT  = 1
BUTTON_RIGHT = 2

_TEXT_ = "\033[36;40m"
_BORDER_ = "\033[30;1;40m"
_RESET_COLOR = "\033[0m"

M = [0]*16
M[0b0000] = ' '
M[0b0001] = '\u2598'
M[0b0010] = '\u259D'
M[0b0011] = '\u2580'
M[0b0100] = '\u2596'
M[0b0101] = '\u258C'
M[0b0110] = '\u259E'
M[0b0111] = '\u259B'
M[0b1000] = '\u2597'
M[0b1001] = '\u259A'
M[0b1010] = '\u2590'
M[0b1011] = '\u259C'
M[0b1100] = '\u2584'
M[0b1101] = '\u2599'
M[0b1110] = '\u259F'
M[0b1111] = '\u2588'

# a b
# c d 
def map_pix(a,b,c,d):
    return M[d<<3 | c<<2 | b<<1 | a]
    
class TextWidget:
    def __init__(self, parent, model):
        self.pixels = {}
        self.model = model
        self.width = parent.width
        self.height = parent.height
        self.previous_screen = 0

    def _redraw(self):
        p = self.pixels
        if p != self.previous_screen:
            self.previous_screen = p
        else:
            return
        f = lambda x,y:p.get((x,y),0)
        w = self.width // 2 + 1
        s = '\n' + _BORDER_ + '+' + '-'*w + '+\n'
        for i in range(0,self.height-2, 2):
            line = []
            for j in range(0, self.width-2,2):
                line.append(map_pix(f(j,i), f(j+1,i),f(j,i+1), f(j+1,i+1)))
            s += '| '+_TEXT_+''.join(line) + _BORDER_ + ' |\n'
        s += '+' + '-'*w + '+' + _RESET_COLOR + '\n'
        print(s)

    def draw_point(self, x, y, color):
        self.pixels[(x, y)] = color!=0 

class TextScreen(Display):
    def __init__(self, apdu, seph, color, model, ontop):
        self.apdu = apdu
        self.seph = seph
        self.model = model
        self.notifiers = {}

        self.width, self.height = MODELS[model].screen_size

        self.m = TextWidget(self, model)
        self.bagl = bagl.Bagl(self.m, self.width, self.height, color)

        self._init_notifiers([ apdu, seph ])

    def display_status(self, data):
        self.bagl.display_status(data)

    def screen_update(self):
        self.m._redraw()
        
    def _init_notifiers(self, classes):
        for klass in classes:
            self.add_notifier(klass)

    def forward_to_app(self, packet):
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet):
        self.apdu.forward_to_client(packet)

    def add_notifier(self, klass):
        assert klass.s.fileno() not in self.notifiers
        self.notifiers[klass.s.fileno()] = klass

    def remove_notifier(self, fd):
        n = self.notifiers.pop(fd)
        del n

    def run(self):
        while True:
            rlist = self.notifiers.keys()
            if not rlist:
                break

            rlist, _, _ = select.select(rlist, [], [])
            for fd in rlist:
                self.notifiers[fd].can_read(fd, self)


def display(apdu, seph, color='MATTE_BLACK', model='nanos', ontop=False):
    display = TextScreen(apdu, seph, color, model, ontop)
    display.run()


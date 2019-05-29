from collections import namedtuple

Model = namedtuple('Model', 'name screen_size box_size')
MODELS = {
    'nanos': Model('Nano S', (128, 32), (100, 26)),
    'blue': Model('Blue', (320, 480), (36, 26)),
}

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

class Display:
    def __init__(self):
        pass

    def display_status(self, data):
        pass

    def display_raw_status(self, data):
        pass

    def screen_update(self):
        pass

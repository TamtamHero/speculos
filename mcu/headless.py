import select

from .display import Display

class Headless(Display):
    def __init__(self, apdu, seph):
        self.apdu = apdu
        self.seph = seph
        self.notifiers = {}
        
        self._init_notifiers([ apdu, seph ])

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
        
def display(apdu, seph, color='MATTE_BLACK', model='nanos'):
    headless = Headless(apdu, seph)
    headless.run()

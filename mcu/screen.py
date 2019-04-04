import binascii
import ctypes
from construct import *
import os
import threading
import sdl2
import sdl2.ext
import sys

from . import bagl_font
from . import bagl_glyph
from . import icon

SDL_WINDOWEVENT_TAKE_FOCUS = 15

BAGL_FILL = 1

BUTTON_LEFT  = 1
BUTTON_RIGHT = 2

BAGL_NONE           = 0
BAGL_BUTTON         = 1
BAGL_LABEL          = 2
BAGL_RECTANGLE      = 3
BAGL_LINE           = 4
BAGL_ICON           = 5
BAGL_CIRCLE         = 6
BAGL_LABELINE       = 7
BAGL_FLAG_TOUCHABLE = 0x80

BAGL_FONT_ALIGNMENT_HORIZONTAL_MASK = 0xC000
BAGL_FONT_ALIGNMENT_LEFT            = 0x0000
BAGL_FONT_ALIGNMENT_RIGHT           = 0x4000
BAGL_FONT_ALIGNMENT_CENTER          = 0x8000
BAGL_FONT_ALIGNMENT_VERTICAL_MASK   = 0x3000
BAGL_FONT_ALIGNMENT_TOP             = 0x0000
BAGL_FONT_ALIGNMENT_BOTTOM          = 0x1000
BAGL_FONT_ALIGNMENT_MIDDLE          = 0x2000

bagl_component_t = Aligned(4, Struct(
    "type"    / Int8ul,
    "userid"  / Int8ul,
    "x"       / Int16ul,
    "y"       / Int16ul,
    "width"   / Int16ul,
    "height"  / Int16ul,
    "stroke"  / Int8ul,
    "radius"  / Int8ul,
    "fill"    / Padded(4, Int8ul),
    "fgcolor" / Int32ul,
    "bgcolor" / Int32ul,
    "font_id" / Int16ul,
    "icon_id" / Int8ul,
))

class Bagl:
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

    def __init__(self, window, width, height, color, offset_x=0, offset_y=0):
        self.window = window
        self.SCREEN_WIDTH = width
        self.SCREEN_HEIGHT = height
        self.offset_x = offset_x
        self.offset_y = offset_y

        '''self.screen_draw_x = 0
        self.screen_draw_y = 0
        self.screen_draw_width = 0
        self.screen_draw_height = 0
        self.screen_draw_YX = 0
        self.screen_draw_YXlinemax = 0
        self.screen_draw_Ybitmask = 0
        self.screen_draw_colors = []'''
        self.screen_framebuffer = [ 0 ] * ((self.SCREEN_WIDTH * self.SCREEN_HEIGHT) // 8)

        self._draw_box(color)

    def _draw_box(self, color):
        '''
        Draw the Nano box. Fancy feature.
        Circle from https://stackoverflow.com/a/41902448
        '''

        if self.offset_x == 0 and self.offset_y == 0:
            return

        windowsurface = self.window.get_surface()
        sdl2.ext.draw.fill(windowsurface, self.COLORS[color])

        radius = self.SCREEN_HEIGHT // 2
        center_x = self.offset_x + self.SCREEN_WIDTH + 50
        center_y = self.offset_y + self.SCREEN_HEIGHT // 2

        pixelview = sdl2.ext.pixels2d(windowsurface)
        for w in range(radius * 2):
            for h in range(radius * 2):
                dx = radius - w
                dy = radius - h
                if dx * dx + dy * dy <= radius * radius:
                    pixelview[center_x + dx][center_y + dy] = 0x000000
        del pixelview
        self.window.refresh()

    def refresh(self):
        windowsurface = self.window.get_surface()
        pixelview = sdl2.ext.pixels2d(windowsurface)
        for y in range(self.SCREEN_HEIGHT // 8):
            for x in range(self.SCREEN_WIDTH):
                v = self.screen_framebuffer[y*self.SCREEN_WIDTH+x]
                for i in range(8):
                    c = (v>>i) & 1
                    if c:
                        pixelview[self.offset_x + x][self.offset_y + 8*y+i] = 0x00fffb
                    else:
                        pixelview[self.offset_x + x][self.offset_y + 8*y+i] = 0x000000
        del pixelview
        self.window.refresh()

    def hal_draw_bitmap_within_rect_internal(self, bpp, bitmap, bitmap_length_bits):
        x = self.screen_draw_x
        xx = x
        y = self.screen_draw_y
        width = self.screen_draw_width
        height = self.screen_draw_height
        YX = self.screen_draw_YX
        YXlinemax = self.screen_draw_YXlinemax
        Ybitmask = self.screen_draw_Ybitmask
        colors = self.screen_draw_colors
        pixel_mask = 1

        while bitmap_length_bits > 0 and height != 0:
            ch = bitmap[0]
            bitmap = bitmap[1:]
            for i in range(0, 8, bpp):
                if bitmap_length_bits == 0:
                    break
                if y >= 0 and xx >= 0:
                    if colors[(ch>>i) & pixel_mask] != 0:
                        if YX < len(self.screen_framebuffer):
                            self.screen_framebuffer[YX] |= Ybitmask
                        else:
                            # XXX: handle alignment
                            #print('bug')
                            pass
                    else:
                        if YX < len(self.screen_framebuffer):
                            self.screen_framebuffer[YX] &= ~Ybitmask
                        else:
                            # XXX: handle alignment
                            #print('bug')
                            pass

                bitmap_length_bits -= bpp

                xx += 1
                YX += 1
                if YX >= YXlinemax:
                    y += 1
                    height -= 1
                    YX = (y // 8) * self.SCREEN_WIDTH + x
                    xx = x
                    YXlinemax = YX + width
                    Ybitmask = 1 << (y % 8)

                if height == 0:
                    break

        self.screen_draw_x = x
        self.screen_draw_y = y
        self.screen_draw_width = width
        self.screen_draw_height = height
        self.screen_draw_YX = YX
        self.screen_draw_YXlinemax = YXlinemax
        self.screen_draw_Ybitmask = Ybitmask

    def hal_draw_bitmap_within_rect(self, x, y, width, height, colors, bpp, bitmap, bitmap_length_bits):
        if x >= self.SCREEN_WIDTH or y >= self.SCREEN_HEIGHT:
            return

        if x + width > self.SCREEN_WIDTH:
            width = self.SCREEN_WIDTH - x

        if y + height > self.SCREEN_HEIGHT:
            height = self.SCREEN_HEIGHT - y

        YX = (y // 8) * self.SCREEN_WIDTH + x
        Ybitmask = 1 << (y % 8)
        YXlinemax = YX + width

        self.screen_draw_x = x
        self.screen_draw_y = y
        self.screen_draw_width = width
        self.screen_draw_height = height
        self.screen_draw_YX = YX
        self.screen_draw_YXlinemax = YXlinemax
        self.screen_draw_Ybitmask = Ybitmask
        self.screen_draw_colors = colors

        self.hal_draw_bitmap_within_rect_internal(bpp, bitmap, bitmap_length_bits)

    def hal_draw_rect(self, color, x, y, width, height):
        if x + width > self.SCREEN_WIDTH or x < 0:
            return
        if y + height > self.SCREEN_HEIGHT or y < 0:
            return

        YX = (y//8)*self.SCREEN_WIDTH + x
        Ybitmask = 1<<(y%8)
        YXlinemax = YX + width

        i = width * height
        while i > 0:
            i -= 1
            if color:
                self.screen_framebuffer[YX] |= Ybitmask
            else:
                self.screen_framebuffer[YX] &= ~Ybitmask
            YX += 1
            if YX >= YXlinemax:
                y += 1
                height -= 1
                YX = (y // 8) * self.SCREEN_WIDTH + x
                YXlinemax = YX + width
                Ybitmask = 1<<(y%8)
            if height == 0:
                break

    def compute_line_width(font_id, width, text, text_encoding):
        font = bagl_font.get(font_id)
        if not font:
            return 0

        xx = 0

        text = text.replace(b'\r\n', b'\n')
        for ch in text:
            ch_width = 0

            if ch < font.first_char or ch > font.last_char:
                if ch in [ '\r', '\n' ]:
                    return xx

                if ch >= 0xc0:
                    ch_width = ch & 0x3f
                elif ch >= 0x80:
                    if ch & 0x20:
                        font_symbol_id = BAGL_FONT_SYMBOLS_1
                    else:
                        font_symbol_id = BAGL_FONT_SYMBOLS_0
                    font_symbols = bagl_font.get(font_symbol_id)
                    if font_symbols:
                        ch_width = font.characters[ch & 0x1f].char_width
            else:
                ch -= font.first_char
                ch_width = font.characters[ch].char_width
                ch_kerning = font.char_kerning

            if xx + ch_width > width and width > 0:
                return xx

            xx += ch_width

        return xx


    def draw_string(self, font_id, fgcolor, bgcolor, x, y, width, height, text, text_encoding):
        font = bagl_font.get(font_id)
        if not font:
            return 0

        colors = [ bgcolor, fgcolor ]
        if font.bpp > 1:
            # TODO
            pass

        width += x
        height += y
        xx = x

        text = text.replace(b'\r\n', b'\n')

        for ch in text:
            ch_height = font.char_height
            ch_kerning = 0
            ch_width = 0
            ch_bitmap = None
            ch_y = y

            if ch < font.first_char or ch > font.last_char:
                if ch in [ '\r', '\n' ]:
                    y += ch_height
                    if y + ch_height > height:
                        return (y << 16) | (xx & 0xFFFF)
                    xx = x
                    continue

                if ch >= 0xc0:
                    ch_width = ch & 0x3f
                elif ch >= 0x80:
                    if ch & 0x20:
                        font_symbol_id = BAGL_FONT_SYMBOLS_1
                    else:
                        font_symbol_id = BAGL_FONT_SYMBOLS_0
                    font_symbols = bagl_font.get(font_symbol_id)
                    if font_symbols:
                        ch_bitmap = font.bitmap[font.characters[ch & 0x1f].bitmap_offset:]
                        ch_width = font.characters[ch & 0x1f].char_width
                        ch_height = font_symbols.char_height
                        ch_y = y + font.baseline_height - font_symbols.baseline_height
            else:
                ch -= font.first_char
                ch_bitmap = font.bitmap[font.characters[ch].bitmap_offset:]
                ch_width = font.characters[ch].char_width
                ch_kerning = font.char_kerning

            if xx + ch_width > width:
                y += ch_height
                if y + ch_height > height:
                    return (y << 16 ) | (xx & 0xFFFF)
                xx = x
                ch_y = y

            if ch_bitmap:
                self.hal_draw_bitmap_within_rect(xx, ch_y, ch_width, ch_height, colors, font.bpp, ch_bitmap, font.bpp * ch_width * ch_height)
            else:
                self.hal_draw_rect(bgcolor, xx, ch_y, ch_width, ch_height)

            xx += ch_width + ch_kerning

        return (y << 16) | (xx & 0xFFFF)

def window_draggable(window, area, data):
    '''
    This callback tells that any part of the window should be seen as draggable
    by the window manager. It allows the borderless window to be moved using a
    left click.
    '''
    return sdl2.SDL_HITTEST_DRAGGABLE

class Screen:
    def __init__(self, color, width=128, height=32):
        self.width = width
        self.height = height
        box_size_x, box_size_y = 100, 26

        self.events = []

        sdl2.ext.init()
        self.window = sdl2.ext.Window("Nano Emulator",
                                      size=(self.width + box_size_x, self.height + box_size_y),
                                      flags=sdl2.SDL_WINDOW_BORDERLESS)
        self.window.show()

        sdl2.video.SDL_SetWindowHitTest(self.window.window, sdl2.video.SDL_HitTest(window_draggable), None)

        image = sdl2.ext.surface.SDL_CreateRGBSurfaceFrom(icon.pxbuf, icon.width, icon.height,
                                                          icon.depth, icon.pitch, icon.rmask,
                                                          icon.gmask, icon.bmask, icon.amask)
        sdl2.video.SDL_SetWindowIcon(self.window.window, image)

        self.bagl = Bagl(self.window, width, height, color, 20, box_size_y // 2)

    def _display_bagl_icon(self, component, context):
        if component.icon_id != 0:
            #print('[*] icon_id', component.icon_id)
            glyph = bagl_glyph.get(component.icon_id)
            if not glyph:
                print('[-] bagl glyph 0x%x not found' % component.icon_id)
                return

            if len(context) != 0:
                assert (1 << glyph.bpp) * 4 == len(context)
                colors = []
                for i in range(0, 1 << bpp):
                    color = int.from_bytes(context[i*4:(i+1)*4], byteorder='big')
                    colors.append(color)
            else:
                colors = glyph.colors

            self.bagl.hal_draw_bitmap_within_rect(
                component.x + (component.width // 2 - glyph.width // 2),
                component.y + (component.height // 2 - glyph.height // 2),
                glyph.width,
                glyph.height,
                colors,
                glyph.bpp,
                glyph.bitmap,
                glyph.bpp * (glyph.width * glyph.height))
        else:
            if len(context) == 0:
                print('len context == 0', binascii.hexlify(context))
                return

            bpp = context[0]
            if bpp > 2:
                return
            colors = []
            n = 1
            for i in range(0, 1 << bpp):
                color = int.from_bytes(context[n:n+4], byteorder='big')
                colors.append(color)
                n += 4
            bitmap = context[n:]
            bitmap_length_bits = bpp * component.width * component.height
            assert len(bitmap) * 8 >= bitmap_length_bits
            self.bagl.hal_draw_bitmap_within_rect(component.x, component.y,
                                                  component.width, component.height,
                                                  colors,
                                                  bpp,
                                                  bitmap, bitmap_length_bits)

    def _display_bagl_rectangle(self, component, context):
        radius = component.radius
        radius = min(radius, min(component.width//2, component.height//2))
        if component.fill != BAGL_FILL:
            coords = [
                # centered top to bottom
                (component.x+radius, component.y, component.width-2*radius, component.height),
                # left to center rect
                (component.x, component.y+radius, radius, component.height-2*radius),
                # center rect to right
                (component.x+component.width-radius-1, component.y+radius, radius, component.height-2*radius),
            ]
            for (x, y, width, height) in coords:
                self.bagl.hal_draw_rect(component.bgcolor, x, y, width, height)
            coords = [
                # outline. 4 rectangles (with last pixel of each corner not set)
                # top, bottom, left, right
                (component.x+radius, component.y, component.width-2*radius, component.stroke),
                (component.x+radius, component.y+component.height-1, component.width-2*radius, component.stroke),
                (component.x, component.y+radius, component.stroke, component.height-2*radius),
                (component.x+component.width-1, component.y+radius, component.stroke, component.height-2*radius),
            ]
            for (x, y, width, height) in coords:
                self.bagl.hal_draw_rect(component.fgcolor, x, y, width, height)
        else:
            coords = [
                # centered top to bottom
                (component.x+radius, component.y, component.width-2*radius, component.height),
                # left to center rect
                (component.x, component.y+radius, radius, component.height-2*radius),
                # center rect to right
                (component.x+component.width-radius, component.y+radius, radius, component.height-2*radius),
            ]
            for (x, y, width, height) in coords:
                self.bagl.hal_draw_rect(component.fgcolor, x, y, width, height)

    def _display_bagl_labeline(self, component, text, halignment, baseline):
        if len(text) == 0:
            return

        # XXX
        context_encoding = 0
        self.bagl.draw_string(component.font_id,
                              component.fgcolor,
                              component.bgcolor,
                              component.x + halignment,
                              component.y - baseline,
                              component.width - halignment,
                              component.height,
                              text,
                              context_encoding)
        self.bagl.refresh()

    def _display_get_alignment(self, component, context, context_encoding):
        halignment = 0
        valignment = 0
        baseline = 0
        char_height = 0
        strwidth = 0

        if component.type == BAGL_ICON:
            return (halignment, valignment, baseline, char_height, strwidth)

        font = bagl_font.get(component.font_id)
        if font:
            baseline = font.baseline_height
            char_height = font.char_height

            if context:
                strwidth = Bagl.compute_line_width(component.font_id,
                                                   component.width + 100,
                                                   context,
                                                   context_encoding)

        haligned = (component.font_id & BAGL_FONT_ALIGNMENT_HORIZONTAL_MASK)
        if haligned == BAGL_FONT_ALIGNMENT_RIGHT:
            halignment = component.width - strwidth
        elif haligned == BAGL_FONT_ALIGNMENT_CENTER:
            halignment = component.width // 2 - strwidth // 2
        elif haligned == BAGL_FONT_ALIGNMENT_LEFT:
            halignment = 0
        else:
            halignment = 0

        valigned = (component.font_id & BAGL_FONT_ALIGNMENT_VERTICAL_MASK)
        if valigned == BAGL_FONT_ALIGNMENT_BOTTOM:
            valignment = component.height - baseline
        elif valigned == BAGL_FONT_ALIGNMENT_MIDDLE:
            valignment = component.height // 2 - baseline // 2 - 1
        elif valigned == BAGL_FONT_ALIGNMENT_TOP:
            valignment = 0
        else:
            valignment = 0

        return (halignment, valignment, baseline, char_height, strwidth)

    def display_status(self, data):
        component = bagl_component_t.parse(data)
        context = data[bagl_component_t.sizeof():]
        context_encoding = 0 # XXX
        #print(component)

        ret = self._display_get_alignment(component, context, context_encoding)
        (halignment, valignment, baseline, char_height, strwidth) = ret

        if component.type == BAGL_NONE:
            # TODO
            #self.renderer.clear()
            pass
        elif component.type == BAGL_RECTANGLE:
            self._display_bagl_rectangle(component, context)
        elif component.type == BAGL_LABELINE:
            self._display_bagl_labeline(component, context, halignment, baseline)
        elif component.type == BAGL_ICON:
            self._display_bagl_icon(component, context)

    def screen_update(self):
        self.bagl.refresh()

    def add_events(self, events):
        self.events += events

    def handle_events(self):
        buttons = { sdl2.SDLK_LEFT: BUTTON_LEFT, sdl2.SDLK_RIGHT: BUTTON_RIGHT }
        results = []
        events = sdl2.ext.get_events()
        while self.events:
            event = self.events.pop(0)
            if event.type == sdl2.SDL_QUIT:
                sys.exit(0)
            elif event.type in [ sdl2.SDL_KEYDOWN, sdl2.SDL_KEYUP ]:
                sym = event.key.keysym.sym
                if sym in buttons:
                    results.append((buttons[sym], event.type == sdl2.SDL_KEYDOWN))
            elif event.type == sdl2.SDL_WINDOWEVENT:
                if event.window.event == SDL_WINDOWEVENT_TAKE_FOCUS:
                    self.bagl.refresh()
        return results

class DisplayThread(threading.Thread):
    def __init__(self, pipe, color):
        threading.Thread.__init__(self)
        self.display = Screen(color)
        self.pipe = pipe
        self._stop_event = threading.Event()

    def stop(self):
        '''This method is called by the main thread.'''

        self._stop_event.set()

        # simulate an event to make SDL_WaitEvent return
        event = sdl2.SDL_Event(0, sdl2.SDL_GenericEvent(sdl2.SDL_QUIT))
        sdl2.events.SDL_PushEvent(event)

    def run(self):
        while not self._stop_event.is_set():
            events = sdl2.ext.get_events()
            self.display.add_events(events)

            # notify the other thread
            os.write(self.pipe, b'x')

            sdl2.SDL_WaitEvent(None)



class HeadlessDisplay:
    def __init__(self):
        pass

    def handle_events(self, *args):
        pass

    def screen_update(self, *args):
        pass

    def display_status(self, data):
        component = bagl_component_t.parse(data)
        context = data[bagl_component_t.sizeof():]
        if len(context) > 0:
            print('[*] display:', context.decode('utf-8'))

class HeadlessDisplayThread:
    def __init__(self, *args):
        self.display = HeadlessDisplay()

    def start(self):
        pass

    def stop(self):
        pass

    def join(self):
        pass

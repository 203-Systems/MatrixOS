# Python port based on aarongeorge/MatrixOSApp-Canvas:
# https://github.com/aarongeorge/MatrixOSApp-Canvas
import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS
UI = MatrixOS.UI

WIDTH = 8
HEIGHT = 8
CANVAS_WIDTH = WIDTH + 2
FUNCTION_KEY = Input.function_key()
ART_KEY = "Python Pixel Art grid"
LEFT_SIDE_X = -1
RIGHT_SIDE_X = WIDTH

STATE_PRESSED = MatrixOS.Input.STATE_PRESSED
STATE_HOLD = MatrixOS.Input.STATE_HOLD
STATE_RELEASED = MatrixOS.Input.STATE_RELEASED

color_grid = [[0x000000 for _ in range(HEIGHT)] for _ in range(CANVAS_WIDTH)]
active_color = 0xFFFFFF
running = True
underglow_enabled = LED.get_partition("Underglow") is not None


def canvas_x(x):
    return x + 1


def unpack_color(data, offset):
    return (data[offset] << 16) | (data[offset + 1] << 8) | data[offset + 2]


def load_art():
    data = NVS.get(ART_KEY)
    if data is None or len(data) != CANVAS_WIDTH * HEIGHT * 3:
        return

    offset = 0
    for y in range(HEIGHT):
        for x in range(CANVAS_WIDTH):
            color_grid[x][y] = unpack_color(data, offset)
            offset += 3


def save_art():
    data = bytearray()
    for y in range(HEIGHT):
        for x in range(CANVAS_WIDTH):
            color = color_grid[x][y]
            data.append((color >> 16) & 0xFF)
            data.append((color >> 8) & 0xFF)
            data.append(color & 0xFF)
    NVS.set(ART_KEY, data)


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


def render():
    for y in range(HEIGHT):
        for x in range(WIDTH):
            LED.set_xy(x, y, color_grid[canvas_x(x)][y])
        LED.set_xy(LEFT_SIDE_X, y, color_grid[canvas_x(LEFT_SIDE_X)][y])
        LED.set_xy(RIGHT_SIDE_X, y, color_grid[canvas_x(RIGHT_SIDE_X)][y])
    if underglow_enabled:
        LED.fill_partition("Underglow", active_color)
    LED.update()


def pick_color():
    global active_color
    picked = UI.color_picker(active_color)
    if picked is not None:
        active_color = picked
    Input.clear()
    render()


def paint_point(x, y):
    color_grid[canvas_x(x)][y] = active_color
    save_art()
    LED.set_xy(x, y, active_color)
    if underglow_enabled:
        LED.fill_partition("Underglow", active_color)
    LED.update()


def handle_function_key(state):
    global running

    if state == STATE_HOLD:
        running = False
        SYS.exit_app()
    elif state == STATE_RELEASED:
        pick_color()


def handle_event(event):
    state = keypad_state(event)

    if event.get("id") == FUNCTION_KEY:
        handle_function_key(state)
        return

    if state != STATE_PRESSED:
        return

    point = event.get("point")
    if point is None:
        return

    x, y = point
    if LEFT_SIDE_X <= x <= RIGHT_SIDE_X and 0 <= y < HEIGHT:
        paint_point(x, y)


def loop():
    if not running:
        return

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()


load_art()
render()
Input.clear()

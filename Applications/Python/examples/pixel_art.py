import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS

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

PICKER_COLORS = [
    0xFF0000,
    0xFF8000,
    0xFFFF00,
    0x00FF00,
    0x00FFFF,
    0x0000FF,
    0xFF00FF,
    0xFFFFFF,
]

color_grid = [[0x000000 for _ in range(HEIGHT)] for _ in range(CANVAS_WIDTH)]
active_color = 0xFFFFFF
picker_showing = True
running = True


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
            if picker_showing and y == 0:
                LED.set_xy(x, y, PICKER_COLORS[x])
            else:
                LED.set_xy(x, y, color_grid[canvas_x(x)][y])
        LED.set_xy(LEFT_SIDE_X, y, color_grid[canvas_x(LEFT_SIDE_X)][y])
        LED.set_xy(RIGHT_SIDE_X, y, color_grid[canvas_x(RIGHT_SIDE_X)][y])
    LED.update()


def show_picker():
    global picker_showing
    picker_showing = True
    render()


def hide_picker():
    global picker_showing
    picker_showing = False
    render()


def paint_point(x, y):
    global active_color

    if picker_showing and y == 0:
        active_color = PICKER_COLORS[x]
        render()
        return

    color_grid[canvas_x(x)][y] = active_color
    save_art()
    LED.set_xy(x, y, active_color)
    LED.update()


def handle_function_key(state):
    global running

    if state == STATE_HOLD:
        running = False
        SYS.exit_app()
    elif state == STATE_RELEASED:
        if picker_showing:
            hide_picker()
        else:
            show_picker()


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
show_picker()
Input.clear()

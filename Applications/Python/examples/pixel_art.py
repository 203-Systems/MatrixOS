import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
Color = MatrixOS.Color


WIDTH = 8
HEIGHT = 8
CANVAS_WIDTH = WIDTH + 2
BLACK_INDEX = 7

PICKER_COLORS = [
    Color(0xFF0000),
    Color(0xFFFF00),
    Color(0x00FF00),
    Color(0x00FFFF),
    Color(0x0000FF),
    Color(0xFF00FF),
    Color(0xFFFFFF),
    Color(0x000000),
]

active_color_index = 6
picker_visible = True
canvas = []
app_running = True
function_key = Input.function_key()


def canvas_index(x, y):
    return (x + 1) * HEIGHT + y


def in_canvas(x, y):
    return x >= -1 and x <= WIDTH and y >= 0 and y < HEIGHT


def reset_canvas():
    global canvas

    canvas = []
    for index in range(CANVAS_WIDTH * HEIGHT):
        canvas.append(BLACK_INDEX)


def draw_canvas_cell(x, y):
    LED.set_color_xy(x, y, PICKER_COLORS[canvas[canvas_index(x, y)]])


def draw_picker():
    if not picker_visible:
        return

    for x in range(WIDTH):
        LED.set_color_xy(x, 0, PICKER_COLORS[x])


def draw_canvas():
    for x in range(-1, WIDTH + 1):
        for y in range(HEIGHT):
            draw_canvas_cell(x, y)

    draw_picker()
    LED.show()


def toggle_picker():
    global picker_visible

    picker_visible = not picker_visible
    draw_canvas()


def paint_cell(x, y):
    canvas[canvas_index(x, y)] = active_color_index
    draw_canvas()


def select_or_paint(x, y):
    global active_color_index

    if not in_canvas(x, y):
        return

    if picker_visible and y == 0 and x >= 0 and x < WIDTH:
        active_color_index = x
    else:
        paint_cell(x, y)


def handle_function_key(event):
    global app_running

    if event.is_hold():
        app_running = False
        SYS.exit_app()
    elif event.is_released():
        toggle_picker()


def handle_input(event):
    if event.id() == function_key:
        handle_function_key(event)
        return

    if not event.is_pressed():
        return

    xy = Input.try_get_point(event.id())
    if xy is not None:
        select_or_paint(xy.x(), xy.y())


def process_input():
    event = Input.get_event()
    while event is not None:
        handle_input(event)
        event = Input.get_event()


def loop():
    if app_running:
        process_input()


reset_canvas()
LED.fill(PICKER_COLORS[BLACK_INDEX])
draw_canvas()
Input.clear_input_buffer()

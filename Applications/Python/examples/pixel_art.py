import MatrixOS


PICKER_COLORS = [
    MatrixOS.Color(0xFF0000),
    MatrixOS.Color(0xFFFF00),
    MatrixOS.Color(0x00FF00),
    MatrixOS.Color(0x00FFFF),
    MatrixOS.Color(0x0000FF),
    MatrixOS.Color(0xFF00FF),
    MatrixOS.Color(0xFFFFFF),
    MatrixOS.Color(0x000000),
]

BLACK_INDEX = 7
active_color_index = 6
picker_showing = True
color_grid = []
app_running = True
function_key = MatrixOS.Input.function_key()


def setup_grid():
    global color_grid
    color_grid = []
    for index in range(80):
        color_grid.append(BLACK_INDEX)


def grid_index(x):
    return x + 1


def color_index(x, y):
    return grid_index(x) * 8 + y


def draw_cell(x, y, color_index_value):
    MatrixOS.LED.set_color_xy(x, y, PICKER_COLORS[color_index_value])


def draw_canvas():
    for x in range(-1, 9):
        for y in range(8):
            index = color_index(x, y)
            draw_cell(x, y, color_grid[index])

    if picker_showing:
        for x in range(8):
            draw_cell(x, 0, x)

    MatrixOS.LED.show()


def show_picker():
    global picker_showing
    picker_showing = True
    draw_canvas()


def hide_picker():
    global picker_showing
    picker_showing = False
    draw_canvas()


def paint_cell(x, y):
    index = color_index(x, y)
    color_grid[index] = active_color_index
    draw_canvas()


def handle_key_event(event):
    global active_color_index
    global app_running

    if event.id() == function_key:
        if event.is_hold():
            app_running = False
            MatrixOS.SYS.exit_app()
            return
        if event.is_released():
            if picker_showing:
                hide_picker()
            else:
                show_picker()
        return

    if not event.is_pressed():
        return

    xy = MatrixOS.Input.try_get_point(event.id())
    if xy is None:
        return

    x = xy.x()
    y = xy.y()

    if x < -1 or x > 8 or y < 0 or y >= 8:
        return

    if x >= 0 and x < 8 and picker_showing and y == 0:
        active_color_index = x
    else:
        paint_cell(x, y)


def loop():
    if not app_running:
        return

    event = MatrixOS.Input.get_event()
    while event is not None:
        handle_key_event(event)
        event = MatrixOS.Input.get_event()


setup_grid()
MatrixOS.LED.fill(PICKER_COLORS[BLACK_INDEX])
show_picker()
MatrixOS.Input.clear_input_buffer()

import MatrixOS
from MatrixOS_Color import Color
from MatrixOS_KeyState import KeyState


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

BLACK_INDEX = 7
active_color_index = 6
picker_showing = True
color_grid = []


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

    state = event.key_state()
    if event.is_function_key():
        if state == KeyState.HOLD:
            return False

        if state == KeyState.RELEASED:
            if picker_showing:
                hide_picker()
            else:
                show_picker()
        return True

    if state != KeyState.PRESSED:
        return True

    if not event.is_grid():
        return True

    x = event.x()
    y = event.y()

    if x < -1 or x > 8 or y < 0 or y >= 8:
        return True

    if x >= 0 and x < 8 and picker_showing and y == 0:
        active_color_index = x
    else:
        paint_cell(x, y)

    return True


def loop():
    event = main_ui.pull_input()
    while event is not None:
        if not handle_key_event(event):
            main_ui.exit()
            return
        event = main_ui.pull_input()


setup_grid()
MatrixOS.LED.fill(PICKER_COLORS[BLACK_INDEX])
show_picker()

main_ui = MatrixOS.UI.UI("PixelArt", Color(0xFFFFFF))
main_ui.allow_exit(False)
main_ui.set_fps(60)
main_ui.set_loop_func(loop)
main_ui.start()

MatrixOS.LED.fill(PICKER_COLORS[BLACK_INDEX])
MatrixOS.LED.show()

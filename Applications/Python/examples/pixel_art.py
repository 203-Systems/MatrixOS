import MatrixOS
from MatrixOS_Color import Color
from MatrixOS_Point import Point
import MatrixOS_InputClass as InputClass
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
    MatrixOS.LED.SetColor(Point(x, y), PICKER_COLORS[color_index_value])


def draw_canvas():
    for x in range(-1, 9):
        for y in range(8):
            index = color_index(x, y)
            draw_cell(x, y, color_grid[index])

    if picker_showing:
        for x in range(8):
            draw_cell(x, 0, x)

    MatrixOS.LED.Update()


def show_picker():
    global picker_showing
    picker_showing = True
    draw_canvas()


def hide_picker():
    global picker_showing
    picker_showing = False
    draw_canvas()


def paint_position(position):
    x = position.X()
    y = position.Y()
    index = color_index(x, y)
    color_grid[index] = active_color_index
    draw_canvas()


def handle_keypad_event(event):
    global active_color_index

    if event.InputClass() != InputClass.KEYPAD:
        return True

    info = event.Keypad()
    if info is None:
        return True

    state = info.State()
    input_id = event.Id()

    if input_id == MatrixOS.Input.FunctionKey():
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

    position = MatrixOS.Input.GetPosition(input_id)
    if position is None:
        return True

    x = position.X()
    y = position.Y()

    if x < -1 or x > 8 or y < 0 or y >= 8:
        return True

    if x >= 0 and x < 8 and picker_showing and y == 0:
        active_color_index = x
    else:
        paint_position(position)

    return True


setup_grid()
MatrixOS.Input.ClearInputBuffer()
MatrixOS.LED.Fill(PICKER_COLORS[BLACK_INDEX])
show_picker()

while True:
    event = MatrixOS.Input.GetEvent(0)
    if event is not None:
        if not handle_keypad_event(event):
            break
    MatrixOS.SYS.DelayMs(16)

MatrixOS.LED.Fill(PICKER_COLORS[BLACK_INDEX])
MatrixOS.LED.Update()

import MatrixOS
import MatrixOS_UI
import MatrixOS_UIButton
from MatrixOS_Color import Color
from MatrixOS_Dimension import Dimension
from MatrixOS_Point import Point
import MatrixOS_ColorEffects as ColorEffects


WIDTH = 8
HEIGHT = 8
CELL_COUNT = WIDTH * HEIGHT

EMPTY = 0
SETUP_BOARD = 0
WAITING = 1
MOVING = 2
COMPACTING = 3
ENDED = 4
SETTINGS = 5

STEP_MS = 100

COLORS = [
    Color(0x000000),
    Color(0xFF0000),
    Color(0x00FF00),
    Color(0x0000FF),
    Color(0xFFFF00),
    Color(0xFF00FF),
]

SETTING_COLORS = [
    Color(0x222222),
    Color(0x662222),
    Color(0xAA2222),
    Color(0xEE2222),
]

board = []
marks = []
island_sizes = []
island_colors = []
num_colors = 4
score = 0
last_color = 4
game_state = SETUP_BOARD
last_event_ms = 0
random_seed = 1
main_ui = None
fn_was_active = False
board_buttons = []


def xy_index(x, y):
    return x + (y * WIDTH)


def in_bounds(x, y):
    return x >= 0 and x < WIDTH and y >= 0 and y < HEIGHT


def get_cell(x, y):
    if not in_bounds(x, y):
        return EMPTY
    return board[xy_index(x, y)]


def set_cell(x, y, color):
    if in_bounds(x, y):
        board[xy_index(x, y)] = color


def next_random():
    global random_seed
    random_seed = (random_seed * 1103515245 + 12345) & 0x7FFFFFFF
    return random_seed


def random_color():
    return ((next_random() // 65536) % num_colors) + 1


def seed_random():
    global random_seed
    random_seed = MatrixOS.SYS.Millis() & 0x7FFFFFFF
    if random_seed == 0:
        random_seed = 1


def reset_board():
    global board
    global marks
    global island_sizes
    global island_colors

    board = []
    marks = []
    island_sizes = []
    island_colors = []

    for index in range(CELL_COUNT):
        board.append(EMPTY)
        marks.append(255)
        island_sizes.append(0)
        island_colors.append(EMPTY)


def reset_game():
    global score
    global last_color
    global game_state
    global last_event_ms

    seed_random()
    reset_board()
    score = 0
    last_color = num_colors
    game_state = SETUP_BOARD
    last_event_ms = MatrixOS.SYS.Millis()


def get_cell_color(cell):
    if cell >= 0 and cell < len(COLORS):
        return COLORS[cell]
    return Color(0xFFFFFF)


def fill_underglow(color):
    MatrixOS.LED.FillPartition("Underglow", color)


def render_board_cells():
    for index in range(CELL_COUNT):
        MatrixOS.LED.SetColorByID(index, get_cell_color(board[index]))


def render_game():
    now = MatrixOS.SYS.Millis()
    sync_board_buttons()
    render_board_cells()

    if game_state == ENDED:
        fill_underglow(ColorEffects.Rainbow(1000, 0))
    elif game_state == MOVING:
        fill_underglow(ColorEffects.ColorSaw(get_cell_color(last_color), 250, now - last_event_ms))
    else:
        fill_underglow(ColorEffects.ColorBreath(get_cell_color(last_color), 2000, now - last_event_ms))


def selected_color_button(index):
    color_count = index + 2
    if color_count == num_colors:
        return Color(0xFFFFFF)
    return SETTING_COLORS[index]


def color_button_0():
    return selected_color_button(0)


def color_button_1():
    return selected_color_button(1)


def color_button_2():
    return selected_color_button(2)


def color_button_3():
    return selected_color_button(3)


def render_settings_background():
    for index in range(CELL_COUNT):
        MatrixOS.LED.SetColorByID(index, COLORS[EMPTY])
    fill_underglow(Color(0x00FFFF))


def render_ui():
    if game_state == SETTINGS:
        render_settings_background()
    else:
        render_game()


def build_islands():
    global marks
    global island_sizes
    global island_colors

    marks = []
    island_sizes = []
    island_colors = []

    for index in range(CELL_COUNT):
        marks.append(255)
        island_sizes.append(0)
        island_colors.append(EMPTY)

    island_count = 0

    for start in range(CELL_COUNT):
        if marks[start] != 255:
            continue

        search_color = board[start]
        island_colors[island_count] = search_color
        queue = [start]
        marks[start] = island_count
        cursor = 0

        while cursor < len(queue):
            index = queue[cursor]
            cursor += 1
            island_sizes[island_count] += 1

            x = index % WIDTH
            y = index // WIDTH
            add_same_color_neighbor(queue, island_count, search_color, x - 1, y)
            add_same_color_neighbor(queue, island_count, search_color, x + 1, y)
            add_same_color_neighbor(queue, island_count, search_color, x, y - 1)
            add_same_color_neighbor(queue, island_count, search_color, x, y + 1)

        island_count += 1


def add_same_color_neighbor(queue, island_id, color, x, y):
    if not in_bounds(x, y):
        return

    index = xy_index(x, y)
    if marks[index] == 255 and board[index] == color:
        marks[index] = island_id
        queue.append(index)


def island_at(x, y):
    build_islands()
    if not in_bounds(x, y):
        return 255
    return marks[xy_index(x, y)]


def island_size(island_id):
    if island_id >= 0 and island_id < len(island_sizes):
        return island_sizes[island_id]
    return 0


def island_color(island_id):
    if island_id >= 0 and island_id < len(island_colors):
        return island_colors[island_id]
    return EMPTY


def fill_island(island_id, color):
    filled = 0

    for index in range(CELL_COUNT):
        if marks[index] == island_id:
            if board[index] == color:
                return 0
            board[index] = color
            filled += 1

    return filled


def gravity_down():
    moved = False

    y = HEIGHT - 1
    while y >= 1:
        for x in range(WIDTH):
            if get_cell(x, y) == EMPTY and get_cell(x, y - 1) != EMPTY:
                lower = xy_index(x, y)
                upper = xy_index(x, y - 1)
                board[lower] = board[upper]
                board[upper] = EMPTY
                moved = True
        y -= 1

    return moved


def gravity_left():
    moved = False

    for x in range(WIDTH - 1):
        if column_has_cells(x):
            continue
        if not column_has_cells(x + 1):
            continue

        for y in range(HEIGHT):
            board[xy_index(x, y)] = board[xy_index(x + 1, y)]
            board[xy_index(x + 1, y)] = EMPTY
        moved = True

    return moved


def column_has_cells(x):
    for y in range(HEIGHT):
        if get_cell(x, y) != EMPTY:
            return True
    return False


def fill_top_row():
    for x in range(WIDTH):
        set_cell(x, 0, random_color())


def largest_non_empty_island():
    build_islands()
    biggest = 0

    for index in range(CELL_COUNT):
        if island_colors[index] != EMPTY and island_sizes[index] > biggest:
            biggest = island_sizes[index]

    return biggest


def place(x, y):
    global score
    global last_color
    global game_state
    global last_event_ms

    if game_state != WAITING:
        return
    if get_cell(x, y) == EMPTY:
        return

    selected_island = island_at(x, y)
    size = island_size(selected_island)
    if size <= 1:
        return

    last_color = island_color(selected_island)
    cleared = fill_island(selected_island, EMPTY)
    if cleared > 0:
        score += cleared * (cleared - 2)
        game_state = MOVING
        last_event_ms = MatrixOS.SYS.Millis()


def update_game():
    global score
    global game_state
    global last_event_ms

    update_function_key()

    if game_state == SETTINGS:
        return

    now = MatrixOS.SYS.Millis()

    if game_state == SETUP_BOARD:
        if now - last_event_ms >= STEP_MS:
            moved = gravity_down()
            if get_cell(0, 0) == EMPTY:
                fill_top_row()
                moved = True

            if moved:
                last_event_ms = now
            else:
                game_state = WAITING
                last_event_ms = now

    elif game_state == MOVING:
        if now - last_event_ms >= STEP_MS:
            moved = gravity_down()
            if moved:
                last_event_ms = now
            else:
                game_state = COMPACTING
                last_event_ms = now

    elif game_state == COMPACTING:
        if now - last_event_ms >= STEP_MS:
            moved = gravity_left()
            if moved:
                last_event_ms = now
            else:
                biggest = largest_non_empty_island()
                if biggest == 0:
                    score = score * 2
                    game_state = ENDED
                elif biggest < 2:
                    game_state = ENDED
                else:
                    game_state = WAITING
                last_event_ms = now


def enter_settings():
    global game_state
    game_state = SETTINGS


def exit_settings():
    global game_state
    game_state = WAITING


def set_two_colors():
    global num_colors
    num_colors = 2


def set_three_colors():
    global num_colors
    num_colors = 3


def set_four_colors():
    global num_colors
    num_colors = 4


def set_five_colors():
    global num_colors
    num_colors = 5


def reset_from_settings():
    reset_game()
    enter_settings()


def settings_enabled():
    return game_state == SETTINGS


def update_function_key():
    global fn_was_active

    snapshot = MatrixOS.Input.GetState(MatrixOS.Input.FunctionKey())
    active = False

    if snapshot is not None:
        info = snapshot.Keypad()
        if info is not None:
            active = info.Active()

    if active and not fn_was_active:
        if game_state == SETTINGS:
            exit_settings()
        else:
            enter_settings()

    fn_was_active = active


def board_enabled():
    return game_state == WAITING


def sync_board_buttons():
    for index in range(len(board_buttons)):
        board_buttons[index].SetColor(get_cell_color(board[index]))


def press_cell_0():
    place(0, 0)


def press_cell_1():
    place(1, 0)


def press_cell_2():
    place(2, 0)


def press_cell_3():
    place(3, 0)


def press_cell_4():
    place(4, 0)


def press_cell_5():
    place(5, 0)


def press_cell_6():
    place(6, 0)


def press_cell_7():
    place(7, 0)


def press_cell_8():
    place(0, 1)


def press_cell_9():
    place(1, 1)


def press_cell_10():
    place(2, 1)


def press_cell_11():
    place(3, 1)


def press_cell_12():
    place(4, 1)


def press_cell_13():
    place(5, 1)


def press_cell_14():
    place(6, 1)


def press_cell_15():
    place(7, 1)


def press_cell_16():
    place(0, 2)


def press_cell_17():
    place(1, 2)


def press_cell_18():
    place(2, 2)


def press_cell_19():
    place(3, 2)


def press_cell_20():
    place(4, 2)


def press_cell_21():
    place(5, 2)


def press_cell_22():
    place(6, 2)


def press_cell_23():
    place(7, 2)


def press_cell_24():
    place(0, 3)


def press_cell_25():
    place(1, 3)


def press_cell_26():
    place(2, 3)


def press_cell_27():
    place(3, 3)


def press_cell_28():
    place(4, 3)


def press_cell_29():
    place(5, 3)


def press_cell_30():
    place(6, 3)


def press_cell_31():
    place(7, 3)


def press_cell_32():
    place(0, 4)


def press_cell_33():
    place(1, 4)


def press_cell_34():
    place(2, 4)


def press_cell_35():
    place(3, 4)


def press_cell_36():
    place(4, 4)


def press_cell_37():
    place(5, 4)


def press_cell_38():
    place(6, 4)


def press_cell_39():
    place(7, 4)


def press_cell_40():
    place(0, 5)


def press_cell_41():
    place(1, 5)


def press_cell_42():
    place(2, 5)


def press_cell_43():
    place(3, 5)


def press_cell_44():
    place(4, 5)


def press_cell_45():
    place(5, 5)


def press_cell_46():
    place(6, 5)


def press_cell_47():
    place(7, 5)


def press_cell_48():
    place(0, 6)


def press_cell_49():
    place(1, 6)


def press_cell_50():
    place(2, 6)


def press_cell_51():
    place(3, 6)


def press_cell_52():
    place(4, 6)


def press_cell_53():
    place(5, 6)


def press_cell_54():
    place(6, 6)


def press_cell_55():
    place(7, 6)


def press_cell_56():
    place(0, 7)


def press_cell_57():
    place(1, 7)


def press_cell_58():
    place(2, 7)


def press_cell_59():
    place(3, 7)


def press_cell_60():
    place(4, 7)


def press_cell_61():
    place(5, 7)


def press_cell_62():
    place(6, 7)


def press_cell_63():
    place(7, 7)


BOARD_PRESS_FUNCS = [
    press_cell_0,
    press_cell_1,
    press_cell_2,
    press_cell_3,
    press_cell_4,
    press_cell_5,
    press_cell_6,
    press_cell_7,
    press_cell_8,
    press_cell_9,
    press_cell_10,
    press_cell_11,
    press_cell_12,
    press_cell_13,
    press_cell_14,
    press_cell_15,
    press_cell_16,
    press_cell_17,
    press_cell_18,
    press_cell_19,
    press_cell_20,
    press_cell_21,
    press_cell_22,
    press_cell_23,
    press_cell_24,
    press_cell_25,
    press_cell_26,
    press_cell_27,
    press_cell_28,
    press_cell_29,
    press_cell_30,
    press_cell_31,
    press_cell_32,
    press_cell_33,
    press_cell_34,
    press_cell_35,
    press_cell_36,
    press_cell_37,
    press_cell_38,
    press_cell_39,
    press_cell_40,
    press_cell_41,
    press_cell_42,
    press_cell_43,
    press_cell_44,
    press_cell_45,
    press_cell_46,
    press_cell_47,
    press_cell_48,
    press_cell_49,
    press_cell_50,
    press_cell_51,
    press_cell_52,
    press_cell_53,
    press_cell_54,
    press_cell_55,
    press_cell_56,
    press_cell_57,
    press_cell_58,
    press_cell_59,
    press_cell_60,
    press_cell_61,
    press_cell_62,
    press_cell_63
]


def add_button(ui, button, x, y, height, name, color_func, press_func):
    button.SetName(name)
    button.SetSize(Dimension(1, height))
    button.SetColorFunc(color_func)
    button.SetEnableFunc(settings_enabled)
    button.OnPress(press_func)
    ui.AddUIComponent(button, Point(x, y))


def add_board_button(ui, x, y):
    button = MatrixOS_UIButton.UIButton()
    button.SetName("Cell")
    button.SetSize(Dimension(1, 1))
    button.SetColor(get_cell_color(get_cell(x, y)))
    button.SetEnableFunc(board_enabled)
    button.OnPress(BOARD_PRESS_FUNCS[xy_index(x, y)])
    ui.AddUIComponent(button, Point(x, y))
    board_buttons.append(button)


reset_game()

main_ui = MatrixOS_UI.UI("SameGame", Color(0xFFFFFF))
main_ui.AllowExit(False)
main_ui.SetFPS(60)
main_ui.SetLoopFunc(update_game)
main_ui.SetPreRenderFunc(render_ui)

for board_y in range(HEIGHT):
    for board_x in range(WIDTH):
        add_board_button(main_ui, board_x, board_y)

color_2_button = MatrixOS_UIButton.UIButton()
color_3_button = MatrixOS_UIButton.UIButton()
color_4_button = MatrixOS_UIButton.UIButton()
color_5_button = MatrixOS_UIButton.UIButton()
reset_button = MatrixOS_UIButton.UIButton()

add_button(main_ui, color_2_button, 0, 0, 2, "2 Colors", color_button_0, set_two_colors)
add_button(main_ui, color_3_button, 1, 0, 3, "3 Colors", color_button_1, set_three_colors)
add_button(main_ui, color_4_button, 2, 0, 4, "4 Colors", color_button_2, set_four_colors)
add_button(main_ui, color_5_button, 3, 0, 5, "5 Colors", color_button_3, set_five_colors)

reset_button.SetName("Reset")
reset_button.SetSize(Dimension(1, 2))
reset_button.SetColor(Color(0xFF0000))
reset_button.SetEnableFunc(settings_enabled)
reset_button.OnPress(reset_from_settings)
main_ui.AddUIComponent(reset_button, Point(7, 3))

main_ui.Start()
main_ui.Close()

MatrixOS.LED.Fill(Color(0x000000))
MatrixOS.LED.Update()

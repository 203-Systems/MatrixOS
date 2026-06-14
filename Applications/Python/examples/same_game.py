import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
UI = MatrixOS.UI
Color = MatrixOS.Color
Point = MatrixOS.Point
Dimension = MatrixOS.Dimension
ColorEffects = MatrixOS.ColorEffects
Timer = MatrixOS.Timer


WIDTH = 8
HEIGHT = 8
CELL_COUNT = WIDTH * HEIGHT

EMPTY = 0
SETUP_BOARD = 0
WAITING = 1
MOVING = 2
COMPACTING = 3
ENDED = 4

STEP_MS = 100
FRAME_MS = 16

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
active_settings_ui = None
app_running = True
pending_cell = None
render_timer = Timer()
function_key = Input.function_key()


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
    random_seed = SYS.millis() & 0x7FFFFFFF
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
    global pending_cell

    seed_random()
    reset_board()
    score = 0
    last_color = num_colors
    game_state = SETUP_BOARD
    last_event_ms = SYS.millis()
    pending_cell = None
    render_timer.record_current()


def get_cell_color(cell):
    if cell >= 0 and cell < len(COLORS):
        return COLORS[cell]
    return Color(0xFFFFFF)


def fill_underglow(color):
    LED.fill_partition("Underglow", color)


def render_board_cells():
    for index in range(CELL_COUNT):
        LED.set_color_by_id(index, get_cell_color(board[index]))


def render_game():
    now = SYS.millis()
    render_board_cells()

    if game_state == ENDED:
        fill_underglow(ColorEffects.rainbow(1000, 0))
    elif game_state == MOVING:
        fill_underglow(ColorEffects.color_saw(get_cell_color(last_color), 250, now - last_event_ms))
    else:
        fill_underglow(ColorEffects.color_breath(get_cell_color(last_color), 2000, now - last_event_ms))


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
        LED.set_color_by_id(index, COLORS[EMPTY])
    fill_underglow(Color(0x00FFFF))


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
        last_event_ms = SYS.millis()


def update_game():
    global score
    global game_state
    global last_event_ms

    process_pending_input()

    now = SYS.millis()

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


def process_pending_input():
    global pending_cell

    event = Input.get_event()
    while event is not None:
        handle_key_input(event)
        event = Input.get_event()

    if pending_cell is not None:
        x = pending_cell[0]
        y = pending_cell[1]
        pending_cell = None
        place(x, y)


def handle_key_input(event):
    global pending_cell

    if event.id() == function_key:
        if event.is_hold():
            open_settings_ui()
        return True

    if not event.is_pressed():
        return True

    xy = Input.try_get_point(event.id())
    if xy is None:
        return True

    x = xy.x()
    y = xy.y()
    if x < 0 or x >= WIDTH or y < 0 or y >= HEIGHT:
        return True

    pending_cell = (x, y)

    return True


def add_button(ui, button, x, y, height, name, color_func, press_func):
    button.set_name(name)
    button.set_size(Dimension(1, height))
    button.set_color_func(color_func)
    button.on_press(press_func)
    ui.add(button, Point(x, y))


def process_settings_input():
    global active_settings_ui
    global app_running

    if active_settings_ui is None:
        return

    event = active_settings_ui.pull_input()
    while event is not None:
        if event.is_function_key():
            if event.is_hold():
                app_running = False
                active_settings_ui.exit()
            elif event.is_released():
                active_settings_ui.exit()
        event = active_settings_ui.pull_input()


def open_settings_ui():
    global active_settings_ui

    settings_ui = UI.UI("Settings", Color(0x00FFFF), True)
    active_settings_ui = settings_ui
    settings_ui.allow_exit(False)
    settings_ui.set_fps(60)
    settings_ui.set_loop_func(process_settings_input)
    settings_ui.set_pre_render_func(render_settings_background)

    color_2_button = UI.Button()
    color_3_button = UI.Button()
    color_4_button = UI.Button()
    color_5_button = UI.Button()
    reset_button = UI.Button()

    add_button(settings_ui, color_2_button, 0, 0, 2, "2 Colors", color_button_0, set_two_colors)
    add_button(settings_ui, color_3_button, 1, 0, 3, "3 Colors", color_button_1, set_three_colors)
    add_button(settings_ui, color_4_button, 2, 0, 4, "4 Colors", color_button_2, set_four_colors)
    add_button(settings_ui, color_5_button, 3, 0, 5, "5 Colors", color_button_3, set_five_colors)

    reset_button.set_name("Reset")
    reset_button.set_size(Dimension(1, 2))
    reset_button.set_color(Color(0xFF0000))
    reset_button.on_press(reset_from_settings)
    settings_ui.add(reset_button, Point(7, 3))

    settings_ui.start()
    active_settings_ui = None
    settings_ui.close()


def render_if_needed():
    if render_timer.tick(FRAME_MS, True):
        render_game()
        LED.update()


def main_loop():
    reset_game()
    Input.clear_input_buffer()

    while app_running:
        update_game()
        render_if_needed()
        SYS.task_yield()

    Input.clear_input_buffer()


main_loop()

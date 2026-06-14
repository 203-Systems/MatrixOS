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
FALLING = 2
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

SETTING_COLOR = Color(0x00FFFF)
WHITE = Color(0xFFFFFF)

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
settings_ui = None
app_running = True

render_timer = Timer()
function_key = Input.function_key()


def xy_index(x, y):
    return x + y * WIDTH


def in_bounds(x, y):
    return x >= 0 and x < WIDTH and y >= 0 and y < HEIGHT


def make_cells(value):
    cells = []
    for index in range(CELL_COUNT):
        cells.append(value)
    return cells


def get_cell(x, y):
    if not in_bounds(x, y):
        return EMPTY
    return board[xy_index(x, y)]


def set_cell(x, y, color):
    if in_bounds(x, y):
        board[xy_index(x, y)] = color


def cell_color(cell):
    if cell >= 0 and cell < len(COLORS):
        return COLORS[cell]
    return WHITE


def mark_event():
    global last_event_ms
    last_event_ms = SYS.millis()


def set_state(state):
    global game_state
    game_state = state
    mark_event()


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

    board = make_cells(EMPTY)
    marks = make_cells(255)
    island_sizes = make_cells(0)
    island_colors = make_cells(EMPTY)


def reset_game():
    global score
    global last_color

    seed_random()
    reset_board()
    score = 0
    last_color = num_colors
    set_state(SETUP_BOARD)
    render_timer.record_current()


def render_board():
    for index in range(CELL_COUNT):
        LED.set_color_by_id(index, cell_color(board[index]))


def render_game():
    now = SYS.millis()
    render_board()

    if game_state == ENDED:
        LED.fill_partition("Underglow", ColorEffects.rainbow(1000, 0))
    elif game_state == FALLING:
        LED.fill_partition("Underglow", ColorEffects.color_saw(cell_color(last_color), 250, now - last_event_ms))
    else:
        LED.fill_partition("Underglow", ColorEffects.color_breath(cell_color(last_color), 2000, now - last_event_ms))


def reset_island_marks():
    global marks
    global island_sizes
    global island_colors

    marks = make_cells(255)
    island_sizes = make_cells(0)
    island_colors = make_cells(EMPTY)


def add_same_color_neighbor(queue, island_id, color, x, y):
    if not in_bounds(x, y):
        return

    index = xy_index(x, y)
    if marks[index] == 255 and board[index] == color:
        marks[index] = island_id
        queue.append(index)


def scan_island(start, island_id):
    color = board[start]
    queue = [start]
    marks[start] = island_id
    island_colors[island_id] = color
    cursor = 0

    while cursor < len(queue):
        index = queue[cursor]
        cursor += 1
        island_sizes[island_id] += 1

        x = index % WIDTH
        y = index // WIDTH
        add_same_color_neighbor(queue, island_id, color, x - 1, y)
        add_same_color_neighbor(queue, island_id, color, x + 1, y)
        add_same_color_neighbor(queue, island_id, color, x, y - 1)
        add_same_color_neighbor(queue, island_id, color, x, y + 1)


def build_islands():
    reset_island_marks()
    island_count = 0

    for index in range(CELL_COUNT):
        if marks[index] == 255:
            scan_island(index, island_count)
            island_count += 1


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


def clear_island(island_id):
    cleared = 0

    for index in range(CELL_COUNT):
        if marks[index] == island_id:
            board[index] = EMPTY
            cleared += 1

    return cleared


def column_has_cells(x):
    for y in range(HEIGHT):
        if get_cell(x, y) != EMPTY:
            return True
    return False


def gravity_down():
    moved = False

    y = HEIGHT - 1
    while y >= 1:
        for x in range(WIDTH):
            if get_cell(x, y) == EMPTY and get_cell(x, y - 1) != EMPTY:
                board[xy_index(x, y)] = board[xy_index(x, y - 1)]
                board[xy_index(x, y - 1)] = EMPTY
                moved = True
        y -= 1

    return moved


def gravity_left():
    moved = False

    for x in range(WIDTH - 1):
        if column_has_cells(x) or not column_has_cells(x + 1):
            continue

        for y in range(HEIGHT):
            board[xy_index(x, y)] = board[xy_index(x + 1, y)]
            board[xy_index(x + 1, y)] = EMPTY
        moved = True

    return moved


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

    if game_state != WAITING or get_cell(x, y) == EMPTY:
        return

    island_id = island_at(x, y)
    size = island_size(island_id)
    if size <= 1:
        return

    last_color = island_color(island_id)
    cleared = clear_island(island_id)
    score += cleared * (cleared - 2)
    set_state(FALLING)


def step_setup_board():
    moved = gravity_down()

    if get_cell(0, 0) == EMPTY:
        fill_top_row()
        moved = True

    if moved:
        mark_event()
    else:
        set_state(WAITING)


def step_falling():
    if gravity_down():
        mark_event()
    else:
        set_state(COMPACTING)


def step_compacting():
    global score

    if gravity_left():
        mark_event()
        return

    biggest = largest_non_empty_island()
    if biggest == 0:
        score = score * 2
        set_state(ENDED)
    elif biggest < 2:
        set_state(ENDED)
    else:
        set_state(WAITING)


def update_game():
    process_input()

    if game_state == WAITING or game_state == ENDED:
        return
    if SYS.millis() - last_event_ms < STEP_MS:
        return

    if game_state == SETUP_BOARD:
        step_setup_board()
    elif game_state == FALLING:
        step_falling()
    elif game_state == COMPACTING:
        step_compacting()


def handle_grid_press(input_id):
    xy = Input.try_get_point(input_id)
    if xy is None:
        return

    x = xy.x()
    y = xy.y()
    if in_bounds(x, y):
        place(x, y)


def handle_input(event):
    if event.id() == function_key:
        if event.is_pressed():
            open_settings_ui()
        return

    if event.is_pressed():
        handle_grid_press(event.id())


def process_input():
    event = Input.get_event()
    while event is not None:
        handle_input(event)
        event = Input.get_event()


def selected_color_button(index):
    if index + 2 == num_colors:
        return WHITE
    return SETTING_COLORS[index]


def color_button_2():
    return selected_color_button(0)


def color_button_3():
    return selected_color_button(1)


def color_button_4():
    return selected_color_button(2)


def color_button_5():
    return selected_color_button(3)


def set_color_count(count):
    global num_colors
    num_colors = count


def set_two_colors():
    set_color_count(2)


def set_three_colors():
    set_color_count(3)


def set_four_colors():
    set_color_count(4)


def set_five_colors():
    set_color_count(5)


def render_settings_background():
    LED.fill(COLORS[EMPTY])
    LED.fill_partition("Underglow", SETTING_COLOR)


def add_settings_button(ui, x, height, name, color_func, press_func):
    button = UI.Button()
    button.set_name(name)
    button.set_size(Dimension(1, height))
    button.set_color_func(color_func)
    button.on_press(press_func)
    ui.add(button, Point(x, 0))
    return button


def add_reset_button(ui):
    button = UI.Button()
    button.set_name("Reset")
    button.set_size(Dimension(1, 2))
    button.set_color(Color(0xFF0000))
    button.on_press(reset_game)
    ui.add(button, Point(7, 3))
    return button


def process_settings_input():
    global app_running

    if settings_ui is None:
        return

    event = settings_ui.pull_input()
    while event is not None:
        if event.is_function_key():
            if event.is_hold():
                app_running = False
                settings_ui.exit()
                SYS.exit_app()
            elif event.is_released():
                settings_ui.exit()
        event = settings_ui.pull_input()


def open_settings_ui():
    global settings_ui

    settings_ui = UI.UI("Settings", SETTING_COLOR, True)
    settings_ui.allow_exit(False)
    settings_ui.set_fps(60)
    settings_ui.set_loop_func(process_settings_input)
    settings_ui.set_pre_render_func(render_settings_background)

    color_2_button = add_settings_button(settings_ui, 0, 2, "2 Colors", color_button_2, set_two_colors)
    color_3_button = add_settings_button(settings_ui, 1, 3, "3 Colors", color_button_3, set_three_colors)
    color_4_button = add_settings_button(settings_ui, 2, 4, "4 Colors", color_button_4, set_four_colors)
    color_5_button = add_settings_button(settings_ui, 3, 5, "5 Colors", color_button_5, set_five_colors)
    reset_button = add_reset_button(settings_ui)

    settings_ui.start()
    settings_ui.close()
    settings_ui = None


def render_if_needed():
    if render_timer.tick(FRAME_MS, True):
        render_game()
        LED.update()


def loop():
    if not app_running:
        return

    update_game()
    render_if_needed()


reset_game()
Input.clear_input_buffer()

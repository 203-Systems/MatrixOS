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
PLAYER_1 = 1
PLAYER_2 = 2
DRAW = 3

WAITING = 0
WIN_ANIMATION = 1
ENDED = 2
RESET_ANIMATION = 3

FRAME_MS = 16
WIN_STEP_MS = 30
RESET_STEP_MS = 30

BLACK = Color(0x000000)
WHITE = Color(0xFFFFFF)
DRAW_COLOR = Color(0x808080)
APP_COLOR = Color(0xF25E13)
RESET_COLOR = Color(0xFF0000)

COLOR_PALETTE = [
    Color(0x00FFAE),
    Color(0x6600FF),
    Color(0xFF4040),
    Color(0x40A0FF),
    Color(0xFFFF00),
    Color(0xFF00FF),
]

board = []
current_player = PLAYER_1
previous_player = EMPTY
first_player = PLAYER_1
winner = EMPTY
winning_length = 4
game_state = WAITING
started = False
placed_cell = None
last_event_ms = 0
last_place_ms = 0
reset_start_ms = 0
player_1_color_index = 0
player_2_color_index = 1
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


def mark_event():
    global last_event_ms
    last_event_ms = SYS.millis()


def set_state(state):
    global game_state
    game_state = state
    mark_event()


def player_color(player):
    if player == PLAYER_1:
        return COLOR_PALETTE[player_1_color_index]
    if player == PLAYER_2:
        return COLOR_PALETTE[player_2_color_index]
    return BLACK


def winner_color():
    if winner == DRAW:
        return DRAW_COLOR
    return player_color(winner)


def switch_player():
    global current_player
    global previous_player

    previous_player = current_player
    if current_player == PLAYER_1:
        current_player = PLAYER_2
    else:
        current_player = PLAYER_1
    mark_event()


def reset_board():
    global board
    board = make_cells(EMPTY)


def reset_game():
    global current_player
    global previous_player
    global winner
    global started
    global placed_cell
    global last_place_ms
    global reset_start_ms

    reset_board()
    current_player = first_player
    previous_player = EMPTY
    winner = EMPTY
    started = False
    placed_cell = None
    last_place_ms = 0
    reset_start_ms = 0
    set_state(WAITING)
    render_timer.record_current()


def board_full():
    for index in range(CELL_COUNT):
        if board[index] == EMPTY:
            return False
    return True


def count_stones(x, y, dx, dy, player):
    count = 0
    while in_bounds(x, y) and board[xy_index(x, y)] == player:
        count += 1
        x += dx
        y += dy
    return count


def check_win_direction(x, y, dx, dy, player):
    count = 1
    count += count_stones(x + dx, y + dy, dx, dy, player)
    count += count_stones(x - dx, y - dy, -dx, -dy, player)
    return count >= winning_length


def check_win(x, y):
    player = board[xy_index(x, y)]
    if player == EMPTY:
        return EMPTY

    if check_win_direction(x, y, 1, 0, player):
        return player
    if check_win_direction(x, y, 0, 1, player):
        return player
    if check_win_direction(x, y, 1, 1, player):
        return player
    if check_win_direction(x, y, 1, -1, player):
        return player
    return EMPTY


def place(x, y):
    global started
    global placed_cell
    global last_place_ms
    global winner
    global previous_player

    if game_state != WAITING or not in_bounds(x, y):
        return
    if board[xy_index(x, y)] != EMPTY:
        return

    started = True
    board[xy_index(x, y)] = current_player
    placed_cell = (x, y)
    last_place_ms = SYS.millis()

    won_by = check_win(x, y)
    if won_by != EMPTY:
        winner = won_by
        previous_player = current_player
        set_state(WIN_ANIMATION)
    elif board_full():
        winner = DRAW
        previous_player = current_player
        set_state(WIN_ANIMATION)
    else:
        switch_player()


def start_reset_sequence():
    global reset_start_ms

    if game_state != RESET_ANIMATION:
        reset_start_ms = SYS.millis()
        set_state(RESET_ANIMATION)


def blend_color(from_color, to_color, amount):
    if amount <= 0:
        return from_color
    if amount >= 255:
        return to_color

    r = from_color.r() + ((to_color.r() - from_color.r()) * amount // 255)
    g = from_color.g() + ((to_color.g() - from_color.g()) * amount // 255)
    b = from_color.b() + ((to_color.b() - from_color.b()) * amount // 255)
    return Color(r, g, b)


def breathe(color, offset):
    return ColorEffects.color_breath_low_bound(color, 64, 2000, offset)


def render_stones():
    for y in range(HEIGHT):
        for x in range(WIDTH):
            player = board[xy_index(x, y)]
            if player != EMPTY:
                color = player_color(player)
                if player == current_player and game_state == WAITING:
                    color = breathe(color, last_event_ms)
                LED.set_color_xy(x, y, color)


def render_place_flash(now):
    if placed_cell is None:
        return

    dt = now - last_place_ms
    if dt >= 200:
        return

    flash = blend_color(BLACK, WHITE, (200 - dt) * 255 // 200)
    x = placed_cell[0]
    y = placed_cell[1]

    if in_bounds(x, y + 1):
        LED.set_color_xy(x, y + 1, flash)
    if in_bounds(x, y - 1):
        LED.set_color_xy(x, y - 1, flash)
    if in_bounds(x + 1, y):
        LED.set_color_xy(x + 1, y, flash)
    if in_bounds(x - 1, y):
        LED.set_color_xy(x - 1, y, flash)


def render_waiting(now):
    LED.clear()
    render_stones()
    render_place_flash(now)

    dt = now - last_event_ms
    if previous_player != EMPTY and dt < 400:
        color = blend_color(player_color(previous_player), player_color(current_player), dt * 255 // 400)
    else:
        color = player_color(current_player)
    LED.fill_partition("Underglow", color)


def win_cell_color(x, y, dt, glow):
    index = (HEIGHT - 1 - y) * WIDTH + x
    start_ms = index * WIN_STEP_MS

    if dt >= start_ms + 450:
        return glow
    if dt >= start_ms + 200:
        return blend_color(WHITE, glow, (dt - start_ms - 200) * 255 // 250)
    if dt >= start_ms + 75:
        return WHITE
    if dt >= start_ms:
        base = player_color(board[xy_index(x, y)])
        return blend_color(base, WHITE, (dt - start_ms) * 255 // 75)
    return player_color(board[xy_index(x, y)])


def render_win_animation(now):
    global game_state

    all_done = True
    dt = now - last_event_ms
    glow = breathe(winner_color(), last_event_ms + 1000)

    LED.clear()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            index = (HEIGHT - 1 - y) * WIDTH + x
            if dt < index * WIN_STEP_MS + 450:
                all_done = False
            LED.set_color_xy(x, y, win_cell_color(x, y, dt, glow))

    LED.fill_partition("Underglow", glow)
    if all_done:
        game_state = ENDED


def render_ended():
    color = breathe(winner_color(), last_event_ms + 1000)
    LED.fill(color)
    LED.fill_partition("Underglow", color)


def reset_cell_color(x, y, dt, glow):
    index = y * WIDTH + x
    start_ms = index * RESET_STEP_MS

    if dt >= start_ms + 450:
        return None
    if dt >= start_ms + 200:
        return blend_color(WHITE, BLACK, (dt - start_ms - 200) * 255 // 250)
    if dt >= start_ms + 75:
        return WHITE
    if dt >= start_ms:
        return blend_color(glow, WHITE, (dt - start_ms) * 255 // 75)
    return glow


def render_reset_animation(now):
    dt = now - reset_start_ms
    glow = breathe(winner_color(), last_event_ms + 1000)
    all_done = True

    LED.clear()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            color = reset_cell_color(x, y, dt, glow)
            if color is not None:
                all_done = False
                LED.set_color_xy(x, y, color)

    if dt >= 500:
        LED.fill_partition("Underglow", BLACK)
    else:
        LED.fill_partition("Underglow", blend_color(glow, BLACK, dt * 255 // 500))

    if all_done:
        reset_game()


def render_game():
    now = SYS.millis()

    if game_state == WAITING:
        render_waiting(now)
    elif game_state == WIN_ANIMATION:
        render_win_animation(now)
    elif game_state == ENDED:
        render_ended()
    elif game_state == RESET_ANIMATION:
        render_reset_animation(now)

    LED.update()


def handle_grid_release(input_id):
    xy = Input.try_get_point(input_id)
    if xy is not None:
        place(xy.x(), xy.y())


def handle_game_input(event):
    if event.id() == function_key:
        if event.is_pressed():
            open_settings_ui()
        return

    if game_state == ENDED and event.is_released():
        start_reset_sequence()
    elif game_state == WAITING and event.is_released():
        handle_grid_release(event.id())


def process_game_input():
    event = Input.get_event()
    while event is not None:
        handle_game_input(event)
        event = Input.get_event()


def cycle_player_1_color():
    global player_1_color_index
    player_1_color_index = (player_1_color_index + 1) % len(COLOR_PALETTE)


def cycle_player_2_color():
    global player_2_color_index
    player_2_color_index = (player_2_color_index + 1) % len(COLOR_PALETTE)


def set_first_player(player):
    global first_player

    if first_player == player:
        return
    first_player = player
    if not started or game_state == ENDED:
        reset_game()


def set_first_player_1():
    set_first_player(PLAYER_1)


def set_first_player_2():
    set_first_player(PLAYER_2)


def get_winning_selector_value():
    return winning_length - 3


def set_winning_selector_value(value):
    global winning_length

    winning_length = value + 3
    if not started or game_state == ENDED:
        reset_game()


def get_winning_display_value():
    return winning_length


def player_1_button_color():
    return player_color(PLAYER_1)


def player_2_button_color():
    return player_color(PLAYER_2)


def player_1_first_color():
    return WHITE.dim_if_not(first_player == PLAYER_1)


def player_2_first_color():
    return WHITE.dim_if_not(first_player == PLAYER_2)


def render_settings_background():
    LED.clear()
    LED.fill_partition("Underglow", APP_COLOR)


def add_button(ui, x, y, width, height, name, color_func, press_func):
    button = UI.Button()
    button.set_name(name)
    button.set_size(Dimension(width, height))
    button.set_color_func(color_func)
    button.on_press(press_func)
    ui.add(button, Point(x, y))
    return button


def add_number(ui):
    number = UI.Number()
    number.set_name("Winning Length")
    number.set_color(APP_COLOR)
    number.set_digits(1)
    number.set_value_func(get_winning_display_value)
    ui.add(number, Point(0, 2))
    return number


def add_winning_selector(ui):
    selector = UI.Selector()
    selector.set_name("Winning Length")
    selector.set_color(WHITE)
    selector.set_dimension(Dimension(3, 1))
    selector.set_count(3)
    selector.set_value_func(get_winning_selector_value)
    selector.on_change(set_winning_selector_value)
    ui.add(selector, Point(0, 6))
    return selector


def add_reset_button(ui):
    button = UI.Button()
    button.set_name("Reset Game")
    button.set_size(Dimension(1, 2))
    button.set_color(RESET_COLOR)
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

    settings_ui = UI.UI("Gomoku Settings", APP_COLOR, True)
    settings_ui.allow_exit(False)
    settings_ui.set_fps(60)
    settings_ui.set_loop_func(process_settings_input)
    settings_ui.set_pre_render_func(render_settings_background)

    player_2_color_button = add_button(
        settings_ui, 0, 0, 8, 1, "Player 2 Color", player_2_button_color, cycle_player_2_color
    )
    player_1_color_button = add_button(
        settings_ui, 0, 7, 8, 1, "Player 1 Color", player_1_button_color, cycle_player_1_color
    )
    player_2_first_button = add_button(
        settings_ui, 0, 1, 1, 1, "Player 2 First", player_2_first_color, set_first_player_2
    )
    player_1_first_button = add_button(
        settings_ui, 7, 6, 1, 1, "Player 1 First", player_1_first_color, set_first_player_1
    )
    winning_number = add_number(settings_ui)
    winning_selector = add_winning_selector(settings_ui)
    reset_button = add_reset_button(settings_ui)

    settings_ui.start()
    settings_ui.close()
    settings_ui = None


def render_if_needed():
    if render_timer.tick(FRAME_MS, True):
        render_game()


def loop():
    if not app_running:
        return

    process_game_input()
    render_if_needed()


reset_game()
Input.clear_input_buffer()

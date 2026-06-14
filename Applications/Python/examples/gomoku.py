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

COLOR_PALETTE = [
    Color(0x00FFAE),
    Color(0x6600FF),
    Color(0xFF4040),
    Color(0x40A0FF),
    Color(0xFFFF00),
    Color(0xFF00FF),
]

board = []
current_player = 1
previous_player = 0
first_player = 1
winner = 0
winning_length = 4
game_state = WAITING
started = False
placed_cell = None
last_event_ms = 0
last_place_ms = 0
reset_start_ms = 0
player_1_color_index = 0
player_2_color_index = 1
active_settings_ui = None
app_running = True
render_timer = Timer()
function_key = Input.function_key()


def xy_index(x, y):
    return x + y * WIDTH


def in_bounds(x, y):
    return x >= 0 and x < WIDTH and y >= 0 and y < HEIGHT


def make_color(r, g, b):
    return Color(r, g, b)


def blend_color(from_color, to_color, amount):
    if amount <= 0:
        return from_color
    if amount >= 255:
        return to_color

    r = from_color.r() + ((to_color.r() - from_color.r()) * amount // 255)
    g = from_color.g() + ((to_color.g() - from_color.g()) * amount // 255)
    b = from_color.b() + ((to_color.b() - from_color.b()) * amount // 255)
    return make_color(r, g, b)


def player_color(player):
    if player == 1:
        return COLOR_PALETTE[player_1_color_index]
    if player == 2:
        return COLOR_PALETTE[player_2_color_index]
    return BLACK


def winner_color():
    if winner == 3:
        return DRAW_COLOR
    return player_color(winner)


def reset_board():
    global board

    board = []
    for index in range(CELL_COUNT):
        board.append(0)


def reset_game():
    global current_player
    global previous_player
    global winner
    global game_state
    global started
    global placed_cell
    global last_event_ms
    global last_place_ms
    global reset_start_ms

    reset_board()
    current_player = first_player
    previous_player = 0
    winner = 0
    game_state = WAITING
    started = False
    placed_cell = None
    last_event_ms = SYS.millis()
    last_place_ms = 0
    reset_start_ms = 0
    render_timer.record_current()


def board_full():
    for index in range(CELL_COUNT):
        if board[index] == 0:
            return False
    return True


def count_stones(x, y, dx, dy, player):
    count = 0
    while in_bounds(x, y) and board[xy_index(x, y)] == player:
        count += 1
        x += dx
        y += dy
    return count


def check_win(x, y):
    player = board[xy_index(x, y)]
    if player == 0:
        return 0

    directions = [
        (1, 0),
        (0, 1),
        (1, 1),
        (1, -1),
    ]

    for direction in directions:
        dx = direction[0]
        dy = direction[1]
        count = 1
        count += count_stones(x + dx, y + dy, dx, dy, player)
        count += count_stones(x - dx, y - dy, -dx, -dy, player)
        if count >= winning_length:
            return player

    return 0


def place(x, y):
    global current_player
    global previous_player
    global winner
    global game_state
    global started
    global placed_cell
    global last_event_ms
    global last_place_ms

    if game_state != WAITING:
        return
    if not in_bounds(x, y):
        return
    if board[xy_index(x, y)] != 0:
        return

    started = True
    board[xy_index(x, y)] = current_player
    placed_cell = (x, y)
    last_place_ms = SYS.millis()

    won_by = check_win(x, y)
    if won_by != 0:
        winner = won_by
        previous_player = current_player
        game_state = WIN_ANIMATION
        last_event_ms = SYS.millis()
        return

    if board_full():
        winner = 3
        previous_player = current_player
        game_state = WIN_ANIMATION
        last_event_ms = SYS.millis()
        return

    previous_player = current_player
    if current_player == 1:
        current_player = 2
    else:
        current_player = 1
    last_event_ms = SYS.millis()


def start_reset_sequence():
    global game_state
    global reset_start_ms

    if game_state == RESET_ANIMATION:
        return
    game_state = RESET_ANIMATION
    reset_start_ms = SYS.millis()


def cycle_player_1_color():
    global player_1_color_index

    player_1_color_index = (player_1_color_index + 1) % len(COLOR_PALETTE)


def cycle_player_2_color():
    global player_2_color_index

    player_2_color_index = (player_2_color_index + 1) % len(COLOR_PALETTE)


def set_first_player_1():
    global first_player

    if first_player == 1:
        return
    first_player = 1
    if not started or game_state == ENDED:
        reset_game()


def set_first_player_2():
    global first_player

    if first_player == 2:
        return
    first_player = 2
    if not started or game_state == ENDED:
        reset_game()


def get_winning_selector_value():
    return winning_length - 3


def set_winning_selector_value(value):
    global winning_length

    winning_length = value + 3
    if not started or game_state == ENDED:
        reset_game()


def get_winning_display_value():
    return winning_length


def render_stones():
    for y in range(HEIGHT):
        for x in range(WIDTH):
            player = board[xy_index(x, y)]
            if player != 0:
                color = player_color(player)
                if player == current_player and game_state == WAITING:
                    color = ColorEffects.color_breath_low_bound(color, 64, 2000, last_event_ms)
                LED.set_color_xy(x, y, color)


def render_place_flash(now):
    if placed_cell is None:
        return

    dt = now - last_place_ms
    if dt >= 200:
        return

    brightness = (200 - dt) * 255 // 200
    flash_color = blend_color(BLACK, WHITE, brightness)
    neighbors = [
        (0, 1),
        (0, -1),
        (1, 0),
        (-1, 0),
    ]

    for neighbor in neighbors:
        x = placed_cell[0] + neighbor[0]
        y = placed_cell[1] + neighbor[1]
        if in_bounds(x, y):
            LED.set_color_xy(x, y, flash_color)


def render_waiting(now):
    LED.clear()
    render_stones()
    render_place_flash(now)

    transition_dt = now - last_event_ms
    if previous_player != 0 and transition_dt < 400:
        amount = transition_dt * 255 // 400
        LED.fill_partition("Underglow", blend_color(player_color(previous_player), player_color(current_player), amount))
    else:
        LED.fill_partition("Underglow", player_color(current_player))


def render_win_animation(now):
    global game_state

    all_done = True
    dt = now - last_event_ms
    glow = ColorEffects.color_breath_low_bound(winner_color(), 64, 2000, last_event_ms + 1000)

    LED.clear()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            index = (HEIGHT - 1 - y) * WIDTH + x
            start_ms = index * WIN_STEP_MS
            color = BLACK

            if dt >= start_ms + 450:
                color = glow
            elif dt >= start_ms + 200:
                all_done = False
                amount = (dt - start_ms - 200) * 255 // 250
                color = blend_color(WHITE, glow, amount)
            elif dt >= start_ms + 75:
                all_done = False
                color = WHITE
            elif dt >= start_ms:
                all_done = False
                amount = (dt - start_ms) * 255 // 75
                base = player_color(board[xy_index(x, y)])
                color = blend_color(base, WHITE, amount)
            else:
                all_done = False
                color = player_color(board[xy_index(x, y)])

            LED.set_color_xy(x, y, color)

    LED.fill_partition("Underglow", glow)

    if all_done:
        game_state = ENDED


def render_ended():
    color = ColorEffects.color_breath_low_bound(winner_color(), 64, 2000, last_event_ms + 1000)
    LED.fill(color)
    LED.fill_partition("Underglow", color)


def render_reset_animation(now):
    all_done = True
    dt = now - reset_start_ms
    glow = ColorEffects.color_breath_low_bound(winner_color(), 64, 2000, last_event_ms + 1000)

    LED.clear()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            index = y * WIDTH + x
            start_ms = index * RESET_STEP_MS

            if dt >= start_ms + 450:
                continue
            if dt >= start_ms + 200:
                all_done = False
                amount = (dt - start_ms - 200) * 255 // 250
                LED.set_color_xy(x, y, blend_color(WHITE, BLACK, amount))
            elif dt >= start_ms + 75:
                all_done = False
                LED.set_color_xy(x, y, WHITE)
            elif dt >= start_ms:
                all_done = False
                amount = (dt - start_ms) * 255 // 75
                LED.set_color_xy(x, y, blend_color(glow, WHITE, amount))
            else:
                all_done = False
                LED.set_color_xy(x, y, glow)

    if dt >= 500:
        LED.fill_partition("Underglow", BLACK)
    else:
        amount = dt * 255 // 500
        LED.fill_partition("Underglow", blend_color(glow, BLACK, amount))

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


def handle_game_input(event):
    if event.id() == function_key:
        if event.is_pressed():
            open_settings_ui()
        return

    if game_state == ENDED and event.is_released():
        start_reset_sequence()
        return

    if game_state != WAITING or not event.is_released():
        return

    xy = Input.try_get_point(event.id())
    if xy is None:
        return

    x = xy.x()
    y = xy.y()
    if in_bounds(x, y):
        place(x, y)


def process_game_input():
    event = Input.get_event()
    while event is not None:
        handle_game_input(event)
        event = Input.get_event()


def add_button(ui, button, x, y, width, height, name, color_func, press_func):
    button.set_name(name)
    button.set_size(Dimension(width, height))
    button.set_color_func(color_func)
    button.on_press(press_func)
    ui.add(button, Point(x, y))


def render_settings_background():
    LED.clear()
    LED.fill_partition("Underglow", APP_COLOR)


def player_1_button_color():
    return player_color(1)


def player_2_button_color():
    return player_color(2)


def player_1_first_color():
    return WHITE.dim_if_not(first_player == 1)


def player_2_first_color():
    return WHITE.dim_if_not(first_player == 2)


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
                SYS.exit_app()
            elif event.is_released():
                active_settings_ui.exit()
        event = active_settings_ui.pull_input()


def open_settings_ui():
    global active_settings_ui

    settings_ui = UI.UI("Gomoku Settings", APP_COLOR, True)
    active_settings_ui = settings_ui
    settings_ui.allow_exit(False)
    settings_ui.set_fps(60)
    settings_ui.set_loop_func(process_settings_input)
    settings_ui.set_pre_render_func(render_settings_background)

    player_1_color_button = UI.Button()
    player_2_color_button = UI.Button()
    player_1_first_button = UI.Button()
    player_2_first_button = UI.Button()
    reset_button = UI.Button()
    winning_number = UI.Number()
    winning_selector = UI.Selector()

    add_button(settings_ui, player_2_color_button, 0, 0, 8, 1, "Player 2 Color", player_2_button_color, cycle_player_2_color)
    add_button(settings_ui, player_1_color_button, 0, 7, 8, 1, "Player 1 Color", player_1_button_color, cycle_player_1_color)
    add_button(settings_ui, player_2_first_button, 0, 1, 1, 1, "Player 2 First", player_2_first_color, set_first_player_2)
    add_button(settings_ui, player_1_first_button, 7, 6, 1, 1, "Player 1 First", player_1_first_color, set_first_player_1)

    winning_number.set_name("Winning Length")
    winning_number.set_color(APP_COLOR)
    winning_number.set_digits(1)
    winning_number.set_value_func(get_winning_display_value)
    settings_ui.add(winning_number, Point(0, 2))

    winning_selector.set_name("Winning Length")
    winning_selector.set_color(WHITE)
    winning_selector.set_dimension(Dimension(3, 1))
    winning_selector.set_count(3)
    winning_selector.set_value_func(get_winning_selector_value)
    winning_selector.on_change(set_winning_selector_value)
    settings_ui.add(winning_selector, Point(0, 6))

    reset_button.set_name("Reset Game")
    reset_button.set_size(Dimension(1, 2))
    reset_button.set_color(Color(0xFF0000))
    reset_button.on_press(reset_game)
    settings_ui.add(reset_button, Point(7, 3))

    settings_ui.start()
    active_settings_ui = None
    settings_ui.close()
    Input.clear_input_buffer()


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

# Python port based on triplefox/Matrix-OS-SameGame:
# https://github.com/triplefox/Matrix-OS-SameGame
import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS
ColorEffects = MatrixOS.ColorEffects
UI = MatrixOS.UI

WIDTH = 8
HEIGHT = 8
CELL_COUNT = WIDTH * HEIGHT
FUNCTION_KEY = Input.function_key()

STATE_PRESSED = MatrixOS.Input.STATE_PRESSED
STATE_HOLD = MatrixOS.Input.STATE_HOLD
STATE_RELEASED = MatrixOS.Input.STATE_RELEASED

SETUP_BOARD = 0
WAITING = 1
MOVING = 2
COMPACTING = 3
ENDED = 4
EMPTY = 0
COLORS = [0x000000, 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF]
NUM_COLORS_KEY = "Python SameGame num_colors"

board = [EMPTY for _ in range(CELL_COUNT)]
score = 0
cleared_score = 0
last_color = 4
num_colors = NVS.get(NUM_COLORS_KEY, 4)
if num_colors < 2 or num_colors > 5:
    num_colors = 4

game_state = SETUP_BOARD
last_event_ms = SYS.millis()
seed = 1
running = True


def index(x, y):
    return x + y * WIDTH


def in_bounds(x, y):
    return 0 <= x < WIDTH and 0 <= y < HEIGHT


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


def next_random():
    global seed
    seed ^= (seed << 13) & 0xFFFFFFFF
    seed ^= seed >> 17
    seed ^= (seed << 5) & 0xFFFFFFFF
    seed &= 0xFFFFFFFF
    if seed == 0:
        seed = 1
    return seed


def random_color():
    return ((next_random() >> 8) % num_colors) + 1


def cell_color(cell):
    if cell < 0 or cell >= len(COLORS):
        return 0xFFFFFF
    return COLORS[cell]


def render_board():
    for i in range(CELL_COUNT):
        LED.set_index(i, cell_color(board[i]))


def render():
    now = SYS.millis()
    LED.clear()
    render_board()

    if game_state == WAITING:
        LED.fill_partition("Underglow", ColorEffects.color_breath(cell_color(last_color), 2000, last_event_ms - 500))
    elif game_state == MOVING:
        LED.fill_partition("Underglow", ColorEffects.color_saw(cell_color(last_color), 250, last_event_ms - 500))
    elif game_state == ENDED:
        LED.fill_partition("Underglow", ColorEffects.rainbow(1000, 0))

    LED.update()


def gravity_down():
    moved = False
    for y in range(HEIGHT - 1, 0, -1):
        for x in range(WIDTH):
            below = index(x, y)
            above = index(x, y - 1)
            if board[below] == EMPTY and board[above] != EMPTY:
                board[below] = board[above]
                board[above] = EMPTY
                moved = True
    return moved


def gravity_left():
    moved = False
    has_cells = []
    for x in range(WIDTH):
        filled = False
        for y in range(HEIGHT):
            if board[index(x, y)] != EMPTY:
                filled = True
        has_cells.append(filled)

    for x in range(WIDTH - 1):
        if has_cells[x + 1] and not has_cells[x]:
            for y in range(HEIGHT):
                board[index(x, y)] = board[index(x + 1, y)]
                board[index(x + 1, y)] = EMPTY
            has_cells[x] = True
            has_cells[x + 1] = False
            moved = True
    return moved


def collect_group(x, y):
    color = board[index(x, y)]
    if color == EMPTY:
        return []

    group = []
    seen = [False for _ in range(CELL_COUNT)]
    queue = [(x, y)]
    seen[index(x, y)] = True

    while queue:
        cx, cy = queue.pop()
        group.append((cx, cy))
        for nx, ny in ((cx - 1, cy), (cx + 1, cy), (cx, cy - 1), (cx, cy + 1)):
            if in_bounds(nx, ny):
                ni = index(nx, ny)
                if not seen[ni] and board[ni] == color:
                    seen[ni] = True
                    queue.append((nx, ny))
    return group


def biggest_non_empty_group():
    seen = [False for _ in range(CELL_COUNT)]
    biggest = 0
    for y in range(HEIGHT):
        for x in range(WIDTH):
            i = index(x, y)
            if seen[i] or board[i] == EMPTY:
                continue
            group = collect_group(x, y)
            for gx, gy in group:
                seen[index(gx, gy)] = True
            if len(group) > biggest:
                biggest = len(group)
    return biggest


def reset_game(confirmed=True):
    global board, score, cleared_score, seed, last_color, game_state, last_event_ms
    seed = ((SYS.millis() << 16) ^ SYS.micros() ^ 0xA5A5A5A5) & 0xFFFFFFFF
    if seed == 0:
        seed = 1
    board = [EMPTY for _ in range(CELL_COUNT)]
    score = 0
    cleared_score = 0
    last_color = num_colors
    game_state = SETUP_BOARD
    last_event_ms = SYS.millis()
    render()


def place(x, y):
    global score, cleared_score, game_state, last_event_ms
    if game_state != WAITING or board[index(x, y)] == EMPTY:
        return

    group = collect_group(x, y)
    if len(group) <= 1:
        return

    for gx, gy in group:
        board[index(gx, gy)] = EMPTY
    cleared_score = len(group) * (len(group) - 2)
    score += cleared_score
    game_state = MOVING
    last_event_ms = SYS.millis()
    render()


def tick_game_state():
    global score, game_state, last_event_ms
    now = SYS.millis()

    if game_state == SETUP_BOARD and now - last_event_ms >= 100:
        moved = gravity_down()
        if board[0] == EMPTY:
            for x in range(WIDTH):
                board[index(x, 0)] = random_color()
            moved = True
        last_event_ms = now
        if not moved:
            game_state = WAITING
        render()

    elif game_state == MOVING and now - last_event_ms >= 100:
        moved = gravity_down()
        last_event_ms = now
        if not moved:
            game_state = COMPACTING
        render()

    elif game_state == COMPACTING and now - last_event_ms >= 100:
        moved = gravity_left()
        last_event_ms = now
        if not moved:
            biggest = biggest_non_empty_group()
            if biggest == 0:
                score *= 2
                game_state = ENDED
            elif biggest < 2:
                game_state = ENDED
            else:
                game_state = WAITING
        render()

    elif game_state == ENDED:
        render()


def color_count_button_color(option):
    if num_colors == option:
        return 0xFFFFFF
    return (0x222222, 0x662222, 0xAA2222, 0xEE2222)[option - 2]


def open_reset_confirm(settings_ui):
    confirm_ui = UI.UI("Confirm Reset", 0xFF0000, True)
    confirm_button = UI.Button("Confirm", 0xFF0000)
    confirm_button.set_size((1, 2))

    def confirm_reset():
        reset_game(True)
        confirm_ui.exit()
        settings_ui.exit()

    confirm_button.on_press(confirm_reset)
    confirm_ui.add(confirm_button, (7, 3))

    def confirm_input_handler(event):
        if event.get("id") != FUNCTION_KEY:
            return False
        if keypad_state(event) == STATE_RELEASED:
            confirm_ui.exit()
        return True

    confirm_ui.set_input_handler(confirm_input_handler)
    confirm_ui.start()


def open_settings():
    global num_colors, running

    settings_ui = UI.UI("Settings", 0xFF0000, True)

    for option in range(2, 6):
        button = UI.Button(str(option))
        button.set_size((1, option))
        button.set_color_func(lambda selected=option: color_count_button_color(selected))

        def set_color_count(selected=option):
            global num_colors
            num_colors = selected
            NVS.set(NUM_COLORS_KEY, num_colors)

        button.on_press(set_color_count)
        settings_ui.add(button, (option - 2, 0))

    reset_button = UI.Button("Reset Game", 0xFF0000)
    reset_button.set_size((1, 2))
    reset_button.on_press(lambda: open_reset_confirm(settings_ui))
    settings_ui.add(reset_button, (7, 3))

    def settings_input_handler(event):
        global running
        if event.get("id") != FUNCTION_KEY:
            return False
        state = keypad_state(event)
        if state == STATE_HOLD:
            running = False
            SYS.exit_app()
        elif state == STATE_RELEASED:
            settings_ui.exit()
        return True

    settings_ui.set_input_handler(settings_input_handler)
    settings_ui.start()
    render()


def handle_event(event):
    global running
    state = keypad_state(event)

    if event.get("id") == FUNCTION_KEY:
        if state == STATE_HOLD:
            running = False
            SYS.exit_app()
        elif state == STATE_RELEASED:
            open_settings()
        return

    if state != STATE_PRESSED or game_state != WAITING:
        return

    point = event.get("point")
    if point is None:
        return

    x, y = point
    if in_bounds(x, y):
        place(x, y)


def loop():
    if not running:
        return

    tick_game_state()

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()


reset_game(True)
Input.clear()

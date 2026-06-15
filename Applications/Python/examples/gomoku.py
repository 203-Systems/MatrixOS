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

WAITING = 0
WIN_ANIMATION = 1
ENDED = 2
RESET_ANIMATION = 3

EMPTY = 0
PLAYER_1 = 1
PLAYER_2 = 2
DRAW = 3

KEY_PLAYER1_COLOR = "Python Gomoku player1_color"
KEY_PLAYER2_COLOR = "Python Gomoku player2_color"
KEY_FIRST_PLAYER = "Python Gomoku first_player"
KEY_WINNING_LENGTH = "Python Gomoku winning_length"

PALETTE = [0x00FFAE, 0x6600FF, 0xFF4040, 0xFFFF00, 0xFFFFFF, 0x00A0FF]

player1_color = NVS.get(KEY_PLAYER1_COLOR, 0x00FFAE)
player2_color = NVS.get(KEY_PLAYER2_COLOR, 0x6600FF)
first_player = NVS.get(KEY_FIRST_PLAYER, PLAYER_1)
winning_length = NVS.get(KEY_WINNING_LENGTH, 5)
if first_player not in (PLAYER_1, PLAYER_2):
    first_player = PLAYER_1
if winning_length < 3 or winning_length > 5:
    winning_length = 5

board = [EMPTY for _ in range(CELL_COUNT)]
current_player = first_player
previous_player = EMPTY
winner = EMPTY
started = False
game_state = WAITING
last_event_ms = SYS.millis()
last_place_ms = 0
reset_start_ms = 0
placed_pos = None
running = True


def index(x, y):
    return x + y * WIDTH


def in_bounds(x, y):
    return 0 <= x < WIDTH and 0 <= y < HEIGHT


def rgb_scale(color, amount):
    r = ((color >> 16) & 0xFF) * amount // 255
    g = ((color >> 8) & 0xFF) * amount // 255
    b = (color & 0xFF) * amount // 255
    return (r << 16) | (g << 8) | b


def crossfade(a, b, ratio):
    if ratio < 0:
        ratio = 0
    if ratio > 255:
        ratio = 255
    ar, ag, ab = (a >> 16) & 0xFF, (a >> 8) & 0xFF, a & 0xFF
    br, bg, bb = (b >> 16) & 0xFF, (b >> 8) & 0xFF, b & 0xFF
    r = ar + (br - ar) * ratio // 255
    g = ag + (bg - ag) * ratio // 255
    b2 = ab + (bb - ab) * ratio // 255
    return (r << 16) | (g << 8) | b2


def low_breath(color, period=2000, offset=0):
    breathed = ColorEffects.color_breath(color, period, offset)
    return crossfade(rgb_scale(color, 64), breathed, 180)


def player_color(player):
    if player == PLAYER_1:
        return player1_color
    if player == PLAYER_2:
        return player2_color
    return 0


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


def reset_game():
    global board, current_player, previous_player, winner, started, game_state, last_event_ms, placed_pos
    board = [EMPTY for _ in range(CELL_COUNT)]
    current_player = first_player
    previous_player = EMPTY
    winner = EMPTY
    started = False
    game_state = WAITING
    placed_pos = None
    last_event_ms = SYS.millis()
    render()


def count_stones(x, y, dx, dy, player):
    count = 0
    while in_bounds(x, y) and board[index(x, y)] == player:
        count += 1
        x += dx
        y += dy
    return count


def check_win(x, y):
    target = board[index(x, y)]
    if target == EMPTY:
        return EMPTY
    for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)):
        count = 1
        count += count_stones(x + dx, y + dy, dx, dy, target)
        count += count_stones(x - dx, y - dy, -dx, -dy, target)
        if count >= winning_length:
            return target
    return EMPTY


def board_full():
    for cell in board:
        if cell == EMPTY:
            return False
    return True


def place(x, y):
    global started, current_player, previous_player, winner, game_state, last_event_ms, last_place_ms, placed_pos
    if game_state != WAITING or not in_bounds(x, y) or board[index(x, y)] != EMPTY:
        return

    started = True
    board[index(x, y)] = current_player
    placed_pos = (x, y)
    last_place_ms = SYS.millis()

    win = check_win(x, y)
    if win != EMPTY:
        winner = win
        game_state = WIN_ANIMATION
        previous_player = current_player
        last_event_ms = SYS.millis()
    elif board_full():
        winner = DRAW
        game_state = WIN_ANIMATION
        previous_player = current_player
        last_event_ms = SYS.millis()
    else:
        previous_player = current_player
        current_player = PLAYER_2 if current_player == PLAYER_1 else PLAYER_1
        last_event_ms = SYS.millis()
    render()


def render_waiting(now):
    for y in range(HEIGHT):
        for x in range(WIDTH):
            player = board[index(x, y)]
            if player != EMPTY:
                color = player_color(player)
                if player == current_player:
                    color = low_breath(color, 2000, last_event_ms)
                LED.set_xy(x, y, color)

    if placed_pos is not None:
        flash_dt = now - last_place_ms
        if flash_dt < 200:
            brightness = 255 - flash_dt * 255 // 200
            flash = rgb_scale(0xFFFFFF, brightness)
            px, py = placed_pos
            for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                nx, ny = px + dx, py + dy
                if in_bounds(nx, ny):
                    LED.set_xy(nx, ny, flash)

    transition_dt = now - last_event_ms
    if previous_player != EMPTY and transition_dt < 400:
        color = crossfade(player_color(previous_player), player_color(current_player), transition_dt * 255 // 400)
        LED.fill_partition("Underglow", color)
    else:
        LED.fill_partition("Underglow", player_color(current_player))


def render_win_animation(now):
    global game_state
    win_color = 0x808080 if winner == DRAW else player_color(winner)
    dt = now - last_event_ms
    all_done = True
    glow = low_breath(win_color, 2000, last_event_ms + 1000)

    for y in range(HEIGHT):
        for x in range(WIDTH):
            i = (7 - y) * 8 + x
            start = i * 30
            color = 0
            if dt >= start + 450:
                color = glow
            elif dt >= start + 200:
                all_done = False
                color = crossfade(0xFFFFFF, glow, (dt - start - 200) * 255 // 250)
            elif dt >= start + 75:
                all_done = False
                color = 0xFFFFFF
            elif dt >= start:
                all_done = False
                start_color = player_color(board[index(x, y)])
                color = crossfade(start_color, 0xFFFFFF, (dt - start) * 255 // 75)
            else:
                all_done = False
                color = player_color(board[index(x, y)])
            LED.set_xy(x, y, color)

    LED.fill_partition("Underglow", glow)
    if all_done:
        game_state = ENDED


def render_reset_animation(now):
    global game_state
    win_color = 0x808080 if winner == DRAW else player_color(winner)
    dt = now - reset_start_ms
    all_done = True
    glow = low_breath(win_color, 2000, last_event_ms + 1000)

    for y in range(HEIGHT):
        for x in range(WIDTH):
            i = y * 8 + x
            start = i * 30
            if dt >= start + 450:
                color = 0
            elif dt >= start + 200:
                all_done = False
                color = crossfade(0xFFFFFF, 0, (dt - start - 200) * 255 // 250)
            elif dt >= start + 75:
                all_done = False
                color = 0xFFFFFF
            elif dt >= start:
                all_done = False
                color = crossfade(glow, 0xFFFFFF, (dt - start) * 255 // 75)
            else:
                all_done = False
                color = glow
            LED.set_xy(x, y, color)

    LED.fill_partition("Underglow", crossfade(glow, 0, min(255, dt * 255 // 500)))
    if all_done:
        reset_game()


def render():
    now = SYS.millis()
    LED.clear()
    if game_state == WAITING:
        render_waiting(now)
    elif game_state == WIN_ANIMATION:
        render_win_animation(now)
    elif game_state == ENDED:
        color = 0x808080 if winner == DRAW else player_color(winner)
        glow = low_breath(color, 2000, last_event_ms + 1000)
        LED.fill(glow)
        LED.fill_partition("Underglow", glow)
    elif game_state == RESET_ANIMATION:
        render_reset_animation(now)
    LED.update()


def start_reset_sequence():
    global game_state, reset_start_ms
    if game_state == RESET_ANIMATION:
        return
    game_state = RESET_ANIMATION
    reset_start_ms = SYS.millis()


def cycle_player_color(player):
    global player1_color, player2_color
    current = player_color(player)
    idx = 0
    for i in range(len(PALETTE)):
        if PALETTE[i] == current:
            idx = i
    new_color = PALETTE[(idx + 1) % len(PALETTE)]
    if player == PLAYER_1:
        player1_color = new_color
        NVS.set(KEY_PLAYER1_COLOR, new_color)
    else:
        player2_color = new_color
        NVS.set(KEY_PLAYER2_COLOR, new_color)


def open_reset_confirm(settings_ui):
    confirm_ui = UI.UI("Confirm Reset", 0xFF0000, True)
    confirm_button = UI.Button("Confirm", 0xFF0000)
    confirm_button.set_size((1, 2))

    def confirm_reset():
        reset_game()
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
    global running, first_player, winning_length

    settings_ui = UI.UI("Settings", 0x00FFFF, True)
    function_key_rearmed = [False]

    player1_button = UI.Button("Player 1")
    player1_button.set_size((8, 1))
    player1_button.set_color_func(lambda: player1_color)
    player1_button.on_press(lambda: cycle_player_color(PLAYER_1))
    settings_ui.add(player1_button, (0, 7))

    player2_button = UI.Button("Player 2")
    player2_button.set_size((8, 1))
    player2_button.set_color_func(lambda: player2_color)
    player2_button.on_press(lambda: cycle_player_color(PLAYER_2))
    settings_ui.add(player2_button, (0, 0))

    def set_first_player_1():
        global first_player
        first_player = PLAYER_1
        NVS.set(KEY_FIRST_PLAYER, first_player)
        reset_game()

    first_player1_button = UI.Button("Player 1 First")
    first_player1_button.set_color_func(lambda: 0xFFFFFF if first_player == PLAYER_1 else 0x333333)
    first_player1_button.on_press(set_first_player_1)
    settings_ui.add(first_player1_button, (7, 6))

    def set_first_player_2():
        global first_player
        first_player = PLAYER_2
        NVS.set(KEY_FIRST_PLAYER, first_player)
        reset_game()

    first_player2_button = UI.Button("Player 2 First")
    first_player2_button.set_color_func(lambda: 0xFFFFFF if first_player == PLAYER_2 else 0x333333)
    first_player2_button.on_press(set_first_player_2)
    settings_ui.add(first_player2_button, (0, 1))

    for length in range(3, 6):
        button = UI.Button(str(length))
        button.set_color_func(lambda selected=length: 0xF25E13 if winning_length == selected else 0x333333)

        def set_length(selected=length):
            global winning_length
            winning_length = selected
            NVS.set(KEY_WINNING_LENGTH, winning_length)

        button.on_press(set_length)
        settings_ui.add(button, (length - 3, 6))

    reset_button = UI.Button("Reset", 0xFF0000)
    reset_button.set_size((1, 2))
    reset_button.on_press(lambda: open_reset_confirm(settings_ui))
    settings_ui.add(reset_button, (7, 3))

    def settings_input_handler(event):
        global running
        if event.get("id") != FUNCTION_KEY:
            return False
        state = keypad_state(event)
        if state == STATE_PRESSED:
            function_key_rearmed[0] = True
        elif state == STATE_HOLD:
            running = False
            SYS.exit_app()
        elif state == STATE_RELEASED and function_key_rearmed[0]:
            settings_ui.exit()
        return True

    settings_ui.set_input_handler(settings_input_handler)
    settings_ui.start()
    render()


def handle_event(event):
    global running
    state = keypad_state(event)

    if event.get("id") == FUNCTION_KEY:
        if state == STATE_PRESSED:
            open_settings()
        elif state == STATE_HOLD:
            running = False
            SYS.exit_app()
        return

    if state == STATE_RELEASED:
        if game_state == ENDED:
            start_reset_sequence()
            return
        if game_state == WAITING:
            point = event.get("point")
            if point is not None:
                x, y = point
                place(x, y)


def loop():
    if not running:
        return

    if game_state in (WAITING, WIN_ANIMATION, ENDED, RESET_ANIMATION):
        render()

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()

    SYS.sleep_ms(1)


reset_game()
Input.clear()

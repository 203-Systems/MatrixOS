import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS
ColorEffects = MatrixOS.ColorEffects
UI = MatrixOS.UI

WIDTH = 8
HEIGHT = 8
FUNCTION_KEY = Input.function_key()

STATE_PRESSED = Input.STATE_PRESSED
STATE_HOLD = Input.STATE_HOLD
STATE_RELEASED = Input.STATE_RELEASED

EMPTY = 0
PLAYER_1 = 1
PLAYER_2 = 2
DRAW = 3
NO_VALID_MOVES = 254
CHECK_PENDING = 255

WAITING = 0
NO_VALID = 1
MOVING = 2
INTERMISSION = 3
WINNER_UNVEIL = 4
ENDED = 5

KEY_PLAYER1_COLOR = "Python Reversi player1_color"
KEY_PLAYER2_COLOR = "Python Reversi player2_color"
KEY_FIRST_PLAYER = "Python Reversi first_player"
KEY_HINT = "Python Reversi hint"

PALETTE = [0xFF00FF, 0x00FFFF, 0x00FF00, 0xFF4040, 0xFFFF00, 0xFFFFFF]

player1_color = NVS.get(KEY_PLAYER1_COLOR, 0xFF00FF)
player2_color = NVS.get(KEY_PLAYER2_COLOR, 0x00FFFF)
first_player = NVS.get(KEY_FIRST_PLAYER, PLAYER_1)
hint = NVS.get(KEY_HINT, True)
if first_player not in (PLAYER_1, PLAYER_2):
    first_player = PLAYER_1

board = [[EMPTY for _ in range(WIDTH)] for _ in range(HEIGHT)]
was_board = [[EMPTY for _ in range(WIDTH)] for _ in range(HEIGHT)]
valid_moves = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]
newly_placed = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

current_player = first_player
game_state = WAITING
last_event_ms = SYS.millis()
invalid_place = None
invalid_place_ms = 0
last_hint_brightness = 0
started = False
placed_pos = (0, 0)
winner = EMPTY
running = True


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


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


def player_color(player):
    if player == PLAYER_1:
        return player1_color
    if player == PLAYER_2:
        return player2_color
    if player == DRAW:
        return 0xFFFFFF
    return 0


def other_player(player):
    return PLAYER_2 if player == PLAYER_1 else PLAYER_1


def flip(x, y, player, update):
    opponent = other_player(player)
    total = 0
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dx == 0 and dy == 0:
                continue
            cx = x + dx
            cy = y + dy
            if not in_bounds(cx, cy) or board[cy][cx] != opponent:
                continue

            distance = 2
            while True:
                cx = x + dx * distance
                cy = y + dy * distance
                if not in_bounds(cx, cy):
                    break
                found = board[cy][cx]
                if found == opponent:
                    distance += 1
                    continue
                if found == player:
                    fx = x + dx
                    fy = y + dy
                    while fx != cx or fy != cy:
                        if update:
                            board[fy][fx] = player
                        total += 1
                        fx += dx
                        fy += dy
                break
    return total


def compute_valid_moves(player):
    found = False
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if board[y][x] == EMPTY and flip(x, y, player, False):
                valid_moves[y][x] = 1
                found = True
            else:
                valid_moves[y][x] = 0
    return found


def check_game_over():
    global valid_moves
    next_player = other_player(current_player)
    if compute_valid_moves(next_player):
        return EMPTY

    has_current = False
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if board[y][x] == EMPTY and flip(x, y, current_player, False):
                has_current = True
    if has_current:
        return NO_VALID_MOVES

    player1_count = 0
    player2_count = 0
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if board[y][x] == PLAYER_1:
                player1_count += 1
            elif board[y][x] == PLAYER_2:
                player2_count += 1
    if player1_count > player2_count:
        return PLAYER_1
    if player2_count > player1_count:
        return PLAYER_2
    return DRAW


def reset_game(confirmed=True):
    global current_player, game_state, last_event_ms, invalid_place, started, winner, placed_pos

    if not confirmed and game_state != ENDED and started:
        if not confirm_menu():
            return False

    for y in range(HEIGHT):
        for x in range(WIDTH):
            board[y][x] = EMPTY
            was_board[y][x] = EMPTY
            valid_moves[y][x] = 0
            newly_placed[y][x] = 0

    second_player = other_player(first_player)
    board[3][3] = first_player
    board[4][4] = first_player
    board[3][4] = second_player
    board[4][3] = second_player
    for y in range(HEIGHT):
        for x in range(WIDTH):
            was_board[y][x] = board[y][x]

    current_player = first_player
    compute_valid_moves(current_player)
    game_state = WAITING
    winner = EMPTY
    started = False
    invalid_place = None
    placed_pos = (0, 0)
    last_event_ms = SYS.millis()
    render()
    return True


def place(x, y):
    global invalid_place, invalid_place_ms, started, placed_pos, game_state, last_event_ms
    if not in_bounds(x, y):
        return

    invalid_place = (x, y)
    invalid_place_ms = SYS.millis()
    if game_state != WAITING or not valid_moves[y][x]:
        return

    invalid_place = None
    started = True
    for yy in range(HEIGHT):
        for xx in range(WIDTH):
            was_board[yy][xx] = board[yy][xx]
            newly_placed[yy][xx] = 0

    board[y][x] = current_player
    newly_placed[y][x] = 1
    placed_pos = (x, y)
    flip(x, y, current_player, True)
    game_state = MOVING
    last_event_ms = SYS.millis()
    render()


def render_waiting(now, time_since):
    global last_hint_brightness
    if hint:
        if time_since <= 750:
            hint_color = ColorEffects.color_breath(player_color(current_player), 1500, last_event_ms)
        else:
            hint_color = ColorEffects.color_breath(player_color(current_player), 1500, last_event_ms)
            hint_color = crossfade(rgb_scale(player_color(current_player), 64), hint_color, 160)
        last_hint_brightness = hint_color & 0xFF
    else:
        hint_color = 0
        last_hint_brightness = 0

    for y in range(HEIGHT):
        for x in range(WIDTH):
            if hint and valid_moves[y][x]:
                LED.set_xy(x, y, rgb_scale(hint_color, 128))
            else:
                LED.set_xy(x, y, player_color(board[y][x]))

    if invalid_place is not None:
        dt = now - invalid_place_ms
        if dt < 800:
            x, y = invalid_place
            if dt <= 500:
                LED.set_xy(x, y, 0xFF0000)
            else:
                LED.set_xy(x, y, crossfade(player_color(board[y][x]), 0xFF0000, 255 - ((dt - 500) * 255 // 300)))

    LED.fill_partition("Underglow", ColorEffects.color_breath(player_color(current_player), 2000, last_event_ms - 500))


def render_moving(time_since):
    global game_state, winner, last_event_ms
    done = True
    px, py = placed_pos
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if board[y][x] != was_board[y][x]:
                if newly_placed[y][x]:
                    LED.set_xy(x, y, player_color(board[y][x]))
                    continue
                distance2 = (x - px) * (x - px) + (y - py) * (y - py)
                start = distance2 * 35 + 200
                if time_since <= start:
                    ratio = 0
                    done = False
                elif time_since - 300 > start:
                    ratio = 255
                else:
                    ratio = (time_since - start) * 255 // 300
                    done = False
                LED.set_xy(x, y, crossfade(player_color(was_board[y][x]), player_color(current_player), ratio))
            elif hint and valid_moves[y][x]:
                if time_since <= 200:
                    ratio = last_hint_brightness - time_since * last_hint_brightness // 200
                else:
                    ratio = 0
                LED.set_xy(x, y, rgb_scale(player_color(current_player), ratio))
            else:
                LED.set_xy(x, y, player_color(board[y][x]))

    LED.fill_partition("Underglow", player_color(current_player))
    if done and time_since >= 250:
        winner = CHECK_PENDING
        game_state = INTERMISSION
        last_event_ms = SYS.millis()


def render_no_valid(time_since):
    global game_state, winner, last_event_ms
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if board[y][x] == EMPTY:
                LED.set_xy(x, y, ColorEffects.color_strobe(rgb_scale(0xFF0000, 128), 500, last_event_ms))
            else:
                LED.set_xy(x, y, player_color(board[y][x]))
    LED.fill_partition("Underglow", ColorEffects.color_strobe(player_color(current_player), 500, last_event_ms))
    if time_since >= 2000:
        winner = CHECK_PENDING
        game_state = INTERMISSION
        last_event_ms = SYS.millis()


def render_intermission(time_since):
    global winner, game_state, current_player, last_event_ms
    if winner == CHECK_PENDING:
        winner = check_game_over()

    next_player = other_player(current_player)
    ratio = min(255, time_since * 255 // 500)
    target = winner if winner not in (EMPTY, NO_VALID_MOVES) else next_player
    LED.fill_partition("Underglow", crossfade(player_color(current_player), player_color(target), ratio))
    for y in range(HEIGHT):
        for x in range(WIDTH):
            LED.set_xy(x, y, player_color(board[y][x]))

    if time_since >= 500:
        current_player = next_player
        if winner == EMPTY:
            game_state = WAITING
        elif winner == NO_VALID_MOVES:
            game_state = NO_VALID
        else:
            game_state = WINNER_UNVEIL
        last_event_ms = SYS.millis()


def local_winner_color(x, y):
    local = winner
    if winner == DRAW:
        local = PLAYER_1 if (x + y * 7) % 2 == 0 else PLAYER_2
    return player_color(local)


def render_winner_unveil(time_since):
    global game_state
    done = True
    for y in range(HEIGHT):
        for x in range(WIDTH):
            start = (y * WIDTH + x) * 50
            if time_since >= start + 1000:
                LED.set_xy(x, y, local_winner_color(x, y))
            elif time_since >= start + 700:
                done = False
                LED.set_xy(x, y, crossfade(0xFFFFFF, local_winner_color(x, y), (time_since - start - 700) * 255 // 300))
            elif time_since >= start + 300:
                done = False
                LED.set_xy(x, y, 0xFFFFFF)
            elif time_since >= start:
                done = False
                LED.set_xy(x, y, crossfade(player_color(board[y][x]), 0xFFFFFF, (time_since - start) * 255 // 300))
            else:
                done = False
                LED.set_xy(x, y, player_color(board[y][x]))
    LED.fill_partition("Underglow", player_color(winner))
    if done:
        game_state = ENDED


def render_ended():
    for y in range(HEIGHT):
        for x in range(WIDTH):
            LED.set_xy(x, y, crossfade(rgb_scale(local_winner_color(x, y), 64), ColorEffects.color_breath(local_winner_color(x, y), 2000, last_event_ms + 1000), 180))
    LED.fill_partition("Underglow", ColorEffects.color_breath(player_color(winner), 2000, last_event_ms + 1000))


def render():
    now = SYS.millis()
    time_since = now - last_event_ms
    LED.clear()
    if game_state == WAITING:
        render_waiting(now, time_since)
    elif game_state == MOVING:
        render_moving(time_since)
    elif game_state == NO_VALID:
        render_no_valid(time_since)
    elif game_state == INTERMISSION:
        render_intermission(time_since)
    elif game_state == WINNER_UNVEIL:
        render_winner_unveil(time_since)
    else:
        render_ended()
    LED.update()


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


def confirm_menu():
    confirmed = [False]
    confirm_ui = UI.UI("Reset Game?", 0xFF0000, False)

    cancel_btn = UI.Button("Cancel", 0xFF0000)
    cancel_btn.set_size((2, 2))
    cancel_btn.on_press(lambda: confirm_ui.exit())
    confirm_ui.add(cancel_btn, (1, 3))

    confirm_btn = UI.Button("Confirm", 0x00FF00)
    confirm_btn.set_size((2, 2))

    def confirm():
        confirmed[0] = True
        confirm_ui.exit()

    confirm_btn.on_press(confirm)
    confirm_ui.add(confirm_btn, (5, 3))
    confirm_ui.start()
    return confirmed[0]


def open_settings():
    global running, first_player, hint
    settings_ui = UI.UI("Settings", 0x00FFFF, True)
    function_rearmed = [False]

    player1_btn = UI.Button("Player 1 Color")
    player1_btn.set_size((8, 1))
    player1_btn.set_color_func(lambda: player1_color)
    player1_btn.on_press(lambda: cycle_player_color(PLAYER_1))
    settings_ui.add(player1_btn, (0, 7))

    player2_btn = UI.Button("Player 2 Color")
    player2_btn.set_size((8, 1))
    player2_btn.set_color_func(lambda: player2_color)
    player2_btn.on_press(lambda: cycle_player_color(PLAYER_2))
    settings_ui.add(player2_btn, (0, 0))

    first1_btn = UI.Button("Player 1 First")
    first1_btn.set_color_func(lambda: 0xFFFFFF if first_player == PLAYER_1 else 0x333333)

    def set_first1():
        global first_player
        if first_player == PLAYER_1:
            return
        if game_state == ENDED or not started or confirm_menu():
            first_player = PLAYER_1
            NVS.set(KEY_FIRST_PLAYER, first_player)
            reset_game(True)

    first1_btn.on_press(set_first1)
    settings_ui.add(first1_btn, (7, 6))

    first2_btn = UI.Button("Player 2 First")
    first2_btn.set_color_func(lambda: 0xFFFFFF if first_player == PLAYER_2 else 0x333333)

    def set_first2():
        global first_player
        if first_player == PLAYER_2:
            return
        if game_state == ENDED or not started or confirm_menu():
            first_player = PLAYER_2
            NVS.set(KEY_FIRST_PLAYER, first_player)
            reset_game(True)

    first2_btn.on_press(set_first2)
    settings_ui.add(first2_btn, (0, 1))

    hint_toggle = UI.Toggle("Placement Hint", hint)
    hint_toggle.set_size((1, 2))
    hint_toggle.set_color(0x00FF00)

    def set_hint(value):
        global hint
        hint = value
        NVS.set(KEY_HINT, hint)

    hint_toggle.on_press(set_hint)
    settings_ui.add(hint_toggle, (0, 3))

    reset_btn = UI.Button("Reset Game", 0xFF0000)
    reset_btn.set_size((1, 2))

    def reset_from_settings():
        if reset_game(False):
            settings_ui.exit()

    reset_btn.on_press(reset_from_settings)
    settings_ui.add(reset_btn, (7, 3))

    def settings_input_handler(event):
        global running
        if event.get("id") != FUNCTION_KEY:
            return False
        state = keypad_state(event)
        if state == STATE_PRESSED:
            function_rearmed[0] = True
        elif state == STATE_HOLD:
            running = False
            SYS.exit_app()
        elif state == STATE_RELEASED and function_rearmed[0]:
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

    if state == STATE_RELEASED and game_state == WAITING:
        point = event.get("point")
        if point is not None:
            x, y = point
            place(x, y)


def loop():
    if not running:
        return
    render()

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()
    SYS.sleep_ms(1)


reset_game(True)
Input.clear()

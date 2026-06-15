import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS
ColorEffects = MatrixOS.ColorEffects
UI = MatrixOS.UI

FUNCTION_KEY = Input.function_key()

STATE_PRESSED = MatrixOS.Input.STATE_PRESSED
STATE_HOLD = MatrixOS.Input.STATE_HOLD
STATE_RELEASED = MatrixOS.Input.STATE_RELEASED

ROLLING = 0
CONFIRMED = 1

DOT = 0
NUMBER = 1

STATIC = 0
OFF = 1
BREATH = 2
STROBE = 3
SAW = 4

KEY_MODE = "Python Dice mode"
KEY_ROLLING_COLOR = "Python Dice rolling_color"
KEY_CONFIRMED_COLOR = "Python Dice confirmed_color"
KEY_DOT_FACES = "Python Dice dot_faces"
KEY_NUMBER_FACES = "Python Dice number_faces"
KEY_ROLLING_RAINBOW = "Python Dice rolling_rainbow"
KEY_CONFIRMED_RAINBOW = "Python Dice confirmed_rainbow"
KEY_ROLLING_UNDERGLOW = "Python Dice rolling_underglow"
KEY_CONFIRMED_UNDERGLOW = "Python Dice confirmed_underglow"
KEY_ROLLING_PERIOD = "Python Dice rolling_period"
KEY_CONFIRMED_PERIOD = "Python Dice confirmed_period"

mode = NVS.get(KEY_MODE, DOT)
rolling_color = NVS.get(KEY_ROLLING_COLOR, 0xFFFFFF)
confirmed_color = NVS.get(KEY_CONFIRMED_COLOR, 0xFFFFFF)
dot_faces = NVS.get(KEY_DOT_FACES, 6)
number_faces = NVS.get(KEY_NUMBER_FACES, 30)
rolling_rainbow = NVS.get(KEY_ROLLING_RAINBOW, True)
confirmed_rainbow = NVS.get(KEY_CONFIRMED_RAINBOW, False)
rolling_underglow = NVS.get(KEY_ROLLING_UNDERGLOW, SAW)
confirmed_underglow = NVS.get(KEY_CONFIRMED_UNDERGLOW, BREATH)
rolling_period = NVS.get(KEY_ROLLING_PERIOD, 300)
confirmed_period = NVS.get(KEY_CONFIRMED_PERIOD, 1000)

if mode not in (DOT, NUMBER):
    mode = DOT
if dot_faces < 2 or dot_faces > 9:
    dot_faces = 6
if number_faces < 1 or number_faces > 99:
    number_faces = 30
if rolling_underglow < STATIC or rolling_underglow > SAW:
    rolling_underglow = SAW
if confirmed_underglow < STATIC or confirmed_underglow > SAW:
    confirmed_underglow = BREATH
if rolling_period < 200 or rolling_period > 1700:
    rolling_period = 300
if confirmed_period < 200 or confirmed_period > 1700:
    confirmed_period = 1000

NUMBER_FONT = [
    0x1E, 0x29, 0x25, 0x1E,
    0x22, 0x3F, 0x20, 0x00,
    0x32, 0x29, 0x29, 0x26,
    0x12, 0x21, 0x25, 0x1A,
    0x0C, 0x0A, 0x3F, 0x08,
    0x17, 0x25, 0x25, 0x19,
    0x1E, 0x25, 0x25, 0x18,
    0x01, 0x39, 0x05, 0x03,
    0x1A, 0x25, 0x25, 0x1A,
    0x06, 0x29, 0x29, 0x1E,
]

DOTS = {
    1: [(3, 3)],
    2: [(1, 3), (5, 3)],
    3: [(0, 3), (3, 3), (6, 3)],
    4: [(1, 1), (5, 1), (1, 5), (5, 5)],
    5: [(1, 1), (5, 1), (1, 5), (5, 5), (3, 3)],
    6: [(0, 1), (3, 1), (6, 1), (0, 5), (3, 5), (6, 5)],
    7: [(0, 0), (3, 0), (6, 0), (3, 3), (0, 6), (3, 6), (6, 6)],
    8: [(0, 0), (3, 0), (6, 0), (0, 3), (6, 3), (0, 6), (3, 6), (6, 6)],
    9: [(0, 0), (3, 0), (6, 0), (0, 3), (3, 3), (6, 3), (0, 6), (3, 6), (6, 6)],
}

seed = SYS.millis() or 1
phase = ROLLING
rolling_start_ms = SYS.millis()
last_roll_ms = SYS.millis()
rolled_number = 1
running = True
underglow_enabled = LED.get_partition("Underglow") is not None


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


def next_random():
    global seed
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
    return seed


def random_number(upper, lower=1):
    return (next_random() % (upper - lower + 1)) + lower


def active_faces():
    return dot_faces if mode == DOT else number_faces


def roll_dice():
    global rolled_number, last_roll_ms
    faces = active_faces()
    old = rolled_number
    rolled_number = random_number(faces)
    if faces > 1:
        while rolled_number == old:
            rolled_number = random_number(faces)
    last_roll_ms = SYS.millis()


def start_roll():
    global phase, rolling_start_ms
    rolling_start_ms = SYS.millis()
    phase = ROLLING
    roll_dice()
    render()


def effect_color(color, effect, period, start_ms):
    if effect == OFF:
        return 0
    if effect == BREATH:
        return ColorEffects.color_breath(color, period, start_ms)
    if effect == STROBE:
        return ColorEffects.color_strobe(color, period, start_ms)
    if effect == SAW:
        return ColorEffects.color_saw(color, period, start_ms)
    return color


def render_dot(point, color):
    x, y = point
    LED.set_xy(x, y, color)
    LED.set_xy(x + 1, y, color)
    LED.set_xy(x, y + 1, color)
    LED.set_xy(x + 1, y + 1, color)


def render_dots(number, color):
    LED.clear()
    if number > 9:
        return
    for point in DOTS[number]:
        render_dot(point, color)


def render_number_at(x0, y0, number, color):
    for x in range(4):
        bits = NUMBER_FONT[number * 4 + x]
        for y in range(6):
            if bits & (1 << y):
                LED.set_xy(x0 + x, y0 + y, color)


def render_numbers(number, color):
    LED.clear()
    if number > 99:
        return
    digit1 = number // 10
    digit2 = number % 10
    if digit1 > 0:
        render_number_at(0, 1, digit1, color)
        render_number_at(4, 1, digit2, color)
    else:
        render_number_at(2, 1, digit2, color)


def render_dice_face(color):
    if mode == DOT:
        render_dots(rolled_number, color)
    else:
        render_numbers(rolled_number, color)


def render_underglow(effect, color, period):
    if not underglow_enabled:
        return
    LED.fill_partition("Underglow", effect_color(color, effect, period, rolling_start_ms))


def render():
    if phase == ROLLING:
        color = ColorEffects.rainbow() if rolling_rainbow else rolling_color
        render_dice_face(color)
        render_underglow(rolling_underglow, color, rolling_period)
    else:
        color = ColorEffects.rainbow(3000) if confirmed_rainbow else confirmed_color
        render_dice_face(color)
        render_underglow(confirmed_underglow, color, confirmed_period)
    LED.update()


def save_settings():
    NVS.set(KEY_MODE, mode)
    NVS.set(KEY_ROLLING_COLOR, rolling_color)
    NVS.set(KEY_CONFIRMED_COLOR, confirmed_color)
    NVS.set(KEY_DOT_FACES, dot_faces)
    NVS.set(KEY_NUMBER_FACES, number_faces)
    NVS.set(KEY_ROLLING_RAINBOW, rolling_rainbow)
    NVS.set(KEY_CONFIRMED_RAINBOW, confirmed_rainbow)
    NVS.set(KEY_ROLLING_UNDERGLOW, rolling_underglow)
    NVS.set(KEY_CONFIRMED_UNDERGLOW, confirmed_underglow)
    NVS.set(KEY_ROLLING_PERIOD, rolling_period)
    NVS.set(KEY_CONFIRMED_PERIOD, confirmed_period)


def dim_if_not(color, enabled):
    if enabled:
        return color
    r = ((color >> 16) & 0xFF) // 5
    g = ((color >> 8) & 0xFF) // 5
    b = (color & 0xFF) // 5
    return (r << 16) | (g << 8) | b


def active_color(is_rolling):
    if is_rolling:
        return ColorEffects.rainbow() if rolling_rainbow else rolling_color
    return ColorEffects.rainbow(3000) if confirmed_rainbow else confirmed_color


def dot_face_selector():
    global dot_faces
    dot_value = dot_faces
    selector_value = dot_faces - 2
    selector_ui = UI.UI("Face Selector", 0x00FFFF, True)

    display = UI.Number(1)
    display.set_color(0x00FFFF)
    display.set_value_func(lambda: dot_value)
    selector_ui.add(display, (5, 0))

    selector = UI.Selector((8, 1), 8)
    selector.set_name("Faces")
    selector.set_color(0x00FFFF)
    selector.set_value(selector_value)

    def on_change(value):
        nonlocal dot_value
        dot_value = value + 2

    selector.on_change(on_change)
    selector_ui.add(selector, (0, 7))

    def selector_input_handler(event):
        if event.get("id") != FUNCTION_KEY:
            return False
        if keypad_state(event) == STATE_RELEASED:
            selector_ui.exit()
        return True

    selector_ui.set_input_handler(selector_input_handler)
    selector_ui.start()
    dot_faces = selector.get_value() + 2
    save_settings()


def underglow_menu(is_rolling):
    global rolling_underglow, confirmed_underglow, rolling_period, confirmed_period

    color = rolling_color if is_rolling else confirmed_color
    rainbow = rolling_rainbow if is_rolling else confirmed_rainbow
    effect = rolling_underglow if is_rolling else confirmed_underglow
    period_steps = ((rolling_period if is_rolling else confirmed_period) // 100) - 2
    timestamp = SYS.millis()
    effect_ui = UI.UI("Underglow Effect Mode", color, True)

    def preview(target_effect):
        preview_color = ColorEffects.rainbow() if rainbow else color
        return effect_color(preview_color, target_effect, period_steps * 100 + 200, timestamp)

    def set_effect(value):
        nonlocal effect
        effect = value

    enable_btn = UI.Button("Static", 0x00FF00)
    enable_btn.set_color_func(lambda: dim_if_not(0x00FF00, effect != OFF))

    def toggle_enabled():
        nonlocal effect
        effect = STATIC if effect == OFF else OFF

    enable_btn.on_press(toggle_enabled)
    effect_ui.add(enable_btn, (0, 0))

    static_btn = UI.Button("Static")
    static_btn.set_color_func(lambda: dim_if_not(preview(STATIC), effect == STATIC))
    static_btn.on_press(lambda: set_effect(STATIC))
    effect_ui.add(static_btn, (2, 0))

    breath_btn = UI.Button("Breathing")
    breath_btn.set_color_func(lambda: dim_if_not(preview(BREATH), effect == BREATH))
    breath_btn.on_press(lambda: set_effect(BREATH))
    effect_ui.add(breath_btn, (3, 0))

    strobe_btn = UI.Button("Strobe")
    strobe_btn.set_color_func(lambda: dim_if_not(preview(STROBE), effect == STROBE))
    strobe_btn.on_press(lambda: set_effect(STROBE))
    effect_ui.add(strobe_btn, (4, 0))

    saw_btn = UI.Button("Saw")
    saw_btn.set_color_func(lambda: dim_if_not(preview(SAW), effect == SAW))
    saw_btn.on_press(lambda: set_effect(SAW))
    effect_ui.add(saw_btn, (5, 0))

    speed_selector = UI.Selector((8, 2), 16)
    speed_selector.set_name("Speed Selector")
    speed_selector.set_color(color)
    speed_selector.set_value(period_steps)
    speed_selector.on_change(lambda value: None)
    effect_ui.add(speed_selector, (0, 6))
    effect_ui.start()

    period = speed_selector.get_value() * 100 + 200
    if is_rolling:
        rolling_underglow = effect
        rolling_period = period
    else:
        confirmed_underglow = effect
        confirmed_period = period
    save_settings()


def open_settings():
    global running, rolling_color, confirmed_color, rolling_rainbow, confirmed_rainbow
    global mode, number_faces

    settings_ui = UI.UI("Settings", 0x00FFFF, True)
    preview_start_ms = SYS.millis()

    rolling_color_btn = UI.Button("Rolling Color")
    rolling_color_btn.set_size((1, 4))
    rolling_color_btn.set_color_func(lambda: active_color(True))

    def pick_rolling_color():
        global rolling_color
        if not rolling_rainbow:
            picked = UI.color_picker(rolling_color)
            if picked is not None:
                rolling_color = picked
                save_settings()

    rolling_color_btn.on_press(pick_rolling_color)
    settings_ui.add(rolling_color_btn, (0, 2))

    rolling_rainbow_toggle = UI.Toggle("Rolling Rainbow Mode", rolling_rainbow)
    rolling_rainbow_toggle.set_color_func(lambda: ColorEffects.rainbow())

    def set_rolling_rainbow(value):
        global rolling_rainbow
        rolling_rainbow = value
        save_settings()

    rolling_rainbow_toggle.on_press(set_rolling_rainbow)
    settings_ui.add(rolling_rainbow_toggle, (0, 0))

    rolling_underglow_btn = UI.Button("Rolling Underglow Effect")
    rolling_underglow_btn.set_color_func(lambda: effect_color(active_color(True), rolling_underglow, rolling_period, preview_start_ms))
    rolling_underglow_btn.set_enabled(underglow_enabled)
    rolling_underglow_btn.on_press(lambda: underglow_menu(True))
    settings_ui.add(rolling_underglow_btn, (0, 7))

    confirmed_color_btn = UI.Button("Confirmed Color")
    confirmed_color_btn.set_size((1, 4))
    confirmed_color_btn.set_color_func(lambda: active_color(False))

    def pick_confirmed_color():
        global confirmed_color
        if not confirmed_rainbow:
            picked = UI.color_picker(confirmed_color)
            if picked is not None:
                confirmed_color = picked
                save_settings()

    confirmed_color_btn.on_press(pick_confirmed_color)
    settings_ui.add(confirmed_color_btn, (7, 2))

    confirmed_rainbow_toggle = UI.Toggle("Confirmed Rainbow Mode", confirmed_rainbow)
    confirmed_rainbow_toggle.set_color_func(lambda: ColorEffects.rainbow())

    def set_confirmed_rainbow(value):
        global confirmed_rainbow
        confirmed_rainbow = value
        save_settings()

    confirmed_rainbow_toggle.on_press(set_confirmed_rainbow)
    settings_ui.add(confirmed_rainbow_toggle, (7, 0))

    confirmed_underglow_btn = UI.Button("Confirmed Underglow Effect")
    confirmed_underglow_btn.set_color_func(lambda: effect_color(active_color(False), confirmed_underglow, confirmed_period, preview_start_ms))
    confirmed_underglow_btn.set_enabled(underglow_enabled)
    confirmed_underglow_btn.on_press(lambda: underglow_menu(False))
    settings_ui.add(confirmed_underglow_btn, (7, 7))

    dot_mode_btn = UI.Button("Dot Mode")
    dot_mode_btn.set_color_func(lambda: dim_if_not(0xFF00FF, mode == DOT))

    def set_dot_mode():
        global mode
        mode = DOT
        save_settings()

    dot_mode_btn.on_press(set_dot_mode)
    settings_ui.add(dot_mode_btn, (3, 0))

    number_mode_btn = UI.Button("Number Mode")
    number_mode_btn.set_color_func(lambda: dim_if_not(0xFF5000, mode == NUMBER))

    def set_number_mode():
        global mode
        mode = NUMBER
        save_settings()

    number_mode_btn.on_press(set_number_mode)
    settings_ui.add(number_mode_btn, (4, 0))

    dot_faces_btn = UI.Button("Faces", 0x00FFFF)
    dot_faces_btn.set_size((4, 1))
    dot_faces_btn.set_enable_func(lambda: mode == DOT)
    dot_faces_btn.on_press(dot_face_selector)
    settings_ui.add(dot_faces_btn, (2, 7))

    number_faces_btn = UI.Button("Faces", 0x00FFFF)
    number_faces_btn.set_size((4, 1))
    number_faces_btn.set_enable_func(lambda: mode == NUMBER)

    def select_number_faces():
        global number_faces
        number_faces = UI.number_selector(number_faces, 0x00FFFF, "Face Selector", 1, 99)
        save_settings()

    number_faces_btn.on_press(select_number_faces)
    settings_ui.add(number_faces_btn, (2, 7))

    def settings_input_handler(event):
        global running
        if event.get("id") != FUNCTION_KEY:
            return False
        state = keypad_state(event)
        if state == STATE_HOLD:
            running = False
            save_settings()
            SYS.exit_app()
        elif state == STATE_RELEASED:
            save_settings()
            settings_ui.exit()
        return True

    settings_ui.set_input_handler(settings_input_handler)
    settings_ui.start()
    save_settings()
    render()


def handle_event(event):
    global running
    state = keypad_state(event)

    if event.get("id") == FUNCTION_KEY:
        if state == STATE_HOLD:
            open_settings()
        elif state == STATE_RELEASED:
            start_roll()
        return

    if state == STATE_PRESSED:
        start_roll()


def loop():
    global phase
    if not running:
        return

    now = SYS.millis()
    if phase == ROLLING:
        if now - last_roll_ms >= 100:
            roll_dice()
            render()
        if now - rolling_start_ms >= 3000:
            phase = CONFIRMED
            render()
    elif phase == CONFIRMED:
        render()

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()

    SYS.sleep_ms(1)


roll_dice()
render()
Input.clear()

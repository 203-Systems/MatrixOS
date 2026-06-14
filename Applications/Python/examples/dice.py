import MatrixOS
import MatrixOS_Utils


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
UI = MatrixOS.UI
UIUtility = MatrixOS.UIUtility
NVS = MatrixOS.NVS
Color = MatrixOS.Color
Point = MatrixOS.Point
Dimension = MatrixOS.Dimension
ColorEffects = MatrixOS.ColorEffects
Timer = MatrixOS.Timer


DOT = 0
NUMBER = 1

ROLLING = 0
CONFIRMED = 1

STATIC = 0
OFF = 1
BREATH = 2
STROBE = 3
SAW = 4

FRAME_MS = 16
SETTING_COLOR = Color(0x00FFFF)
WHITE = Color(0xFFFFFF)
BLACK = Color(0x000000)
ORANGE = Color(0xFFA500)

NVS_SCOPE = "PythonDice"

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

mode = DOT
dot_faces = 6
number_faces = 30
rolling_speed = 30
flashing_speed = 10

rolling_color = WHITE
confirmed_color = WHITE
rolling_rainbow = 1
confirmed_rainbow = 0
rolling_underglow_effect = SAW
confirmed_underglow_effect = BREATH
rolling_underglow_period = 300
confirmed_underglow_period = 1000

phase = ROLLING
rolled_number = 1
rolling_start_ms = 0
last_roll_ms = 0
random_seed = 1
app_running = True
settings_ui = None
underglow_enabled = LED.get_partition_by_name("Underglow") is not None

render_timer = Timer()
function_key = Input.function_key()


def nvs_hash(name):
    return MatrixOS_Utils.string_hash(NVS_SCOPE + "-" + name)


def bool_to_u8(value):
    if value:
        return 1
    return 0


def is_rolling_rainbow():
    return rolling_rainbow != 0


def is_confirmed_rainbow():
    return confirmed_rainbow != 0


def load_config():
    global mode
    global dot_faces
    global number_faces
    global rolling_speed
    global flashing_speed
    global rolling_color
    global confirmed_color
    global rolling_rainbow
    global confirmed_rainbow
    global rolling_underglow_effect
    global confirmed_underglow_effect
    global rolling_underglow_period
    global confirmed_underglow_period

    mode = clamp(NVS.get_u8(nvs_hash("mode"), mode), DOT, NUMBER)
    dot_faces = clamp(NVS.get_u8(nvs_hash("dot_faces"), dot_faces), 2, 9)
    number_faces = clamp(NVS.get_u8(nvs_hash("number_faces"), number_faces), 1, 99)
    rolling_speed = clamp(NVS.get_u8(nvs_hash("rolling_speed"), rolling_speed), 1, 255)
    flashing_speed = clamp(NVS.get_u8(nvs_hash("flashing_speed"), flashing_speed), 1, 255)
    rolling_color = Color(NVS.get_u32(nvs_hash("rolling_color"), rolling_color.rgb()))
    confirmed_color = Color(NVS.get_u32(nvs_hash("confirmed_color"), confirmed_color.rgb()))
    rolling_rainbow = bool_to_u8(NVS.get_u8(nvs_hash("rolling_rainbow"), rolling_rainbow))
    confirmed_rainbow = bool_to_u8(NVS.get_u8(nvs_hash("confirmed_rainbow"), confirmed_rainbow))
    rolling_underglow_effect = clamp(NVS.get_u8(nvs_hash("rolling_underglow_effect"), rolling_underglow_effect), STATIC, SAW)
    confirmed_underglow_effect = clamp(
        NVS.get_u8(nvs_hash("confirmed_underglow_effect"), confirmed_underglow_effect), STATIC, SAW
    )
    rolling_underglow_period = clamp(NVS.get_u16(nvs_hash("rolling_underglow_period"), rolling_underglow_period), 200, 1700)
    confirmed_underglow_period = clamp(
        NVS.get_u16(nvs_hash("confirmed_underglow_period"), confirmed_underglow_period), 200, 1700
    )


def save_config():
    NVS.set_u8(nvs_hash("mode"), mode)
    NVS.set_u8(nvs_hash("dot_faces"), dot_faces)
    NVS.set_u8(nvs_hash("number_faces"), number_faces)
    NVS.set_u8(nvs_hash("rolling_speed"), rolling_speed)
    NVS.set_u8(nvs_hash("flashing_speed"), flashing_speed)
    NVS.set_u32(nvs_hash("rolling_color"), rolling_color.rgb())
    NVS.set_u32(nvs_hash("confirmed_color"), confirmed_color.rgb())
    NVS.set_u8(nvs_hash("rolling_rainbow"), rolling_rainbow)
    NVS.set_u8(nvs_hash("confirmed_rainbow"), confirmed_rainbow)
    NVS.set_u8(nvs_hash("rolling_underglow_effect"), rolling_underglow_effect)
    NVS.set_u8(nvs_hash("confirmed_underglow_effect"), confirmed_underglow_effect)
    NVS.set_u16(nvs_hash("rolling_underglow_period"), rolling_underglow_period)
    NVS.set_u16(nvs_hash("confirmed_underglow_period"), confirmed_underglow_period)


def clamp(value, lower, upper):
    if value < lower:
        return lower
    if value > upper:
        return upper
    return value


def next_random():
    global random_seed

    random_seed = (random_seed * 1103515245 + 12345) & 0x7FFFFFFF
    return random_seed


def seed_random():
    global random_seed

    random_seed = SYS.millis() & 0x7FFFFFFF
    if random_seed == 0:
        random_seed = 1


def random_between(lower, upper):
    return (next_random() % (upper - lower + 1)) + lower


def current_faces():
    if mode == DOT:
        return dot_faces
    return number_faces


def roll_dice():
    global rolled_number
    global last_roll_ms

    faces = current_faces()
    last_roll_ms = SYS.millis()
    previous = rolled_number
    rolled_number = random_between(1, faces)

    if faces > 1:
        while rolled_number == previous:
            rolled_number = random_between(1, faces)


def start_roll():
    global phase
    global rolling_start_ms

    phase = ROLLING
    rolling_start_ms = SYS.millis()
    roll_dice()
    render_timer.record_current()


def selected_rolling_color():
    return rolling_color


def selected_confirmed_color():
    return confirmed_color


def display_color():
    if phase == ROLLING:
        if is_rolling_rainbow():
            return ColorEffects.rainbow()
        return selected_rolling_color()

    if is_confirmed_rainbow():
        return ColorEffects.rainbow(3000)
    return selected_confirmed_color()


def apply_effect(color, effect, period, offset):
    if effect == OFF:
        return BLACK
    if effect == BREATH:
        return ColorEffects.color_breath(color, period, offset)
    if effect == STROBE:
        return ColorEffects.color_strobe(color, period, offset)
    if effect == SAW:
        return ColorEffects.color_saw(color, period, offset)
    return color


def render_underglow(color):
    if not underglow_enabled:
        return

    now = SYS.millis()
    if phase == ROLLING:
        effect = rolling_underglow_effect
        period = rolling_underglow_period
    else:
        effect = confirmed_underglow_effect
        period = confirmed_underglow_period

    LED.fill_partition("Underglow", apply_effect(color, effect, period, now - rolling_start_ms))


def render_dot(x, y, color):
    LED.set_color_xy(x, y, color)
    LED.set_color_xy(x + 1, y, color)
    LED.set_color_xy(x, y + 1, color)
    LED.set_color_xy(x + 1, y + 1, color)


def render_dots(number, color):
    LED.clear()

    if number == 1:
        render_dot(3, 3, color)
    elif number == 2:
        render_dot(1, 3, color)
        render_dot(5, 3, color)
    elif number == 3:
        render_dot(0, 3, color)
        render_dot(3, 3, color)
        render_dot(6, 3, color)
    elif number == 4:
        render_dot(1, 1, color)
        render_dot(5, 1, color)
        render_dot(1, 5, color)
        render_dot(5, 5, color)
    elif number == 5:
        render_dot(1, 1, color)
        render_dot(5, 1, color)
        render_dot(1, 5, color)
        render_dot(5, 5, color)
        render_dot(3, 3, color)
    elif number == 6:
        render_dot(0, 1, color)
        render_dot(3, 1, color)
        render_dot(6, 1, color)
        render_dot(0, 5, color)
        render_dot(3, 5, color)
        render_dot(6, 5, color)
    elif number == 7:
        render_dot(0, 0, color)
        render_dot(3, 0, color)
        render_dot(6, 0, color)
        render_dot(3, 3, color)
        render_dot(0, 6, color)
        render_dot(3, 6, color)
        render_dot(6, 6, color)
    elif number == 8:
        render_dot(0, 0, color)
        render_dot(3, 0, color)
        render_dot(6, 0, color)
        render_dot(0, 3, color)
        render_dot(6, 3, color)
        render_dot(0, 6, color)
        render_dot(3, 6, color)
        render_dot(6, 6, color)
    elif number == 9:
        render_dot(0, 0, color)
        render_dot(3, 0, color)
        render_dot(6, 0, color)
        render_dot(0, 3, color)
        render_dot(3, 3, color)
        render_dot(6, 3, color)
        render_dot(0, 6, color)
        render_dot(3, 6, color)
        render_dot(6, 6, color)


def render_digit(x0, y0, digit, color):
    offset = digit * 4

    for x in range(4):
        column = NUMBER_FONT[offset + x]
        for y in range(6):
            if column & (1 << y):
                LED.set_color_xy(x0 + x, y0 + y, color)


def render_number(number, color):
    LED.clear()

    if number > 99:
        return

    tens = number // 10
    ones = number % 10
    if tens > 0:
        render_digit(0, 1, tens, color)
        render_digit(4, 1, ones, color)
    else:
        render_digit(2, 1, ones, color)


def render_game():
    color = display_color()

    if mode == DOT:
        render_dots(rolled_number, color)
    else:
        render_number(rolled_number, color)

    render_underglow(color)
    LED.update()


def update_rolling_state():
    global phase

    if phase != ROLLING:
        return

    now = SYS.millis()
    if now - last_roll_ms >= flashing_speed * 10:
        roll_dice()
    if now - rolling_start_ms >= rolling_speed * 100:
        phase = CONFIRMED


def pick_rolling_color():
    global rolling_color

    if is_rolling_rainbow():
        return

    picked = UIUtility.color_picker()
    if picked is not None:
        rolling_color = picked
        save_config()


def pick_confirmed_color():
    global confirmed_color

    if is_confirmed_rainbow():
        return

    picked = UIUtility.color_picker()
    if picked is not None:
        confirmed_color = picked
        save_config()


def toggle_rolling_rainbow():
    global rolling_rainbow
    if is_rolling_rainbow():
        rolling_rainbow = 0
    else:
        rolling_rainbow = 1
    save_config()


def toggle_confirmed_rainbow():
    global confirmed_rainbow
    if is_confirmed_rainbow():
        confirmed_rainbow = 0
    else:
        confirmed_rainbow = 1
    save_config()


def set_dot_mode():
    global mode
    mode = DOT
    save_config()
    if rolled_number > dot_faces:
        start_roll()


def set_number_mode():
    global mode
    mode = NUMBER
    save_config()


def select_dot_faces():
    global dot_faces
    dot_faces = UIUtility.number_selector_8x8(dot_faces, SETTING_COLOR, "Dot Faces", 2, 9)
    save_config()
    if mode == DOT and rolled_number > dot_faces:
        start_roll()


def select_number_faces():
    global number_faces
    number_faces = UIUtility.number_selector_8x8(number_faces, SETTING_COLOR, "Number Faces", 1, 99)
    save_config()
    if mode == NUMBER and rolled_number > number_faces:
        start_roll()


def rolling_color_button_color():
    if is_rolling_rainbow():
        return ColorEffects.rainbow()
    return selected_rolling_color()


def confirmed_color_button_color():
    if is_confirmed_rainbow():
        return ColorEffects.rainbow(3000)
    return selected_confirmed_color()


def rolling_rainbow_button_color():
    return ColorEffects.rainbow().dim_if_not(is_rolling_rainbow())


def confirmed_rainbow_button_color():
    return ColorEffects.rainbow(3000).dim_if_not(is_confirmed_rainbow())


def dot_mode_button_color():
    return Color(0xFF00FF).dim_if_not(mode == DOT)


def number_mode_button_color():
    return Color(0xFF5000).dim_if_not(mode == NUMBER)


def dot_faces_enabled():
    return mode == DOT


def number_faces_enabled():
    return mode == NUMBER


def render_settings_background():
    LED.clear()
    LED.fill_partition("Underglow", ORANGE)


def add_button(ui, x, y, width, height, name, color_func, press_func):
    button = UI.Button()
    button.set_name(name)
    button.set_size(Dimension(width, height))
    button.set_color_func(color_func)
    button.on_press(press_func)
    ui.add(button, Point(x, y))
    return button


def add_static_button(ui, x, y, width, height, name, color, press_func):
    button = UI.Button()
    button.set_name(name)
    button.set_size(Dimension(width, height))
    button.set_color(color)
    button.on_press(press_func)
    ui.add(button, Point(x, y))
    return button


def add_static_button_with_enable(ui, x, y, width, height, name, color, press_func, enable_func):
    button = add_static_button(ui, x, y, width, height, name, color, press_func)
    button.set_enable_func(enable_func)
    return button


def effect_period_selector_value(period):
    return clamp((period - 200) // 100, 0, 15)


def make_effect_color(effect, selected, rainbow, color, period):
    if rainbow:
        base = ColorEffects.rainbow()
    else:
        base = color
    return apply_effect(base, effect, period, SYS.millis()).dim_if_not(effect == selected)


def open_underglow_menu(rolling):
    global rolling_underglow_effect
    global confirmed_underglow_effect
    global rolling_underglow_period
    global confirmed_underglow_period
    global settings_ui
    global app_running

    if rolling:
        effect = rolling_underglow_effect
        period = rolling_underglow_period
        rainbow = is_rolling_rainbow()
        color = selected_rolling_color()
    else:
        effect = confirmed_underglow_effect
        period = confirmed_underglow_period
        rainbow = is_confirmed_rainbow()
        color = selected_confirmed_color()
    effect_ui = UI.UI("Underglow", ORANGE, True)
    effect_ui.allow_exit(False)
    effect_ui.set_fps(60)
    selected_effect = [effect]
    selected_period = [effect_period_selector_value(period)]

    def process_effect_input():
        global app_running

        event = effect_ui.pull_input()
        while event is not None:
            if event.is_function_key():
                if event.is_hold():
                    app_running = False
                    effect_ui.exit()
                    SYS.exit_app()
                elif event.is_released():
                    effect_ui.exit()
            event = effect_ui.pull_input()

    def set_effect(value):
        selected_effect[0] = value

    def set_static():
        set_effect(STATIC)

    def toggle_effect_enabled():
        if selected_effect[0] == OFF:
            set_effect(STATIC)
        else:
            set_effect(OFF)

    def set_breath():
        set_effect(BREATH)

    def set_strobe():
        set_effect(STROBE)

    def set_saw():
        set_effect(SAW)

    def static_color():
        return make_effect_color(STATIC, selected_effect[0], rainbow, color, selected_period[0] * 100 + 200)

    def enabled_color():
        return Color(0x00FF00).dim_if_not(selected_effect[0] != OFF)

    def breath_color():
        return make_effect_color(BREATH, selected_effect[0], rainbow, color, selected_period[0] * 100 + 200)

    def strobe_color():
        return make_effect_color(STROBE, selected_effect[0], rainbow, color, selected_period[0] * 100 + 200)

    def saw_color():
        return make_effect_color(SAW, selected_effect[0], rainbow, color, selected_period[0] * 100 + 200)

    def get_period_selector():
        return selected_period[0]

    def set_period_selector(value):
        selected_period[0] = value

    effect_ui.set_loop_func(process_effect_input)
    effect_ui.set_pre_render_func(render_settings_background)
    enable_button = add_button(effect_ui, 0, 0, 1, 1, "Static", enabled_color, toggle_effect_enabled)
    static_button = add_button(effect_ui, 2, 0, 1, 1, "Static", static_color, set_static)
    breath_button = add_button(effect_ui, 3, 0, 1, 1, "Breath", breath_color, set_breath)
    strobe_button = add_button(effect_ui, 4, 0, 1, 1, "Strobe", strobe_color, set_strobe)
    saw_button = add_button(effect_ui, 5, 0, 1, 1, "Saw", saw_color, set_saw)

    speed_selector = UI.Selector()
    speed_selector.set_name("Speed")
    speed_selector.set_color(color)
    speed_selector.set_dimension(Dimension(8, 2))
    speed_selector.set_count(16)
    speed_selector.set_value_func(get_period_selector)
    speed_selector.on_change(set_period_selector)
    effect_ui.add(speed_selector, Point(0, 6))

    effect_ui.start()
    effect_ui.close()

    if rolling:
        rolling_underglow_effect = selected_effect[0]
        rolling_underglow_period = selected_period[0] * 100 + 200
    else:
        confirmed_underglow_effect = selected_effect[0]
        confirmed_underglow_period = selected_period[0] * 100 + 200

    save_config()


def open_rolling_underglow_menu():
    open_underglow_menu(True)


def open_confirmed_underglow_menu():
    open_underglow_menu(False)


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

    settings_ui = UI.UI("Dice Settings", ORANGE, True)
    settings_ui.allow_exit(False)
    settings_ui.set_fps(60)
    settings_ui.set_loop_func(process_settings_input)
    settings_ui.set_pre_render_func(render_settings_background)

    rolling_rainbow_button = add_button(
        settings_ui, 0, 0, 1, 1, "Rolling Rainbow", rolling_rainbow_button_color, toggle_rolling_rainbow
    )
    confirmed_rainbow_button = add_button(
        settings_ui, 7, 0, 1, 1, "Confirmed Rainbow", confirmed_rainbow_button_color, toggle_confirmed_rainbow
    )
    rolling_color_button = add_button(
        settings_ui, 0, 2, 1, 4, "Rolling Color", rolling_color_button_color, pick_rolling_color
    )
    confirmed_color_button = add_button(
        settings_ui, 7, 2, 1, 4, "Confirmed Color", confirmed_color_button_color, pick_confirmed_color
    )
    dot_mode_button = add_button(settings_ui, 3, 0, 1, 1, "Dot Mode", dot_mode_button_color, set_dot_mode)
    number_mode_button = add_button(settings_ui, 4, 0, 1, 1, "Number Mode", number_mode_button_color, set_number_mode)
    dot_faces_button = add_static_button_with_enable(
        settings_ui, 2, 7, 4, 1, "Faces", SETTING_COLOR, select_dot_faces, dot_faces_enabled
    )
    number_faces_button = add_static_button_with_enable(
        settings_ui, 2, 7, 4, 1, "Faces", SETTING_COLOR, select_number_faces, number_faces_enabled
    )

    if underglow_enabled:
        rolling_underglow_button = add_button(
            settings_ui, 0, 7, 1, 1, "Rolling Glow", rolling_color_button_color, open_rolling_underglow_menu
        )
        confirmed_underglow_button = add_button(
            settings_ui, 7, 7, 1, 1, "Confirmed Glow", confirmed_color_button_color, open_confirmed_underglow_menu
        )

    settings_ui.start()
    settings_ui.close()
    settings_ui = None


def handle_input(event):
    if not event.is_function_key():
        return

    if event.is_hold():
        open_settings_ui()
    elif event.is_released():
        start_roll()


def process_input():
    event = Input.get_event()
    while event is not None:
        handle_input(event)
        event = Input.get_event()


def render_if_needed():
    if render_timer.tick(FRAME_MS, True):
        render_game()


def loop():
    if not app_running:
        return

    process_input()
    update_rolling_state()
    render_if_needed()


load_config()
seed_random()
start_roll()
Input.clear_input_buffer()

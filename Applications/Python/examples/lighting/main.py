import MatrixOS


LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS
ColorEffects = MatrixOS.ColorEffects
UI = MatrixOS.UI

FUNCTION_KEY = Input.function_key()
STATE_PRESSED = Input.STATE_PRESSED
STATE_HOLD = Input.STATE_HOLD
STATE_RELEASED = Input.STATE_RELEASED

RGB = 0
TEMPERATURE = 1

STATIC = 0
BREATH = 1
STROBE = 2
SAW = 3

KEY_MODE = "Python Lighting mode"
KEY_COLOR = "Python Lighting color"
KEY_RGB_EFFECT = "Python Lighting rgb_effect"
KEY_RGB_BPM = "Python Lighting rgb_effect_bpm"
KEY_TEMPERATURE_COLOR = "Python Lighting temperature_color"
KEY_TEMPERATURE_EFFECT = "Python Lighting temperature_effect"
KEY_TEMPERATURE_BPM = "Python Lighting temperature_effect_bpm"

mode = NVS.get(KEY_MODE, RGB)
color = NVS.get(KEY_COLOR, 0xFFFFFF)
rgb_effect = NVS.get(KEY_RGB_EFFECT, STATIC)
rgb_effect_bpm = NVS.get(KEY_RGB_BPM, 60)
temperature_color = NVS.get(KEY_TEMPERATURE_COLOR, 0xFFFFFF)
temperature_effect = NVS.get(KEY_TEMPERATURE_EFFECT, STATIC)
temperature_effect_bpm = NVS.get(KEY_TEMPERATURE_BPM, 60)

if mode not in (RGB, TEMPERATURE):
    mode = RGB
if rgb_effect < STATIC or rgb_effect > SAW:
    rgb_effect = STATIC
if temperature_effect < STATIC or temperature_effect > SAW:
    temperature_effect = STATIC
if rgb_effect_bpm < 10 or rgb_effect_bpm > 299:
    rgb_effect_bpm = 60
if temperature_effect_bpm < 10 or temperature_effect_bpm > 299:
    temperature_effect_bpm = 60

start_ms = SYS.millis()
last_render_ms = 0
running = True


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


def rgb_scale(value, amount):
    r = ((value >> 16) & 0xFF) * amount // 255
    g = ((value >> 8) & 0xFF) * amount // 255
    b = (value & 0xFF) * amount // 255
    return (r << 16) | (g << 8) | b


def dim_if_not(value, enabled):
    return value if enabled else rgb_scale(value, 51)


def period_from_bpm(bpm):
    if bpm < 1:
        bpm = 1
    return 60000 // bpm


def effect_color(value, effect, bpm, offset):
    period = period_from_bpm(bpm)
    if effect == BREATH:
        return ColorEffects.color_breath(value, period, offset)
    if effect == STROBE:
        return ColorEffects.color_strobe(value, period, offset)
    if effect == SAW:
        return ColorEffects.color_saw(value, period, offset)
    return value


def active_color():
    if mode == TEMPERATURE:
        return effect_color(temperature_color, temperature_effect, temperature_effect_bpm, start_ms)
    return effect_color(color, rgb_effect, rgb_effect_bpm, start_ms)


def save_settings():
    NVS.set(KEY_MODE, mode)
    NVS.set(KEY_COLOR, color)
    NVS.set(KEY_RGB_EFFECT, rgb_effect)
    NVS.set(KEY_RGB_BPM, rgb_effect_bpm)
    NVS.set(KEY_TEMPERATURE_COLOR, temperature_color)
    NVS.set(KEY_TEMPERATURE_EFFECT, temperature_effect)
    NVS.set(KEY_TEMPERATURE_BPM, temperature_effect_bpm)


def render():
    LED.fill(active_color())
    LED.update()


def make_bpm_tapper(get_bpm, set_bpm):
    taps = []
    component = UI.CustomComponent((8, 4))

    tap_points = (
        (0, 0, 0xFF00FF), (1, 0, 0xFF00FF), (2, 0, 0xFF00FF),
        (1, 1, 0xFF00FF), (1, 2, 0xFF00FF), (1, 3, 0xFF00FF),
        (3, 0, 0xFFFFFF), (4, 0, 0xFFFFFF), (5, 0, 0xFFFFFF),
        (3, 1, 0xFFFFFF), (5, 1, 0xFFFFFF),
        (3, 2, 0xFFFFFF), (4, 2, 0xFFFFFF), (5, 2, 0xFFFFFF),
        (3, 3, 0xFFFFFF), (5, 3, 0xFFFFFF),
        (6, 0, 0xFF00FF), (7, 0, 0xFF00FF),
        (6, 1, 0xFF00FF), (7, 1, 0xFF00FF),
        (6, 2, 0xFF00FF), (6, 3, 0xFF00FF),
    )

    def trim_taps(now):
        if len(taps) and now - taps[-1] > 3000000:
            taps.clear()
        while len(taps) and now - taps[0] > 6000000:
            taps.pop(0)

    def calculate_bpm():
        if len(taps) < 3:
            return
        taps_to_use = min(len(taps), 5)
        start_index = len(taps) - taps_to_use
        total_interval = taps[-1] - taps[start_index]
        avg_interval = total_interval // (taps_to_use - 1)
        if avg_interval <= 0:
            return
        bpm_value = 60000000 // avg_interval
        if bpm_value > 299:
            bpm_value = 299
        if bpm_value < 30:
            bpm_value = 30
        set_bpm(bpm_value)

    def render_tapper(origin):
        now = SYS.micros()
        trim_taps(now)
        if len(taps) > 0 and len(taps) < 3:
            ox, oy = origin
            for y in range(4):
                for x in range(8):
                    LED.set_xy(ox + x, oy + y, 0)
            for x, y, point_color in tap_points:
                LED.set_xy(ox + x, oy + y, point_color)
        return True

    def handle_tap(xy, keypad):
        state = keypad.get("state", -1)
        if state == STATE_RELEASED:
            taps.append(SYS.micros())
            if len(taps) > 10:
                taps.pop(0)
            calculate_bpm()
        elif state == STATE_HOLD:
            UI.text_scroll("BPM", 0xFF00FF)
            taps.clear()
        return True

    component.set_render_func(render_tapper)
    component.set_key_func(handle_tap)
    return component


def make_bpm_modifier(get_bpm, set_bpm):
    component = UI.CustomComponent((8, 1))
    modifiers = (-50, -20, -5, -1, 1, 5, 20, 50)
    gradient = (255, 127, 64, 32, 32, 64, 127, 255)

    def render_modifier(origin):
        ox, oy = origin
        for x in range(8):
            LED.set_xy(ox + x, oy, rgb_scale(0xFF00FF, gradient[x]))
        return True

    def handle_modifier(xy, keypad):
        if keypad.get("state", -1) == STATE_PRESSED:
            x, _ = xy
            bpm_value = get_bpm() + modifiers[x]
            if bpm_value < 10:
                bpm_value = 10
            if bpm_value > 299:
                bpm_value = 299
            set_bpm(bpm_value)
        return True

    component.set_render_func(render_modifier)
    component.set_key_func(handle_modifier)
    return component


def effect_menu(target_mode):
    global rgb_effect, rgb_effect_bpm, temperature_effect, temperature_effect_bpm, start_ms

    base = color if target_mode == RGB else temperature_color
    selected = rgb_effect if target_mode == RGB else temperature_effect
    bpm = rgb_effect_bpm if target_mode == RGB else temperature_effect_bpm
    start_ms = SYS.millis()

    effect_ui = UI.UI("Effect Mode", base, True)

    def preview(effect):
        return effect_color(base, effect, bpm, start_ms)

    def set_effect(effect):
        nonlocal selected
        selected = effect

    static_btn = UI.Button("Static")
    static_btn.set_color_func(lambda: dim_if_not(preview(STATIC), selected == STATIC))
    static_btn.on_press(lambda: set_effect(STATIC))
    effect_ui.add(static_btn, (2, 0))

    breath_btn = UI.Button("Breathing")
    breath_btn.set_color_func(lambda: dim_if_not(preview(BREATH), selected == BREATH))
    breath_btn.on_press(lambda: set_effect(BREATH))
    effect_ui.add(breath_btn, (3, 0))

    strobe_btn = UI.Button("Strobe")
    strobe_btn.set_color_func(lambda: dim_if_not(preview(STROBE), selected == STROBE))
    strobe_btn.on_press(lambda: set_effect(STROBE))
    effect_ui.add(strobe_btn, (4, 0))

    saw_btn = UI.Button("Saw")
    saw_btn.set_color_func(lambda: dim_if_not(preview(SAW), selected == SAW))
    saw_btn.on_press(lambda: set_effect(SAW))
    effect_ui.add(saw_btn, (5, 0))

    display = UI.Number(3)
    display.set_name("BPM")
    display.set_color(0xFF00FF)
    display.set_value_func(lambda: bpm)
    effect_ui.add(display, (-1, 2))

    def get_bpm():
        return bpm

    def set_bpm(value):
        nonlocal bpm
        bpm = value

    effect_ui.add(make_bpm_tapper(get_bpm, set_bpm), (0, 2))
    effect_ui.add(make_bpm_modifier(get_bpm, set_bpm), (0, 7))

    def effect_input_handler(event):
        if event.get("id") != FUNCTION_KEY:
            return False
        if keypad_state(event) == STATE_RELEASED:
            effect_ui.exit()
        return True

    effect_ui.set_input_handler(effect_input_handler)
    effect_ui.start()

    if target_mode == RGB:
        rgb_effect = selected
        rgb_effect_bpm = bpm
    else:
        temperature_effect = selected
        temperature_effect_bpm = bpm
    save_settings()
    start_ms = SYS.millis()


def open_settings():
    global mode, color, temperature_color, running, start_ms

    settings_ui = UI.UI("Settings", 0x00FFFF, True)
    start_ms = SYS.millis()

    rgb_btn = UI.Button("RGB Mode")
    rgb_btn.set_color_func(lambda: dim_if_not(0xFF00FF, mode == RGB))

    def set_rgb():
        global mode
        mode = RGB
        save_settings()

    rgb_btn.on_press(set_rgb)
    settings_ui.add(rgb_btn, (3, 0))

    temp_btn = UI.Button("Temperature Mode")
    temp_btn.set_color_func(lambda: dim_if_not(0xFFFFFF, mode == TEMPERATURE))

    def set_temperature():
        global mode
        mode = TEMPERATURE
        save_settings()

    temp_btn.on_press(set_temperature)
    settings_ui.add(temp_btn, (4, 0))

    color_btn = UI.Button("Color")
    color_btn.set_size((8, 2))
    color_btn.set_color_func(lambda: color)
    color_btn.set_enable_func(lambda: mode == RGB)

    def pick_color():
        global color
        picked = UI.color_picker(color)
        if picked is not None:
            color = picked
            save_settings()

    color_btn.on_press(pick_color)
    settings_ui.add(color_btn, (0, 6))

    rgb_effect_btn = UI.Button("Effect & Speed")
    rgb_effect_btn.set_size((1, 2))
    rgb_effect_btn.set_color_func(lambda: effect_color(color, rgb_effect, rgb_effect_bpm, start_ms))
    rgb_effect_btn.set_enable_func(lambda: mode == RGB)
    rgb_effect_btn.on_press(lambda: effect_menu(RGB))
    settings_ui.add(rgb_effect_btn, (0, 3))

    temp_color_btn = UI.Button("Temperature")
    temp_color_btn.set_size((8, 2))
    temp_color_btn.set_color_func(lambda: temperature_color)
    temp_color_btn.set_enable_func(lambda: mode == TEMPERATURE)

    def pick_temperature():
        global temperature_color
        picked = UI.color_picker(temperature_color)
        if picked is not None:
            temperature_color = picked
            save_settings()

    temp_color_btn.on_press(pick_temperature)
    settings_ui.add(temp_color_btn, (0, 6))

    temp_effect_btn = UI.Button("Effect & Speed")
    temp_effect_btn.set_size((1, 2))
    temp_effect_btn.set_color_func(lambda: effect_color(temperature_color, temperature_effect, temperature_effect_bpm, start_ms))
    temp_effect_btn.set_enable_func(lambda: mode == TEMPERATURE)
    temp_effect_btn.on_press(lambda: effect_menu(TEMPERATURE))
    settings_ui.add(temp_effect_btn, (0, 3))

    function_rearmed = [False]

    def settings_input_handler(event):
        global running
        if event.get("id") != FUNCTION_KEY:
            return False
        state = keypad_state(event)
        if state == STATE_PRESSED:
            function_rearmed[0] = True
        elif state == STATE_HOLD:
            running = False
            save_settings()
            SYS.exit_app()
        elif state == STATE_RELEASED and function_rearmed[0]:
            save_settings()
            settings_ui.exit()
        return True

    settings_ui.set_input_handler(settings_input_handler)
    settings_ui.start()
    save_settings()
    start_ms = SYS.millis()
    render()


def handle_event(event):
    global running
    if event.get("id") != FUNCTION_KEY:
        return
    state = keypad_state(event)
    if state == STATE_PRESSED:
        open_settings()
    elif state == STATE_HOLD:
        running = False
        SYS.exit_app()


def loop():
    global last_render_ms
    if not running:
        return

    now = SYS.millis()
    if now - last_render_ms >= 16:
        last_render_ms = now
        render()

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()
    SYS.sleep_ms(1)


render()
Input.clear()

import MatrixOS


TAG = "PyProbe"
LED = MatrixOS.LED
SYS = MatrixOS.SYS
Input = MatrixOS.Input
NVS = MatrixOS.NVS
Logging = MatrixOS.Logging
ColorEffects = MatrixOS.ColorEffects

FUNCTION_KEY = Input.function_key()
STATE_HOLD = MatrixOS.Input.STATE_HOLD
STATE_RELEASED = MatrixOS.Input.STATE_RELEASED

step_index = 0
loop_count = 0
last_log_ms = 0
last_frame_ms = 0
running = True


def log(message):
    text = str(message)
    print(TAG + ": " + text)
    Logging.info(TAG, text)


def warn(message):
    text = str(message)
    print(TAG + " WARN: " + text)
    Logging.warning(TAG, text)


def fail(message):
    text = str(message)
    print(TAG + " ERROR: " + text)
    Logging.error(TAG, text)
    LED.clear()
    LED.set_xy(0, 0, 0xFF0000)
    LED.set_xy(1, 0, 0xFF0000)
    LED.set_xy(0, 1, 0xFF0000)
    LED.update()


def mark(color):
    global step_index
    x = step_index % 8
    y = step_index // 8
    if y < 8:
        LED.set_xy(x, y, color)
        LED.update()
    step_index += 1


def run_step(name, fn):
    log("BEGIN " + name)
    mark(0x202020)
    try:
        result = fn()
        mark(0x00FF00)
        log("OK " + name + ": " + str(result))
        return result
    except Exception as error:
        fail("FAIL " + name + ": " + str(error))
        raise


def keypad_state(event):
    keypad = event.get("keypad")
    if keypad is None:
        return -1
    return keypad.get("state", -1)


def step_basic():
    return "ms=" + str(SYS.millis()) + " version=" + SYS.version()


def step_led():
    count = LED.count()
    partitions = LED.partitions()
    LED.clear()
    LED.set_xy(0, 0, 0x0033FF)
    LED.set_xy(7, 0, 0x0033FF)
    LED.set_xy(0, 7, 0x0033FF)
    LED.set_xy(7, 7, 0x0033FF)
    LED.update()
    return "count=" + str(count) + " partitions=" + str(len(partitions))


def step_underglow():
    partition = LED.get_partition("Underglow")
    if partition is None:
        return "no underglow"
    LED.fill_partition("Underglow", 0x3300FF)
    LED.update()
    return "underglow size=" + str(partition.get("size", -1))


def step_nvs():
    key = "Python Runtime Probe"
    old = NVS.get(key, 0)
    new_value = old + 1
    NVS.set(key, new_value)
    read_back = NVS.get(key, 0)
    if read_back != new_value:
        raise RuntimeError("NVS mismatch " + str(read_back) + " != " + str(new_value))
    return "old=" + str(old) + " new=" + str(read_back)


def step_color_effects():
    a = ColorEffects.rainbow()
    b = ColorEffects.color_breath(0x00FF00, 1000, SYS.millis())
    c = ColorEffects.color_strobe(0xFF0000, 500, SYS.millis())
    d = ColorEffects.color_saw(0x0000FF, 700, SYS.millis())
    LED.set_xy(2, 2, a)
    LED.set_xy(3, 2, b)
    LED.set_xy(4, 2, c)
    LED.set_xy(5, 2, d)
    LED.update()
    return str(a) + "," + str(b) + "," + str(c) + "," + str(d)


def step_input():
    Input.clear()
    clusters = Input.clusters()
    primary = Input.primary_grid_cluster()
    log("function_key=" + str(FUNCTION_KEY))
    log("clusters=" + str(clusters))
    log("primary=" + str(primary))
    return "clusters=" + str(len(clusters))


def render_heartbeat():
    now = SYS.millis()
    color = ColorEffects.rainbow(2000)
    LED.clear()
    LED.set_xy((loop_count // 4) % 8, 3, color)
    LED.set_xy(loop_count % 8, 4, 0xFFFFFF)
    if LED.get_partition("Underglow") is not None:
        LED.fill_partition("Underglow", color)
    LED.update()
    return now


def startup():
    log("startup")
    run_step("basic", step_basic)
    run_step("led", step_led)
    run_step("underglow", step_underglow)
    run_step("nvs", step_nvs)
    run_step("color_effects", step_color_effects)
    run_step("input", step_input)
    log("startup complete")


def handle_event(event):
    global running
    log("event=" + str(event))
    if event.get("id") == FUNCTION_KEY:
        state = keypad_state(event)
        if state == STATE_HOLD:
            log("exit requested")
            running = False
            SYS.exit_app()
        elif state == STATE_RELEASED:
            log("fn released")


def loop():
    global loop_count, last_log_ms, last_frame_ms

    if not running:
        return

    now = SYS.millis()
    if now - last_frame_ms >= 125:
        loop_count += 1
        last_frame_ms = render_heartbeat()

    if now - last_log_ms >= 1000:
        last_log_ms = now
        log("loop count=" + str(loop_count) + " ms=" + str(now))

    event = Input.get_event()
    while event is not None:
        handle_event(event)
        event = Input.get_event()

    SYS.sleep_ms(1)


startup()

# Smoke test: UI input callback
#
# Exercises:
#   UI creation, SetInputHandler, Start, Close
#   InputClass filtering inside the handler
#
# Expected: UI opens, processes a few events, then closes cleanly.

import MatrixOS
from MatrixOS_UI import UI
from MatrixOS_Color import Color
import MatrixOS_InputClass as InputClass

event_count = 0
MAX_EVENTS = 10

def on_input(event):
    global event_count
    event_count = event_count + 1
    cls = event.InputClass()
    cid = event.Id().ClusterId()
    mid = event.Id().MemberId()

    msg = "[smoke] on_input #" + str(event_count) + " id=(" + str(cid) + "," + str(mid) + ") class=" + str(cls)

    if cls == InputClass.KEYPAD:
        info = event.Keypad()
        if info is not None:
            msg = msg + " state=" + str(info.State())
    else:
        msg = msg + " (non-keypad, skipped payload)"

    print(msg)
    return True  # consumed

def run():
    global event_count
    print("[smoke] ui_callback start")

    # Create UI
    ui = UI("SmokeUI", Color(0x00FF00))
    print("[smoke] UI created")

    # Install input handler
    ok = ui.SetInputHandler(on_input)
    print("[smoke] SetInputHandler -> " + str(ok))

    # Set a short FPS
    ui.SetFPS(10)

    # Start — this is blocking; it will run the UI loop.
    # The user must press the function key (or equivalent exit) to leave.
    print("[smoke] Starting UI (press fn-key to exit)...")
    ui.Start()

    print("[smoke] UI exited, events received: " + str(event_count))

    # Explicit close for deterministic teardown
    ui.Close()
    print("[smoke] ui_callback done")

run()

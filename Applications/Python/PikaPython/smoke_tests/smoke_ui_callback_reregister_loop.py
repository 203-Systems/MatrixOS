# Smoke test: UI callback re-registration loop stability
#
# Exercises:
#   Repeated SetInputHandler() calls on a single UI instance
#   Checks for hardfault, double-free, or callback context leak
#
# Expected: no crash; all re-registrations complete; final "done" message prints.

import MatrixOS
from MatrixOS_UI import UI
from MatrixOS_Color import Color
import MatrixOS_InputClass as InputClass

ITERATIONS = 20

call_count = 0

def make_handler(tag):
    def handler(event):
        global call_count
        call_count = call_count + 1
        return True
    return handler

def run():
    global call_count
    print("[smoke] ui_callback_reregister_loop start (" + str(ITERATIONS) + " iterations)")

    ui = UI("ReregLoop", Color(0xFF8800))
    ui.AllowExit(False)
    ui.SetFPS(30)
    print("[smoke]  UI created")

    for i in range(ITERATIONS):
        ok = ui.SetInputHandler(make_handler(i))
        print("[smoke]  iter " + str(i) + " SetInputHandler -> " + str(ok))

    ui.Close()
    print("[smoke] ui_callback_reregister_loop done — " + str(ITERATIONS) + " re-registrations, no crash")

run()

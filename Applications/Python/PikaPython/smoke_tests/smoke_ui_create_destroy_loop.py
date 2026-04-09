# Smoke test: UI create/destroy loop stability
#
# Exercises:
#   Repeated UI() construction and Close() teardown
#   Checks for hardfault or memory corruption over many iterations
#
# Expected: no crash; all iterations complete; final "done" message prints.

import MatrixOS
from MatrixOS_UI import UI
from MatrixOS_Color import Color

ITERATIONS = 20

def run():
    print("[smoke] ui_create_destroy_loop start (" + str(ITERATIONS) + " iterations)")

    for i in range(ITERATIONS):
        ui = UI("Loop" + str(i), Color(0x0000FF))
        ui.AllowExit(False)
        ui.SetFPS(30)
        ui.Close()
        print("[smoke]  iter " + str(i) + " OK")

    print("[smoke] ui_create_destroy_loop done — " + str(ITERATIONS) + " cycles, no crash")

run()

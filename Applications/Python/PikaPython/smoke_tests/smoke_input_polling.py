# Smoke test: Input polling API
#
# Exercises:
#   MatrixOS.Input.GetEvent()
#   MatrixOS.Input.GetState()
#   MatrixOS.Input.FunctionKey()
#   MatrixOS.Input.ClearQueue()
#   MatrixOS.Input.ClearState()
#
# Expected: no crash; print confirms each call returns the correct type.

import MatrixOS
import MatrixOS_InputClass as InputClass
from MatrixOS_KeyState import KeyState

def run():
    print("[smoke] input_polling start")

    # --- FunctionKey ---
    fk = MatrixOS.Input.FunctionKey()
    print("[smoke] FunctionKey() -> cluster=" + str(fk.ClusterId()) +
          " member=" + str(fk.MemberId()))

    # --- ClearQueue / ClearState ---
    MatrixOS.Input.ClearQueue()
    MatrixOS.Input.ClearState()
    print("[smoke] ClearQueue + ClearState OK")

    # --- Non-blocking GetEvent ---
    evt = MatrixOS.Input.GetEvent(0)
    if evt is None:
        print("[smoke] GetEvent(0) -> None (idle, expected)")
    else:
        print("[smoke] GetEvent(0) -> class=" + str(evt.InputClass()))

    # --- GetState on function key ---
    snap = MatrixOS.Input.GetState(fk)
    if snap is None:
        print("[smoke] GetState(fk) -> None (no state yet, OK)")
    else:
        cls = snap.InputClass()
        print("[smoke] GetState(fk) -> class=" + str(cls))
        if cls == InputClass.KEYPAD:
            info = snap.Keypad()
            if info is not None:
                print("[smoke]   state=" + str(info.State()) +
                      " force=" + str(info.Force()))

    # --- Blocking poll for a few events ---
    print("[smoke] Polling 5 events (1s timeout each)...")
    for i in range(5):
        evt = MatrixOS.Input.GetEvent(1000)
        if evt is None:
            print("[smoke]  " + str(i) + ": timeout")
            continue
        cid = evt.Id().ClusterId()
        mid = evt.Id().MemberId()
        cls = evt.InputClass()
        msg = "[smoke]  " + str(i) + ": id=(" + str(cid) + "," + str(mid) + ") class=" + str(cls)
        if cls == InputClass.KEYPAD:
            kp = evt.Keypad()
            if kp is not None:
                msg = msg + " state=" + str(kp.State()) + " force=" + str(kp.Force())
        print(msg)

    print("[smoke] input_polling done")

run()

# Smoke test: Value wrappers
#
# Exercises:
#   InputId  — construction, accessors, FunctionKey(), equality
#   Point    — construction, accessors, arithmetic
#   Color    — construction, component access, Dim
#   InputCluster / KeypadInfo — read-only accessors via live data
#
# Expected: no crash; printed values are sane.

import MatrixOS
from MatrixOS_InputId import InputId
from MatrixOS_Point import Point
from MatrixOS_Color import Color
import MatrixOS_InputClass as InputClass

def run():
    print("[smoke] value_wrappers start")

    # --- InputId ---
    fk = InputId.FunctionKey()
    inv = InputId.Invalid()
    print("[smoke] InputId.FunctionKey() -> (" +
          str(fk.ClusterId()) + "," + str(fk.MemberId()) + ")")
    print("[smoke] InputId.Invalid() -> (" +
          str(inv.ClusterId()) + "," + str(inv.MemberId()) + ")")
    print("[smoke] fk == fk: " + str(fk == fk))
    print("[smoke] fk != inv: " + str(fk != inv))

    # --- Point ---
    p1 = Point(1, 2)
    p2 = Point(3, 4)
    p3 = p1 + p2
    print("[smoke] Point(1,2) + Point(3,4) = (" +
          str(p3.X()) + "," + str(p3.Y()) + ")")
    p4 = p2 - p1
    print("[smoke] Point(3,4) - Point(1,2) = (" +
          str(p4.X()) + "," + str(p4.Y()) + ")")
    print("[smoke] Point eq: " + str(p1 == p1) + ", ne: " + str(p1 != p2))

    # --- Color ---
    red = Color(0xFF0000)
    print("[smoke] Color(0xFF0000) R=" + str(red.R()) +
          " G=" + str(red.G()) + " B=" + str(red.B()))
    dim = red.Dim(0.5)
    print("[smoke] Dim(0.5) R=" + str(dim.R()) +
          " G=" + str(dim.G()) + " B=" + str(dim.B()))
    white = Color(255, 255, 255)
    print("[smoke] Color(255,255,255) WRGB=0x" + hex(white.WRGB()))

    # --- InputCluster (via live system data) ---
    clusters = MatrixOS.Input.GetClusters()
    print("[smoke] GetClusters() returned " + str(len(clusters)) + " cluster(s)")
    for c in clusters:
        print("[smoke]   id=" + str(c.ClusterId()) +
              " name=" + c.Name() +
              " class=" + str(c.InputClass()) +
              " count=" + str(c.InputCount()))

    primary = MatrixOS.Input.GetPrimaryGridCluster()
    if primary is None:
        print("[smoke] GetPrimaryGridCluster() -> None")
    else:
        print("[smoke] GetPrimaryGridCluster() -> id=" +
              str(primary.ClusterId()) +
              " dim=(" + str(primary.Dimension().X()) + "x" +
              str(primary.Dimension().Y()) + ")")

    # --- GetPosition / GetInputsAt round-trip ---
    fk_pos = MatrixOS.Input.GetPosition(fk)
    if fk_pos is None:
        print("[smoke] GetPosition(fk) -> None (no coords, OK)")
    else:
        print("[smoke] GetPosition(fk) -> (" +
              str(fk_pos.X()) + "," + str(fk_pos.Y()) + ")")
        ids = MatrixOS.Input.GetInputsAt(fk_pos)
        print("[smoke] GetInputsAt(fk_pos) -> " + str(len(ids)) + " id(s)")

    print("[smoke] value_wrappers done")

run()

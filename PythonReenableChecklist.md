# Python Re-enable Checklist

Operator-focused checklist for bringing Python back on Mystrix1 and Mystrix2.

---

## Prerequisites

- [x] ESP-IDF ldgen parser patched (see "ldgen fix" below)
- [x] `xtensa-esp-elf-gcc` is on PATH and matches the ESP-IDF version
- [x] IDF_PATH is set correctly (`C:\espressif\v5.3.1`)
- [x] ESP-IDF export script has been sourced (`export.ps1` / `export.sh`)
- [x] Mystrix1 build succeeds with Python enabled

## ldgen Fix (ESP-IDF v5.3.1)

ESP-IDF v5.3.1's `ldgen` fails parsing section flags that contain underscores
(e.g. `LINK_ONCE_DISCARD`).  This was fixed upstream in v5.3.4.

One-line patch applied to `C:\espressif\v5.3.1\tools\ldgen\ldgen\entity.py`
(line 142–143):

```diff
-        section_entry = (Suppress(Word(nums)) + Regex(r'\.\S+') + Suppress(rest_of_line)
-                         + Suppress(ZeroOrMore(Word(alphas) + Literal(',')) + Word(alphas)))
+        section_entry = (Suppress(Word(nums)) + Regex(r'\.\S+') + Suppress(rest_of_line)
+                         + Suppress(ZeroOrMore(Word(alphas + '_') + Literal(',')) + Word(alphas + '_')))
```

No pyparsing version change is required.  The default venv version (3.2.5) works.

## Step 1: Regenerate Binding Artifacts

```
cd Applications/Python/PikaPython
./rust-msc-latest-win10.exe
```

Verify no legacy `KeyPad`, `KeyEvent`, or `KeyInfo` symbols appear in
`pikascript-api/__pikaBinding.c`:

```
grep -i "KeyPad\|KeyEvent\|KeyInfo" pikascript-api/__pikaBinding.c
```

Only `KeypadInfo` (the new API) should match.  No `MatrixOS_KeyPad`,
`MatrixOS_KeyEvent`, or `MatrixOS_KeyInfo` modules should be present.

## Step 2: Re-enable Python for Mystrix1

Already done — `[System]Python` is active in `Devices/Mystrix1/ApplicationList.txt`.

## Step 3: Build Mystrix1

```powershell
# Set up environment
$env:IDF_PATH = "C:\espressif\v5.3.1"
. "$env:IDF_PATH\export.ps1"

# Configure and build (do NOT use idf.py build — it picks the wrong clang)
cmake -B build/Mystrix1 -Wno-dev . `
  -DCMAKE_TOOLCHAIN_FILE="$env:IDF_PATH/tools/cmake/toolchain-esp32s3.cmake" `
  -DFAMILY=Mystrix1 -DDEVICE=Mystrix1 -DMODE=DEVELOPMENT -GNinja

cmake --build build/Mystrix1
```

Expected: build completes with `libPython.a` linked.  ✅ Verified 2026-04-09.

## Step 4: Flash and Boot

```powershell
# Flash (adjust port as needed)
idf.py -p COM3 flash monitor
```

Or use the direct `esptool.py` flash command for your environment.

## Step 5: Run Smoke Tests

Execute in order.  Copy each script as the Python app entry point (or
import from `main.py`), then boot the device.

| Order | Script                                    | What to observe on serial                  |
|-------|-------------------------------------------|--------------------------------------------|
| 1     | `smoke_tests/smoke_value_wrappers.py`     | Printed InputId, Point, Color, cluster info |
| 2     | `smoke_tests/smoke_input_polling.py`      | `GetEvent` returns None on timeout, events on key press |
| 3     | `smoke_tests/smoke_ui_callback.py`        | UI opens, input events printed, clean exit on fn-key |

### What to look for

- **No hardfault / crash** on any smoke test
- `GetEvent(0)` returns `None` when no key is pressed
- `GetEvent(1000)` returns `InputEvent` when a key is pressed within 1 second
- UI callback prints event details with correct cluster/member IDs
- `ui.Close()` completes without crash
- No watchdog timeout

## Step 6: Re-enable Python for Mystrix2

Only after Mystrix1 smoke tests pass:

Edit `Devices/Mystrix2/ApplicationList.txt` with the same change (uncomment
`[System]Python`).

Build and flash Mystrix2, then repeat the smoke tests.

---

## If Build Still Fails

### Compiler errors in Python C++

If any Python C++ file fails to compile, the binding surface may have
drifted.  Re-run the Pika generator (Step 1) and check for `.pyi` /
C++ implementation mismatches.

---

## Remaining Known Blockers

| Blocker                         | Type          | Status     |
|---------------------------------|---------------|------------|
| ldgen `ParseException`          | Environment   | **Resolved** (entity.py patch) |
| Build verification (Mystrix1)   | Software      | **Passed** |
| Hardware smoke test validation  | Hardware      | Pending    |

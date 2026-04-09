# Python Re-enable Checklist

Operator-focused checklist for bringing Python back on Mystrix1 and Mystrix2.

---

## Prerequisites

- [ ] ESP-IDF toolchain is functional (no ldgen parse failures)
- [ ] `xtensa-esp-elf-gcc` is on PATH and matches the ESP-IDF version
- [ ] IDF_PATH is set correctly (e.g. `C:\espressif\v5.3.1`)
- [ ] ESP-IDF export script has been sourced (`export.ps1` / `export.sh`)

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

Edit `Devices/Mystrix1/ApplicationList.txt`:

```diff
-# [System]Python  # Disabled: build env blocker (ldgen parse failure, not code-related)
+[System]Python
```

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

Expected: build completes with `libPython.a` linked.

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

### ldgen parse failure

This is an ESP-IDF toolchain issue, not a Python code issue.

Workarounds:
1. Use a different ESP-IDF version (v5.3.1 worked for compilation, ldgen may
   need v5.3.2+ or a pyparsing downgrade)
2. Downgrade `pyparsing`: `pip install pyparsing==3.0.9`
3. Check ESP-IDF GitHub issues for ldgen-related fixes

### Compiler errors in Python C++

If any Python C++ file fails to compile, the binding surface may have
drifted.  Re-run the Pika generator (Step 1) and check for `.pyi` /
C++ implementation mismatches.

---

## Remaining Known Blockers

| Blocker                         | Type          | Status     |
|---------------------------------|---------------|------------|
| ldgen `ParseException`          | Environment   | Unresolved |
| Hardware smoke test validation  | Hardware      | Pending    |

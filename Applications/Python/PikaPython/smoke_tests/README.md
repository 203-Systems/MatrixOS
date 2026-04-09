# Python API Smoke Tests

These scripts are minimal bring-up tests for the new input-first Python API.

They are not unit tests — they are designed to be run manually on hardware
(e.g. Mystrix) after Python is re-enabled in the device application list.

## How to run

1. Copy a smoke script to the device as the Python app entry point, or
   `import` it from `main.py`.
2. Boot the device and enter the Python application.
3. Observe serial output / LED feedback for pass/fail indicators.

## Recommended execution order

| Order | Script                                  | Purpose                                         |
|-------|-----------------------------------------|-------------------------------------------------|
| 1     | `smoke_value_wrappers.py`               | Value types: InputId, Point, Color, cluster     |
| 2     | `smoke_input_polling.py`                | Input polling: GetEvent, GetState, FunctionKey  |
| 3     | `smoke_ui_callback.py`                  | UI lifecycle: create, SetInputHandler, close    |
| 4     | `smoke_ui_create_destroy_loop.py`       | Hardfault check: repeated UI create/destroy     |
| 5     | `smoke_ui_callback_reregister_loop.py`  | Hardfault check: repeated callback replacement  |

Scripts 1–3 verify basic API functionality.
Scripts 4–5 stress-test lifecycle paths that are most likely to expose
memory corruption, double-free, or callback context leaks.

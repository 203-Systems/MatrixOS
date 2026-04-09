# Python API Smoke Tests

These scripts are minimal bring-up tests for the new input-first Python API.

They are not unit tests — they are designed to be run manually on hardware
(e.g. Mystrix) after Python is re-enabled in the device application list.

## How to run

1. Copy a smoke script to the device as the Python app entry point, or
   `import` it from `main.py`.
2. Boot the device and enter the Python application.
3. Observe serial output / LED feedback for pass/fail indicators.

## Scripts

| Script                       | Purpose                                      |
|------------------------------|----------------------------------------------|
| `smoke_input_polling.py`     | Input polling: GetEvent, GetState, FunctionKey |
| `smoke_ui_callback.py`       | UI lifecycle: create, SetInputHandler, close |
| `smoke_value_wrappers.py`    | Value types: InputId, Point, Color, cluster  |

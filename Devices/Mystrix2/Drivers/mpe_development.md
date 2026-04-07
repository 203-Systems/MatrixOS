# MPE Development Notes

## Scope

This document tracks the recent iterations on Mystrix2 MPE touch detection and tracking in:

- `Devices/Mystrix2/Drivers/KeypadMPE.cpp`
- `Devices/Mystrix2/Drivers/CoprocessorLink/MPEDriver/MPECoprocessorLink.cpp`

The goal is to derive stable touch points from the MPE heatmap and eventually generalize the result into a reusable input pipeline.

## Hardware Model

- The user-facing surface is an `8x8` pad grid.
- Each pad contains a `3x3` sensing region in the MPE data.
- Movement across pads causes larger coordinate jumps than movement inside a pad.
- Real motion is expected to be primarily axis-aligned, not diagonal.

This means a generic continuous touch algorithm is not sufficient by itself; matching should be pad-aware and axis-biased.

## Iteration History

### 1. Initial Touch Point Extraction

Added touch point extraction inside `Device::KeyPad::MPE::Scan()`:

- Detect high-pressure connected regions from the MPE frame
- Compute weighted centroid for each region
- Print touch points for debugging

Observed issues:

- Single finger could split into multiple touch points
- No stable ID across frames
- Pressure and position were noisy

### 2. Added Tracking IDs

Implemented per-touch tracking:

- Stable `tracking_id`
- Matching between current detections and previous tracked points
- `UP` event logging when a point disappears

Observed issues:

- IDs frequently broke during sliding
- Short interruptions or blob splits created new IDs

### 3. Threshold / Hysteresis / Timeout

Added:

- `touch_on_threshold`
- `touch_off_threshold`
- hysteresis using `mpe_config.low_threshold`
- timeout / missing-frame tolerance

Reason:

- prevent immediate dropouts from small pressure dips
- avoid ending a point on a single bad frame

Observed issues:

- Touch stayed more stable, but detection still split one finger into multiple blobs

### 4. Smoothed Position / Pressure

Added smoothing to:

- touch position
- touch pressure

Reason:

- reduce jitter
- make logs easier to interpret

Observed issues:

- smoothing helped noise, but did not solve blob topology changes

### 5. Single-Line Logging + Measured Scan Rate

Logging was simplified to one line:

- format like `IDxx X-Y (P)`
- scan rate included in the prefix

Measured result:

- scan rate is around `250Hz`

Conclusion:

- the main instability is not caused by scan frequency being too low

### 6. Boxier Blob Detection

Detection was changed to be more region-oriented:

- larger merge radius
- blob bounding boxes
- blob merge based on neighboring boxes

Reason:

- a single finger should produce one broader region instead of several tiny peaks

Observed issues:

- reduced some fragmentation
- still not enough for reliable single-finger sliding across pads

### 7. Pad-Aware / Axis-Biased Tracking

Matching was changed from generic distance matching to pad-aware matching:

- same-pad matching first
- then only 4-neighbor pad transitions
- horizontal transitions prefer X movement and penalize Y drift
- vertical transitions prefer Y movement and penalize X drift
- diagonal jumps are not preferred

Reason:

- the sensing geometry is structured, not fully continuous
- slide behavior should respect pad adjacency

Observed issues:

- still seeing ID churn during long slides
- duplicate nearby IDs still appear during pad transitions

### 8. DRAM Reduction

To fit ESP32 DRAM:

- removed mirrored log-state array
- changed timeout bookkeeping to `uint32_t` milliseconds
- converted tracked point state from float-heavy storage to fixed-point / integer storage
- moved tracked touch point storage off `.bss` into heap allocation during init

Result:

- reduced `.dram0.bss` pressure enough to keep development moving

## Current Observations

Recent logs show:

- scan rate is stable at about `250Hz`
- the algorithm still occasionally splits one slide into multiple IDs
- duplicate IDs often appear near pad boundaries
- ID handoff still occurs during long axis-aligned movement

Example pattern:

- one ID remains stable inside a pad
- near pad boundary a second nearby ID appears
- original ID disappears
- movement continues with the new ID

This indicates:

- detection is still over-segmenting the pressure cloud
- tracking is still accepting multiple competing candidates during boundary crossing

## Current Force Path

This section summarizes the current force data path in the Mystrix2 MPE implementation.

### Raw Source

The coprocessor produces a `24x24` MPE force map:

- user surface: `8x8` pads
- per pad sensing region: `3x3`
- total grid: `24x24`

This data is maintained in:

- `Devices/Mystrix2/Drivers/CoprocessorLink/MPEDriver/MPECoprocessorLink.cpp`
- `Devices/Mystrix2/Drivers/CoprocessorLink/MPEDriver/MPECoprocessorLink.h`

Pipeline:

1. Coprocessor sends change entries for pad sectors
2. `ApplyChange()` writes the 8 edge readings into the `3x3` sector
3. `ComputeCenterValue()` estimates the missing center cell
4. `GetMPEData()` exposes the full `24x24` matrix

### Key / Pad Force Path

This path is used for keypad press behavior and `KeyInfo`.

Code path:

- `Devices/Mystrix2/Drivers/KeypadMPE.cpp`
- `OS/Framework/KeyEvent/KeyInfo.cpp`

Pipeline:

1. `Scan()` iterates over each `8x8` pad
2. For each pad, it reads the corresponding `3x3` MPE sector
3. It takes the maximum value from that `3x3`
4. `ScaleReading()` expands coprocessor `12-bit` data into `16-bit`
5. `SmoothEMA()` filters the per-pad force
6. Result is written into `pad_force[x][y]`
7. `keypad_state[x][y].Update(mpe_config, ...)` processes the force

Normalization behavior:

- `KeyInfo::ApplyForceCurve()` maps input according to `mpe_config`
- below `low_threshold` -> `0`
- above `high_threshold` -> `65535`
- between thresholds -> linear mapping

This means `KeyInfo::Force()` is already thresholded and normalized by `mpe_config`.

### Touch Force Path

This path is used for touch point detection and tracking.

Code path:

- `Devices/Mystrix2/Drivers/KeypadMPE.cpp`

Pipeline:

1. Start from the same `24x24` MPE matrix
2. `NormalizeTouchReading()`:
   - expands raw `12-bit` reading to `16-bit`
   - applies `mpe_config.low_threshold/high_threshold`
   - returns a normalized `0..65535` touch-force value
3. `DetectTouchPoints()` uses normalized values for:
   - seed thresholding
   - flood fill / blob growth
   - weighted centroid
   - peak pressure
4. `TrackTouchPoints()` uses detected candidates to:
   - assign stable IDs
   - apply smoothing
   - handle axis-biased pad transitions
   - suppress duplicates

This means touch pressure logs now also use normalized pressure, not raw expanded values.

### Difference Between Key Path and Touch Path

Key path:

- compresses each `3x3` sector to a single force value
- optimized for press / release / hold / aftertouch
- stable because it only looks at sector maximum

Touch path:

- operates on the entire `24x24` normalized grid
- optimized for multi-point region extraction and tracking
- more sensitive to blob splitting and boundary behavior

### Current Force-Related Concerns

1. Key path is relatively stable because it uses per-pad maximum force.
2. Touch path is much more sensitive to pressure cloud shape.
3. Sliding across pad boundaries still causes:
   - region splitting
   - duplicate candidates
   - ID churn
4. After moving touch normalization into `mpe_config` space, touch thresholds may need retuning because they are now interpreted on a normalized `0..65535` scale.

## Likely Remaining Problems

### 1. Blob segmentation is still too eager

Even with box-based merge, the pressure cloud can still be split when crossing pad boundaries.

### 2. Tracking does not yet model boundary crossing explicitly enough

We currently allow adjacent-pad matching, but we do not yet have a dedicated boundary-crossing state.

### 3. Detection and tracking are still too tightly coupled

Current detection output is directly consumed by tracking. A better pipeline may need:

- region candidate extraction
- candidate merge
- continuity scoring
- final track assignment

### 4. Pressure cloud shape across pad seams is probably asymmetric

The sensing response across seams likely changes enough that the weighted centroid alone is not a stable representation.

## Recommended Next Steps

### A. Add explicit seam / boundary transition handling

When a point is near a pad edge and the next candidate is in the adjacent pad, prefer continuity over spawning a second ID.

### B. Add stronger duplicate suppression

If two detected points are:

- in the same pad or adjacent pads
- very close in the non-primary axis
- similar in time / pressure

then prefer keeping one active track instead of two.

### C. Separate blob threshold from touch existence threshold

Use:

- one lower threshold for region growth
- one higher threshold for creating a new tracked touch

This should reduce blob fragmentation.

### D. Log more track-assignment debug info when needed

Optional debug lines could include:

- detected blob count
- chosen match score
- old pad -> new pad transition
- whether a new ID was spawned due to unmatched candidate

This should be added behind a debug flag to avoid clutter.

## Current Status

The current implementation is suitable for continued iteration and debug logging, but not yet stable enough to be considered final touch tracking behavior.

Main unresolved issue:

- single-finger sliding across pads can still split into multiple tracked IDs

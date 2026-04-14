# Review 66d5556 Follow-up

This note records the disposition of findings 2 through 6 from review-66d5556-detailed-findings.md, based on the current branch state and the fixes applied here.

## 2. workflow_dispatch inputs breaking GITHUB_OUTPUT / JSON metadata

Status: addressed.

What changed:

- The prepare job now writes every output through a multiline-safe helper that uses the documented delimiter form for GITHUB_OUTPUT.
- Release metadata JSON is no longer assembled with shell heredoc interpolation. The workflow now calls Tools/release/write-device-metadata.mjs so release_name, release_tag, and the rest of the metadata are serialized by a real JSON writer.
- release_tag is additionally validated with git check-ref-format before any asset packaging happens.

Result:

- Newlines in release_name no longer corrupt GITHUB_OUTPUT.
- Quotes and backslashes in metadata fields no longer produce invalid JSON.
- Invalid manual tag overrides now fail early with a clear workflow error.

## 3. publish-release race with development artifacts

Status: addressed.

Disposition:

- The fix here is to remove development artifacts from the publish path instead of making publish-release wait for them.
- build-firmware-dev and build-sil-dev now only run when publish is false.
- publish-release also explicitly downloads only release-* artifacts.

Why this is the right model:

- The current manifest/release schema is release-centric and does not model a second development variant for the same device/tag cleanly.
- Treating development builds as preview-only outputs avoids nondeterministic release contents and keeps the published asset set stable.

## 4. development artifacts missing manifest metadata

Status: addressed by design change.

Disposition:

- Development artifacts are no longer part of the publishing path, so they are intentionally excluded from Pages manifest generation.
- That removes the prior half-published state where GitHub Release assets and manifest contents could diverge.

Result:

- Published releases and generated manifest entries now describe the same release asset set.
- Development builds remain available only as non-publish preview artifacts.

## 5. MystrixSIL release packaging incomplete

Status: addressed.

What changed:

- The MystrixSIL release package now ships MatrixOSHost.js and MatrixOSHost.wasm together.
- The workflow also emits a self-contained zip bundle that preserves the canonical filenames inside the archive.
- The non-publish MystrixSIL preview artifact was aligned to the same JS + WASM + zip shape so dry-run downloads are usable too.
- Metadata for MystrixSIL now records loader, wasm, and bundle assets.

Why this closes the issue:

- The current MystrixSIL frontend boot path fetches MatrixOSHost.js first and then resolves MatrixOSHost.wasm through that loader.
- Shipping the loader plus the wasm, and providing a bundled archive, makes the published simulator artifact self-contained instead of relying on an implicit local pairing.

## 6. release_ver validation and transport consistency

Status: addressed.

What changed:

- workflow_dispatch.release_ver is now documented and validated as 1-31 for RC and Beta builds.
- Non-numeric and out-of-range values fail in the prepare job before any build starts.
- Release, nightly, and development channels still ignore release_ver and force it to 0 as before.

Why 1-31:

- This is the range that fits the existing protocol usage without silent clipping in the MIDI code path.
- Keeping the workflow-level contract inside that range avoids mismatched reporting between metadata, MIDI identity, and other transport surfaces.

## Net Result

Items 2, 3, 5, and 6 were real defects and are now fixed.

Item 4 was also real in the previous design, but the correct resolution was not to add more release metadata plumbing for development artifacts. Instead, the workflow now makes those artifacts preview-only and keeps the published release model internally consistent.
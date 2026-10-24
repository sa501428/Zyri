# Zyri implementation status

The old prototype has been replaced with a C++20 / Qt 6 editor. The application now connects input, transactions, history, rendering, saving, and recovery. Source is concentrated in five components under `src/editor`; the old scaffolding was removed after the replacement tests passed.

## Delivered behavior

| Area | Implementation |
| --- | --- |
| Document model | Value-owned layers/objects, persistent UUIDs, validated dimensions, sparse copy-on-write tiles |
| Editing | Session-owned begin/mutate/commit/cancel transactions; one completed gesture per history entry |
| History | Exact snapshots, saved-state identity, branching, count and resident-byte budgets, compressed local spill |
| Raster | Resampled pressure-aware brush, pencil, eraser, separate flow/stroke opacity, canvas/selection clipping |
| Selection | Rectangle/ellipse/lasso, add/subtract/intersect/invert, copy/cut/delete and selected-pixel transforms |
| Layers | Raster/vector, add/delete/duplicate/reorder, rename, lock, visibility, opacity, four blend modes |
| Illustration | Editable basic shapes, paths, text, object transforms/order/style, node/handle movement, append/delete nodes |
| Rendering | Headless tiled CPU renderer, bounded per-canvas composite cache, visible-tile culling, consistent view coordinates |
| Persistence | Real ZIP projects through libzip, explicit RGBA8 premultiplied tile bytes, structured geometry and text |
| File safety | Complete temporary archive, QSaveFile commit without direct-write fallback, validated archive sizes and content |
| Jobs | Snapshot-based save/export/fill/transform, at most two global workers, one background edit per session, stale-result rejection |
| Recovery | 30-second timer for modified committed states, two validated generations, process locks, explicit recovery/discard prompts |
| Interchange | PNG/JPEG color conversion and tagging, basic SVG scene import/export, PDF page raster import and PDF export |
| Desktop | Document tabs, panels, property controls, shortcuts, view controls, native dialogs, drag/drop, recent files, themes, configurable history/recovery and menu shortcuts |
| Build | Root test registration, real sanitizer options, dependency pinning, presets, CI matrix, install/deploy/CPack support |

Ordinary native save does not store transient selection or history. PNG/JPEG are flattened images. Native projects preserve editability. Complex SVG can fall back to a clearly disclosed raster import; arbitrary PDF is not an editable scene.

## Important implementation choices

A Session stores committed state identities independently of render revisions. Completing a save marks the exact saved snapshot, so edits made during the write remain dirty. Separate session IDs isolate two open copies of the same persistent document.

Raster images use Qt copy-on-write storage: snapshots share unchanged buffers and mutations detach changed images. Missing tiles remain transparent and are restored as missing by undo. The history budget accounts for unique image buffers and conservative scene-data costs. Spill restores are validated before changing the history cursor.

The viewport is currently software-based. This supplies a working compatibility path and headless reference implementation. The proposed QRhi/Metal experiment has not been adopted or benchmarked; GPU and 60 fps claims would be premature.

Qt handles RGB profile conversion into sRGB and tags output. Text has a single font/style per object, with explicit geometry. Exported normal-layer PDFs can retain paths and text; non-normal blends flatten the page for consistent composition. SVG text/layout support is limited; native files remain the editable master.

## Verification

The behavioral suite covers premultiplied blending, independent layers, tile sharing, cancellation, dirty-state branches, save-state races, continuous strokes, opacity, locks/bounds, selection clipping, tolerance fill, structural undo, history spill, transforms/resizing, native round-trips, corrupt files, format rejection, PNG/JPEG/PDF/SVG output, isolated document tabs, synthetic mouse input, recovery generation rotation, asynchronous saving, and recovery writes.

A synthetic fixture mode generates an editable project and image/vector/PDF exports without opening a window. Desktop validation uses Qt's offscreen platform. No personal screen, screenshot capture, or accessibility interface is involved.

The normal and UBSan builds pass the behavioral suite. AddressSanitizer binaries build but their runtime stalls before main on this host, also reproduced with a minimal standalone probe; no ASan runtime pass is claimed. See the implementation delivery message for final counts and package checks. CI configuration is included but its remote execution is not claimed.

## Remaining release gates and limitations

- Validate the native desktop layout, tablet hardware, platform gestures, and clean-machine behavior through isolated/user-run checks.
- Measure the proposed startup, latency, and memory scorecard on named reference hardware. No performance threshold is declared passed.
- History compaction/spill currently runs during commit when memory pressure requires it. Raster move previews, crop, and clipboard rendering can still perform substantial UI-thread work.
- The implementation has tested upper bounds and budget checks, but it does not implement general document tile paging.
- Node insertion appends to a path rather than splitting an arbitrary Bézier segment. Smooth/corner node constraints and advanced path operations are deferred.
- Text style runs, advanced typography, groups, masks, effects, boolean geometry, wide-gamut/16-bit editing, and arbitrary SVG/PDF preservation remain outside this build.
- Recovery generation rotation and save failure paths have automated tests; abrupt power loss, full-filesystem injection, and every platform filesystem are not exhaustively simulated.
- Windows/Linux jobs, signing, notarization, installer behavior, and older OS compatibility need actual release validation.
- The project license and asset provenance still need an owner decision before public distribution.

These limits distinguish the implemented editor from a release-certified product. The original roadmap remains available for subsequent performance and release work.

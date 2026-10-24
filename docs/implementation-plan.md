# Zyri implementation plan

Implementation update: see [implementation status](implementation-status.md) for delivered behavior and departures from this design. The original planning text below is retained as context.

Status: proposed development plan, October 22, 2026. This pass changes documentation only. It does not implement the plan or delete application source.

## Recommendation

Build a focused native hybrid graphics editor: fast raster painting and image editing, with editable vector shapes, paths, and basic text in the same document. Retain C++20 and Qt. Replace the incomplete connections between input, document storage, rendering, history, and persistence before expanding the feature set.

The first meaningful deliverable is a complete workflow: create a document, draw a continuous stroke, undo/redo it, save an editable project, close it, and reopen exactly the same pixels. A layer panel with working actions is more valuable than twenty unconnected tool classes.

The [code audit](code-audit.md) establishes the baseline. The [architecture](architecture.md), [tile design](tile-system.md), and [history design](undo-design.md) specify the replacement contracts.

## Working assumptions

These defaults make implementation actionable without requiring a product questionnaire:

- macOS is the first release platform; maintain portable core code and add Windows/Linux build checks early.
- The product is entirely local and offline-capable.
- The primary workflows are painting, modest image editing, and lightweight illustration, rather than professional print production or a complete Illustrator replacement.
- v1 uses finite canvases, 8-bit sRGB pixels, ordinary layer blending, and editable vector objects.
- One developer can deliver milestones sequentially. No parallel rewrite or large team is assumed.
- The first macOS release uses direct distribution. Sandboxing and an app-store route are separate decisions.
- Scope expands only after the previous workflow is correct, measurable, and recoverable.

If the intended product becomes primarily a vector design application, reconsider the raster-grid preview and prioritize path editing, groups, alignment, and SVG earlier. That would be a product change, not a reason to discard the proposed transaction and persistence foundations.

## Release scope

| Capability | First usable raster build | Public hybrid v1 | Later |
| --- | --- | --- | --- |
| Documents | New, open native, save/save as, close protection | Tabs, recent files, import/drop, recovery | Multiple windows, artboards |
| Painting | Round brush, hard pencil, eraser, mouse input | Pressure, size/hardness/spacing/flow, useful presets | Texture brushes, smudge, wet paint |
| Layers | Raster add/delete/reorder/visibility/opacity | Raster and vector, rename/duplicate/lock, four blend modes | Groups, masks, adjustments, effects |
| Selections | None required for first workflow | Rectangle/ellipse/lasso, add/subtract/intersect, move/copy/cut/delete | Magic wand, sophisticated feathering/refinement |
| Transforms | Pan and zoom view | Move/scale/rotate/flip content, crop, image/canvas resize | Perspective, warp, non-destructive raster transforms |
| Vector | Data model reserved; no pretend tools | Shapes, cubic paths, fill/stroke, object transforms, basic node editing | Boolean operations, advanced strokes, complex groups |
| Text | Not required | Editable basic text objects, font/style/size/alignment | Text on paths, advanced typography |
| Color | RGBA/hex, sRGB | RGB/HSV, swatches, tested import/export profiles | Wide-gamut documents, linear/16-bit/HDR, CMYK |
| Interchange | PNG import/export after native save works | JPEG, bounded SVG subset, raster PDF import, PDF export | PSD/AI fidelity, editable arbitrary PDF |
| Reliability | Atomic project save, exact undo | Recovery, resource limits, history spill, failure reporting | General document tile paging |

PNG/JPEG exports are flattened interchange images; the editable master is the native project. PDF import is rasterization of selected pages at an explicit size/resolution. SVG support must identify its supported subset and any rasterization. Neither is a promise of arbitrary source-document preservation.

## Architectural decisions to settle early

| Decision | Default | Evidence that would change it |
| --- | --- | --- |
| Desktop UI | Qt Widgets, native actions/dialogs, restrained custom styling | A proven interaction requirement that Widgets cannot reasonably deliver |
| Authoritative storage | CPU sparse immutable tiles plus vector records | Large-document workload proving mandatory paging or another representation |
| First renderer | CPU reference, QPainter presentation | Profiling identifies a bottleneck after correctness passes |
| Accelerated viewport | Isolated QRhiWidget experiment, Metal on macOS | Backend correctness, packaging, or maintenance cost fails the spike |
| Working color | Explicit 8-bit encoded sRGB, premultiplied alpha | Product commits to higher precision or wide-gamut editing |
| Native file | Versioned ZIP, canonical tile bytes and structured objects | Measured save/recovery limitations that justify another container |
| History | Atomic transactions, immutable references, measured compression/spill | Workload data favors a different encoding; semantics remain unchanged |
| Vector/text data | Model-owned scene objects, explicit serialization | More advanced document semantics are actually required |

Do not choose a new technology just because the old prototype uses it badly. Do not preserve an implementation merely because its class name matches the roadmap.

## Milestone 0: Reproducible build and honest baseline

**Purpose:** make subsequent evidence trustworthy.

Work:

- Enable CTest at the project root and gate tests with BUILD_TESTING.
- Attach compiler options to real targets; add explicit sanitizer options that work for single- and multi-configuration builds.
- Enable Objective-C++ only on Apple platforms.
- Pin the initial supported Qt/toolchain configuration and enforce the minimum version.
- Remove unnecessary mandatory PDF/Poppler dependencies from the minimal editor path. Retain current dependencies until their replacement is implemented.
- Add CMake configure/build/test presets and CI for macOS. Add Windows/Linux core compilation early; platform release support remains gated separately.
- Separate headless component tests from optional GPU/platform integration tests.
- Verify resources survive static linking and that shader/resource initialization errors are reported.
- Record the current seven tests as a baseline, then add targeted regression cases for the concrete audit defects.

**Acceptance:** a clean checkout configures, builds, and discovers nonzero tests from the documented root command. Sanitizer flags are visible in actual compile/link commands. A missing optional backend does not prevent core tests from building.

**Cleanup:** remove duplicate find/version scaffolding only after one dependency/version source replaces it. Empty placeholder targets can leave the build; no need to preserve an archive with no functionality.

## Milestone 1: Document model, tiles, transactions, and CPU correctness

**Depends on:** milestone 0.

Work:

- Define DocumentSession ownership, persistent IDs, content-state IDs, mutation revisions, and validated dimensions.
- Move vector data below the engines; consolidate text into vector objects.
- Introduce immutable tile reads and transaction-scoped writable access.
- Implement structured change sets and exact commit/cancel behavior.
- Replace the current CPU blend implementation with reference-tested premultiplied compositing.
- Implement a headless raster compositor for Normal, followed by the other three modes.
- Add a software viewport with a tested document/view/framebuffer coordinate boundary.
- Introduce byte accounting for tiles, transactions, and history before optimizing compression.
- Connect layer changes and history replay to the same invalidation path.
- Implement saved-state semantics, including undo to saved and branch-after-undo.

**Acceptance:** two layers with different pixels at the same tile coordinate render correctly. Half-transparent color cases match reference values. Mutations emit coherent changes; cancellation restores the original state. Undo/redo are byte-exact. A session can be created, edited, rendered, and destroyed without a window or GPU.

**Decision experiment:** build a small QRhiWidget adapter after the software path exists. Test known tile uploads, alpha, target resizing, resource recreation, and output parity using isolated render fixtures. Record whether to adopt it in milestone 4. This experiment must not block the CPU editing workflow.

**Cleanup:** replace current controller/history ownership, invalid blend math, and core-to-engine includes. Keep old BrushStamp code only if it remains useful as a reference.

## Milestone 2: First complete painting and saving workflow

**Depends on:** milestone 1.

Work:

- Create a real main window and one document session with working New, Save, Open, Undo, Redo, and close protection.
- Add a minimal layer list, active-layer selection, foreground color, and brush controls.
- Normalize mouse input and synthetic tablet samples; implement capture, finish, cancel, and focus-loss policies.
- Implement continuous distance-resampled strokes, including fast movement across multiple tiles.
- Share coverage/selection/alpha logic across brush, pencil, and eraser.
- Make a stroke one transaction; lock and canvas-bound checks happen before mutation.
- Implement native-format version 1 for raster documents, with stable IDs and explicit pixel encoding.
- Add snapshot-based saving, atomic destination replacement, validation on load, and clear errors.
- Add PNG import/export through the CPU renderer; choose the JPEG matte policy before adding JPEG.

**Acceptance demonstration:** create a transparent 2048×2048 document, paint across several tile boundaries on two layers, erase, undo/redo, save, reopen, and compare every pixel, layer property, and ID. Cancelled strokes make no change. Closing modified work offers Save/Discard/Cancel; a failed save keeps the document open and dirty.

This is the first usable build. Do not postpone it until vector tools, all panels, or polished packaging exist.

## Milestone 3: Recovery, background jobs, and storage budgets

**Depends on:** milestone 2. Complete before anyone relies on the app for work they cannot afford to lose.

Work:

- Add bounded worker queues, snapshot-based export/save, cancellation, and stale-result checks.
- Serialize writes to the same destination; save completion marks only the saved content state clean.
- Autosave committed states after a configurable interval, initially 30 seconds while modified. Coalesce overlapping requests and avoid duplicate saves of unchanged content.
- Keep a previous valid recovery generation while writing the next.
- Add recovery registry, app-instance ownership, recovery dialog, and cleanup policy.
- Introduce history compression based on measured workloads, then bounded local spill.
- Implement editable-document and operation allocation limits with a clear response when exceeded.
- Add disk-full, denied-write, corrupt-project, interrupted-save, and failed-recovery tests.

**Acceptance:** killing an isolated test process during save leaves either the old valid project or the new completed project, never an accepted partial archive. Recovery restores the last completed generation into a separate unsaved document. Editing during save does not incorrectly clear the dirty flag. Closing one document safely cancels its jobs.

Recovery interval is a target for healthy conditions, not a promise during disk failure or a permanently saturated worker queue.

## Milestone 4: Responsive viewport and complete raster editing

**Depends on:** milestones 2 and 3; uses milestone 1's renderer decision.

Work:

- Add visible-tile culling, composite caching, bounded GPU cache, and filtered zoom-out support.
- Adopt the accelerated adapter if it passes the spike; upload CPU composites first.
- Verify resource lifetime, monitor-scale transitions, rotation, pointer anchoring, and software fallback.
- Add rectangle/ellipse/lasso selections and explicit deselected-versus-empty-mask semantics.
- Implement contiguous fill with defined tolerance and sample-current/sample-visible modes. Use scanline/span traversal and bounded tile work instead of a document-sized visited array where practical.
- Add eyedropper, clipboard paste as a new layer, and consistent foreground/background colors.
- Implement move/scale/rotate/flip as preview-then-commit transactions.
- Implement crop, canvas resize, and image resize with explicit anchor/resampling choices.
- Offer transparent pixel lock only if it can be integrated and tested within the shared coverage pipeline.

**Acceptance:** selections constrain painting, erasing, fill, and transforms consistently. A transform preview does not repeatedly resample the authoritative image. Escape restores the original. Moving between tiles and zoom levels produces no seams or wrong-layer reuse. Large cancellable operations do not lock the event loop.

Full-image resampling is destructive in v1 and undoable. A move operation on selected raster pixels uses a floating preview until committed, rather than deleting source pixels before the operation can succeed.

## Milestone 5: Editable vector illustration and basic text

**Depends on:** milestones 1–4; keeps the same transaction/history/persistence system.

Work:

- Add a vector layer type to the UI and native-format schema.
- Implement rectangle, rounded rectangle, ellipse, line, and polygon objects with fill, stroke, width, opacity, and affine transform.
- Add object hit testing with a tolerance expressed in view units, selection outlines, handles, and front/back ordering.
- Define local-object versus document transforms and stroke-scaling behavior.
- Add pen paths with move/line/cubic/close commands and minimal node editing: move node, adjust handles, add/delete node, open/close path.
- Add basic text objects with editable content, font descriptors, size, alignment, and simple style runs.
- Persist explicit geometry/text data. Record missing fonts and provide a visible substitution result.
- Extend CPU rendering and native round-trip fixtures for mixed documents.

**Interaction rules:** painting on a vector layer offers creation/selection of a raster layer; it never silently destroys objects. Rasterizing a vector layer is an explicit undoable command. Selection of scene objects is distinct from a raster pixel mask. A sample-visible fill samples the rendered scene but writes only to the selected raster layer.

**Acceptance:** create a painted background, an editable shape, a cubic path, and text; move and restyle each; undo; save and reopen with editability and ordering preserved. Rasterize one vector layer, then undo back to editable objects.

Object groups, boolean geometry, text on paths, and advanced typography remain deferred.

## Milestone 6: Interchange and color fidelity

**Depends on:** milestone 5 and the stable CPU renderer.

Work:

- Finish JPEG import/export, orientation handling, alpha matting, and quality controls.
- Verify source-profile conversion and sRGB metadata in PNG/JPEG. Handle invalid and unsupported inputs clearly.
- Implement SVG export for the supported object subset, preserving layer/object order and embedding raster content when necessary.
- Implement a bounded SVG import subset: basic shapes, paths, affine transforms, simple fills/strokes. Unsupported features must be identified; offer explicit raster import where available.
- Disable external resource/network loading in importers. Preserve local-only behavior.
- Add raster PDF import with page selection and explicit raster size/resolution.
- Export PDF at a physical size derived from document pixels and PPI. Use Qt's QPainter/PDF path where suitable; do not introduce another graphics engine solely for basic PDF output.
- Preserve representable vector content on PDF export; flatten affected composites or the whole page when required to retain appearance, and state that choice in export options.
- Add size-scaled export and deterministic metadata where practical.

Qt's [QPdfWriter](https://doc.qt.io/qt-6/qpdfwriter.html) accepts QPainter drawing commands. That supplies an implementation path, not proof that every blend/text feature round-trips. Test exported results against the CPU reference and document fidelity limits.

**Acceptance:** PNG transparent edges, JPEG matte/orientation, SVG supported objects, and PDF physical size have fixtures. Unsupported SVG/PDF content never silently becomes a corrupted editable scene. Native save/load remains the authoritative editability test.

## Milestone 7: Desktop workflow and product polish

**Depends on:** the usable workflows above; minimal UI exists since milestone 2.

Work:

- Add multi-document tabs and consistent active-session action routing.
- Complete layers, colors/swatches, contextual properties, and tool options.
- Centralize shortcuts, action enablement, tool cursors, and undo/redo labels.
- Provide native menus/dialogs, keyboard navigation, standard control labels, dark/light themes, and sensible resettable dock layouts.
- Implement fit/100%/actual-pixel controls, canvas rotation reset, and clear canvas-size/zoom status.
- Make Save versus Export unambiguous. Keep unsupported features out of toolbars.
- Add recent documents, drag/drop, configurable shortcuts, and preferences that do not change document contents.
- Validate tablet pressure and input-device behavior on real hardware as a separate release-validation activity.

**Acceptance:** two documents can be edited, saved, closed, and recovered independently; every visible action has defined behavior and appropriate enabled state. Dialog cancellation never mutates content. UI styling does not break native focus, shortcuts, or control readability.

This planning/review task uses code only. Future device and visual validation should occur in an isolated test environment or through user-run checks, without assuming permission to inspect a personal screen.

## Milestone 8: Release hardening and distribution

**Depends on:** all public-v1 feature gates.

Work:

- Freeze native schema v1, add compatibility fixtures, and document migration rules.
- Add malformed-input fuzzing and randomized edit/undo/redo sequences.
- Validate clean installation, bundled dependencies, file associations, and launch with no development paths.
- Set compiler and bundle deployment targets consistently.
- Add install rules, reproducible package generation, code signing, and notarization.
- Select the project license and review shipped dependency/asset notices.
- Validate startup, memory, responsiveness, and recovery against the reference workload.
- Ship macOS first. Promote Windows/Linux from build targets to supported releases only after equivalent rendering, file, input, and packaging checks.

**Acceptance:** a packaged app on a clean supported machine can complete the full mixed-document workflow, survive the failure suite, and open projects produced on another supported platform. Release notes list actual limitations.

## Performance and correctness scorecard

These are proposed gates, not measurements of the current code. Establish a named reference Mac, RAM size, OS, Qt version, viewport resolution, and release build before comparing results.

| Area | Reference workload | Initial target or invariant |
| --- | --- | --- |
| Startup | Empty session, packaged release, warm and cold runs separately | Warm p95 under 2 seconds; report cold result independently |
| Input dispatch | Recorded pointer trace during ordinary editing | p95 handler time under 4 ms; no compression or blocking I/O inside handlers |
| Frame delivery | 4096×4096, 10 sparse raster layers, 200 shapes, 1600×1000 logical viewport at DPR 2 | Aim for p95 under 16.7 ms after acceleration; report CPU fallback separately |
| Stroke presentation | 32/128-pixel brushes at recorded sample rates | Target p95 event-to-frame submission below 25 ms; this is not a measured input-to-photon guarantee |
| Stroke correctness | Same path resampled into dense/sparse event batches | Stable spacing/pressure behavior, no missing segments |
| History | 100 ordinary strokes, structural edits, dense fills | Byte-exact replay, budgets enforced; no invented compression guarantee |
| Stress memory | 8192×8192 and dense multi-layer content | Predict/reject excessive allocation; no unbounded cache or worker queue |
| Export | Mixed raster/vector fixtures | CPU/GPU parity where used; lossless native raster round-trip |
| Data safety | Full disk, interrupted write, corrupt archive, concurrent save/edit | No silent data loss or false clean state |

Use generated fixtures and recorded event traces so profiling does not depend on manual painting. Capture wall times, p50/p95/p99, peak resident memory, cache bytes, allocations, dirty tile counts, and worker backlog. A slow benchmark should identify which stage costs time before prompting a rewrite.

## Test strategy

The seven existing tests remain useful but do not establish product correctness. Grow coverage around externally meaningful behavior:

- Model/property tests: valid dimensions, identity preservation, bounds, tile absence, immutable snapshots.
- Pixel fixtures: premultiplied alpha, each blend mode against transparent and opaque backdrops, erasing, color conversion, tile borders.
- Editing sequences: begin/update/commit/cancel, delete/restore layers, saved-state branching, selection constraints.
- Persistence: exact native round-trip, schema migration, interrupted commit, malformed/oversized entries, stable error reporting.
- Concurrency: stale fill results, close during save, two documents, old save finishing after a newer edit.
- Renderer tests: cross-layer cache identity, DPR transforms, culling, GPU versus CPU fixtures, resource recreation.
- Product integration: scripted document workflows and later isolated hardware/visual checks.

Do not expand testing by writing tests that merely restate trivial implementation details. Every regression fixture should correspond to a contract or an observed defect.

## First implementation backlog

The next code session should execute these small changes in order:

1. Repair root test discovery and compiler/sanitizer target wiring.
2. Add failing regression cases for premultiplied blending, layer-qualified tile identity, dirty-state propagation, and view-transform round trips.
3. Introduce the minimal DocumentSession and validated immutable tile access.
4. Implement transaction commit/cancel and saved-state identity; wire structural layer operations through it.
5. Implement CPU Normal composition and a software viewport.
6. Connect one continuous brush stroke to active-layer editing, invalidation, and undo.
7. Implement raster native save/load and the end-to-end round-trip fixture.
8. Add crash recovery and resource accounting before expanding the tool palette.

This backlog deliberately leaves many old source files untouched until their replacement is available. Each change should be independently reviewable; compile and run the relevant behavioral tests before moving on.

## What to remove and when

No application source is removed in this documentation pass. Git history already preserves the prototype, so there is no need for a second legacy source tree.

During implementation:

- Remove empty tool/panel/handler classes when a real feature or a smaller registry replaces them.
- Delete the old controller, history, and tile-cache paths once their replacement tests pass.
- Retire the OpenGL renderer once the selected viewport path meets its gates; do not maintain two unfinished GPU compositors.
- Remove unused PDF/Poppler and platform abstractions until a milestone needs them.
- Keep one dependency-discovery route and one version source.
- Do not remove artwork merely for tidiness; first establish its intended use and provenance.

## Remaining decisions and delivery risk

Only a few choices require evidence before they can be finalized: QRhi maintenance cost versus its viewport benefit, Qt color-path behavior for the supported inputs, practical document-size limits, and archive save throughput. Each has a milestone and a fallback; none requires designing the entire editor in advance.

The largest risks are incorrect alpha/color semantics, asynchronous state races, undo memory growth, and vector/raster fidelity mismatches. Resolve them with fixtures and complete workflows, not additional abstraction layers.

A public hybrid editor is a multi-month engineering project. Calendar estimates before the transaction and rendering experiments would be unreliable. Estimate milestone 2 after milestone 1 is measured, and estimate public v1 after the first mixed raster/vector round-trip. Maintain a usable build at every completed milestone.

# Zyri code audit

Review date: October 22, 2026. Baseline commit: `5caa082`. This audit covers repository files, a local build, and headless component tests. No editor window, screen capture, or accessibility interface was used.

The audited source has since been replaced. Paths below refer to baseline commit `5caa082`; see [implementation status](implementation-status.md) for the replacement.

## Assessment

Zyri is a useful graphics-programming prototype, but its module names and old documentation substantially overstate the implemented application. The main problem is not C++ or Qt. It is the absence of an integrated editing lifecycle: input, document mutation, history, invalidation, saving, and recovery do not operate as one system.

Keep the product direction and selected low-level ideas. Rebuild the foundations in small, executable milestones rather than implementing every placeholder class. Preserve the current source as reference until replacements pass their behavioral checks.

## What exists

There are 89 `.cpp` files under `src`; 55 contain only one or two lines. This measures scaffold density, not product completion.

| Area | Observed implementation | Assessment |
| --- | --- | --- |
| Application | `main.cpp` creates one document, a raster layer, canvas, menus, cursor coordinates | Demonstration shell; most actions have no handlers |
| Core | Document properties, layer order, UUIDs, sparse tiles, vector/text layer types | Useful concepts; mutation and notification contracts need redesign |
| Raster | Radial brush stamps, pressure scaling, full-image flood fill, mask, simple transforms | Isolated algorithms; no complete interactive paint engine |
| Vector | Paths with fill/stroke, ordered object list, tile rasterizer | Minimal representation; no working editing tools or interchange |
| Canvas | OpenGL checkerboard, raster drawing, pan/zoom, border, texture cache | Prototype with cache identity and coordinate errors |
| History | Command cursor, structural/property commands, XOR compression, paint command | Incomplete integration and unbounded bytes per command |
| Tools | Pointer interface, registry, partial brush/eraser | Engine pointer remains null; most tools are empty |
| UI and application services | Named windows, panels, dialogs, application/session classes | Mostly empty shells |
| Persistence | Named PNG/JPEG/PDF/native-file handlers | Empty classes; no native format, export, or recovery |
| Platform | macOS bookmark/appearance wrappers, bundle metadata, icons | Standalone helpers; no integrated release workflow |
| Tests | Seven component test executables | Useful seeds, insufficient integration and edge-case coverage |

## Verified build and tests

Configured a Debug build outside the repository at `/private/tmp/zyri-audit-20261022` using AppleClang 17.0.0, Qt 6.10.2, lcms2 2.19, and zlib 1.2.12. The complete build succeeded.

Root-level CTest discovery reported **zero tests**. Running CTest in the build's `tests` subdirectory with `QT_QPA_PLATFORM=offscreen` ran all seven executables successfully. No source changes were needed. `TestPaintEngine` tests BrushStamp output, not the full PaintEngine stroke lifecycle.

The Debug compile commands contain neither the sanitizer flags nor the warning flags defined in `ZyriCompilerOptions`. Passing tests are **not** evidence of an ASan/UBSan-clean build. Warnings include empty placeholder archives, duplicate library links, and deprecated image mirroring calls.

## Findings that determine the redesign

### Texture cache identity is incorrect

`src/canvas/GLCompositor.cpp` asks a shared cache for textures using only `(tx, ty)`. `src/canvas/TileCache.h` has no document ID, layer ID, or revision in its key.

After one layer uploads tile `(0, 0)`, another layer at that coordinate can reuse its pixels. `CanvasViewport::setDocument` does not clear or namespace the cache either. Correct rendering needs stable identity and revision contracts.

### Painting is disconnected

`src/canvas/CanvasViewport.cpp` consumes tablet events without tool dispatch. Mouse handlers implement panning but not painting. `src/tools/painting/BrushTool.h` initializes `m_engine` to null, with no assignment elsewhere. The brush chooses layer zero rather than an active layer.

`src/engine/raster/PaintEngine.cpp` stamps once per input point without using accumulated spacing to interpolate. At stroke end it computes compressed deltas and discards them. It publishes neither history commands nor dirty tile notifications. This path has no cancellation, selection application, canvas clipping, or locked-layer enforcement.

The next milestone must connect a complete edit rather than add more tool classes.

### Mutations bypass document state tracking

`src/core/DocumentController.h` describes a single mutation entry point, but its implementation only manages foreground/background colors. Public setters and layer/tile mutators bypass it. Adding a layer, changing opacity, or applying a paint command does not automatically mark the document dirty or notify the viewport.

A transaction boundary must update history, revisions, dirty state, and render invalidation together. Tool color preferences belong outside persistent document state.

### CPU blending violates its pixel contract

`src/core/BlendMode.h` declares premultiplied inputs. `src/core/BlendMode.cpp` weights already-premultiplied source channels by alpha again and divides by output alpha. Half-transparent premultiplied red `0x80800000` over opaque blue `0xFF0000FF` yields `0xFF40007F`; correct premultiplied source-over yields `0xFF80007F`.

Non-normal modes also mishandle transparent backdrops: opaque red in Multiply mode over transparent black becomes black instead of red. Tests mostly use opaque colors and identity cases. The GPU implements only Normal and ignores the layer blend-mode setting.

Replace the compositing contract and golden tests before exposing more modes.

### Coordinates mix widget units and display pixels

`src/canvas/ViewTransform.cpp` multiplies positions by zoom and DPR. The compositor normalizes these against logical widget dimensions, while event positions also use logical coordinates. The wheel anchor correction has inconsistent sign/scale handling as well.

Document pixels, widget coordinates, and framebuffer pixels must be distinct. Qt's [High DPI documentation](https://doc.qt.io/qt-6/highdpi.html) specifies device-independent widget and event geometry. The target design applies DPR only at the framebuffer boundary and defines zoom separately.

### History compression does not bound memory

`src/history/commands/PaintTileCommand.h` retains an uncompressed 256 KiB before-buffer for every touched tile, in addition to its compressed delta. A fully allocated 4096×4096 RGBA8 layer uses 64 MiB. One hundred whole-layer edits therefore retain at least 6.25 GiB of before-images alone.

`src/history/UndoStack.cpp` limits entry count, not bytes, has no saved-state marker, and does not validate nonpositive capacity. The previous approximately 50 MB worst-case estimate was unsupported.

`src/history/DeltaCompressor.cpp` silently treats decompression failure as unchanged content and does not require the inflated length to match the expected length before reading it. Undo must detect errors and preserve document/cursor consistency.

### Tile access copies too much and loses sparse state

`RasterLayer::tileAt` deep-copies existing tiles on every read and allocates full blank images for missing ones. Painting repeats this per stamp and searches snapshot/dirty vectors linearly. Undo restores a blank allocation instead of restoring absence.

Use immutable tile references for reads/snapshots and one writable copy per touched tile per transaction. Preserve missing-versus-present state in history and serialization.

### Dependency direction is not enforced

`core/VectorLayer.cpp` constructs engine-owned vector types while Engine depends on Core. `engine/raster/PaintEngine.cpp` calls history compression without an engine-to-history target dependency. All tests link Core, Engine, and History together, masking isolation problems.

Move serializable vector data into the document model. Engines should return edits or tile results; application editing code should manage history. Verify independent library targets.

### Color management is incomplete

`src/core/ColorProfile.cpp` is not integrated with import, display, or export. Its sRGB factory leaves the ICC byte array empty. The scanline API advertises premultiplied input while the lcms transform uses plain BGRA, without an explicit unpremultiply/repremultiply boundary or alpha-preservation policy.

Define working-space and alpha semantics first. The proposal starts with sRGB documents and validates Qt color transforms before deciding whether lcms2 is required for v1.

### Build and packaging claims exceed the configuration

- `enable_testing()` exists only under `tests`; root CTest misses them.
- `ZyriCompilerOptions` is never attached to targets.
- Objective-C++ is enabled on every platform instead of Apple only.
- The claimed Qt minimum is not enforced in `find_package`.
- Tests and PDF dependencies are required for all builds.
- The minimum in `Info.plist` does not set the compiler deployment target.
- CPack has metadata but no install rules or dependency deployment/signing pipeline.
- ZIP is described but has no implementation or archive dependency; zlib alone does not implement ZIP containers.

A macOS development build is a starting point, not proof of cross-platform or distributable readiness.

## Documentation claims to retire

The old docs described atomic saves, autosave, a complete UI tree, shader blend modes, and a history memory bound as if implemented. They also equated Retina scaling with larger stored document images, called 256-pixel tiles a universal industry standard, and assumed `QOpenGLWidget::update(rect)` automatically restricts custom GL drawing.

Tile size is a benchmark choice. Dirty data and drawing invalidation are separate. Qt documents [QOpenGLWidget update behavior](https://doc.qt.io/qt-6/qopenglwidget.html), but application code must still implement its own culling/render policy. The current canvas clears the framebuffer and iterates all allocated raster tiles on each paint.

## Reuse decisions

| Keep or adapt | Replace | Defer or remove from the future build |
| --- | --- | --- |
| C++20, Qt Widgets, CMake | Mutation/controller and saved-state design | Empty future-feature classes |
| UUID layer/object identity | GPU cache and coordinate conversion | Poppler until justified |
| Sparse tiles and transparent absence | Per-read deep-copy tile API | Sandbox helpers until needed for distribution |
| BrushStamp as a correctness reference | Stroke sampling, commit, cancellation | Advanced text, groups, effects, huge-document paging |
| Basic path/style representation | Alpha compositing and color boundaries | Duplicate find modules and version constants |
| Useful tests as regression seeds | History ownership and memory policy | Unverified packaging claims |

Retain branding provisionally and verify asset provenance before distribution. No source cleanup is required in this planning pass. Remove replaced files in the milestone that supplies their tested replacement.

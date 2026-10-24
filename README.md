# Zyri

Zyri is a local desktop editor for painting, image editing, and lightweight vector illustration. It is built with C++20 and Qt 6 Widgets.

![Zyri logo](logo.png)

The original prototype has been replaced with a working document engine and desktop editor. Raster pixels use sparse tiles, vector shapes and text remain editable, and all persistent edits share transaction and undo handling.

## Implemented

- Document tabs, New/Open/Save/Save As, recent files, drag and drop, and unsaved-change protection.
- Continuous brush, pencil, eraser, pressure input, hardness, spacing, flow, and stroke opacity.
- Rectangle, ellipse, and lasso selections, with add/subtract/intersect; clipboard images, fill, and eyedropper.
- Raster and vector layers with visibility, lock, opacity, ordering, duplication, and Normal/Multiply/Screen/Overlay blending.
- Editable rectangles, rounded rectangles, ellipses, lines, polygons, cubic paths, and basic text.
- Object move/scale, node/handle movement, append/delete nodes, fill/stroke, font controls, and object ordering.
- Content transforms, crop, canvas/image resize, view pan/zoom/rotation, and high DPI coordinates.
- Exact snapshot undo/redo, saved-state tracking, bounded history with local spill, and cancellation.
- Atomic native ZIP project files preserving raster bytes, identities, shapes, paths, text, and layer properties.
- PNG/JPEG import/export with sRGB conversion/metadata; bounded SVG import/export; PDF raster import and PDF export.
- Background save/export/fill/transform work, stale-result rejection, local recovery generations, and recovery prompts.
- Light/dark/system appearance, dockable panels, native menus/dialogs, canvas tool shortcuts, and preferences for history, recovery, and menu shortcuts.

All processing and storage stay local. The application has no account, telemetry, or cloud service.

## Build and test

Required: CMake 3.25+, C++20 compiler, Qt 6.8+ Core/Gui/Widgets/Concurrent/Svg, and optionally Qt Pdf. Qt 6.10.2 is the tested baseline. A system libzip can be used; otherwise CMake builds the included, hash-verified libzip 1.11.4 source archive.

On macOS with Homebrew Qt already installed:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6;/opt/homebrew" \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
```

Launch `build/Zyri.app` on macOS, or `build/Zyri` on Linux. See [build notes](docs/build.md) for sanitizer, offline dependency, Windows, and packaging instructions.

A headless fixture command creates an editable project and PNG/SVG/PDF output without opening a window:

```sh
QT_QPA_PLATFORM=offscreen build/Zyri.app/Contents/MacOS/Zyri \
  --render-demo /tmp/zyri-demo
```

## Editing conventions

Space or middle mouse pans; Ctrl/Cmd + wheel and pinch zoom. B/N/E/G/I/M/V/P/T select brush, pencil, eraser, fill, picker, rectangle selection, move, pen, and text while the canvas has focus.

A drag is one undo step. Escape cancels gestures and pending background edits. Selections constrain raster edits; Shift adds, Alt subtracts, Shift+Alt intersects. Pen clicks add nodes, dragging makes curves, and Enter/right click finishes. The node tool moves nodes and handles; double click appends a node and Delete removes one. Double click text to edit its content.

Painting requires an unlocked raster layer. Shape tools create a vector layer when needed. Rasterizing vector content is explicit and undoable. Save preserves an editable `.zyri` master; Export creates an interchange file.

## Current limits

This is a development build with automated coverage, not a signed public release. The viewport currently uses CPU composition and a bounded tile cache; GPU presentation and the proposed performance targets are not claimed. History spill can pause under heavy memory pressure. Large move previews and clipboard rendering still perform work on the UI thread.

Documents support dimensions up to 8192 per side, up to 64 Mi pixels, 256 layers, and a 512 MiB document-data budget. Working pixels are 8-bit sRGB. Untagged images are assumed sRGB. Text uses a single font/style per object and depends on installed fonts.

SVG supports simple geometry and transforms. Complex supported-by-Qt content can be imported explicitly as a raster image; external/active resources are rejected. PDF import rasterizes selected pages. PDF/SVG export preserves representable vectors for normal layers and flattens blend-mode compositions to preserve appearance. SVG text interchange is basic; use native projects to preserve editable layout.

Windows/Linux CI configuration and macOS deployment/signing tooling are included. Physical tablet behavior, native desktop visual quality, distribution signing, clean-machine compatibility, and sustained performance still require release validation. No personal screen or accessibility interfaces were used in implementation or verification.

## Source and documentation

- [Implementation status](docs/implementation-status.md): architecture, evidence, limits, and remaining release gates.
- [Build notes](docs/build.md): reproducible commands and dependencies.
- [Verification record](docs/verification.md): observed test, bundle, and artifact checks.
- [Original code audit](docs/code-audit.md): findings from baseline commit `5caa082`.
- [Development plan](docs/implementation-plan.md), [architecture](docs/architecture.md), [tiles](docs/tile-system.md), and [history](docs/undo-design.md): design rationale and longer-term work.
- [Original product brief](Instructions.md): historical context.

The current implementation lives in `src/editor`: Document, Session, ProjectIO, Canvas, and MainWindow. The old empty classes and disconnected engines have been removed.

## License

The project license remains unselected. Dependency license notices belong with distributed packages; the bundled-source libzip notice is retained in `third_party/licenses`.

# Zyri — Architecture Overview

## Module Graph

```
main.cpp
  └─ ZyriApp (Application, AppController, SessionManager)
       ├─ ZyriUI (MainWindow, panels, dialogs, theme)
       │    └─ ZyriCanvas (CanvasViewport, TileCache, GLCompositor)
       │         └─ ZyriEngine (PaintEngine, FillEngine, VectorScene, ...)
       ├─ ZyriTools (ToolRegistry, all tools)
       │    └─ ZyriEngine
       ├─ ZyriHistory (UndoStack, commands, DeltaCompressor)
       │    └─ ZyriCore
       ├─ ZyriIO (ZyriFormat, PNG/JPEG/PDF handlers)
       │    └─ ZyriCore + lcms2 + poppler-qt6
       ├─ ZyriPreferences
       └─ ZyriCore (Document, Layer hierarchy, ColorProfile)
```

## Key Design Decisions

See [tile-system.md](tile-system.md) and [undo-design.md](undo-design.md) for deep dives.

### Canvas Rendering (Phase 2)

- `CanvasViewport` is a `QOpenGLWidget` subclass.
- The canvas is partitioned into 256×256 ARGB32_Premultiplied tiles stored in `RasterLayer`.
- Only dirty tiles are re-uploaded to the GPU via `TileCache`.
- `GLCompositor` renders layer quads bottom-to-top using blend-mode GLSL shaders.
- `ViewTransform` handles pan, zoom, and `devicePixelRatio` (Retina support).

### QTabletEvent Gotcha (macOS + Qt6)

`tabletEvent()` is **not reliably delivered** on macOS with Qt6.
Always intercept tablet events in `CanvasViewport::event()`:

```cpp
bool CanvasViewport::event(QEvent* e) {
    if (e->type() == QEvent::TabletPress || ...) {
        // handle here
    }
    return QOpenGLWidget::event(e);
}
```

### Undo Memory Management

See [undo-design.md](undo-design.md).
Short version: `DeltaCompressor` XORs before/after tile snapshots and zlib-deflates the result.
A 256×256 sparse brush tile typically compresses from 256 KB → < 2 KB.

### macOS Security-Scoped Bookmarks

Without `BookmarkManager`, files opened via `QFileDialog` lose sandbox access on restart.
Always create a bookmark after every successful file open and persist it in `QSettings`.
See [src/platform/macos/BookmarkManager.h](../src/platform/macos/BookmarkManager.h).

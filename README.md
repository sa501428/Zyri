# Zyri

A local-only native desktop graphics editor for macOS (with Windows and Linux targets). Inspired by Paint.NET with lightweight Illustrator-style vector features. Built with C++20 and Qt 6.

![Zyri Logo](logo.png)

---

## Features (Roadmap)

| Phase | Status | Scope |
|-------|--------|-------|
| 0 — Build Infrastructure | **Done** | CMake scaffold, blank window, smoke tests |
| 1 — Core Data Model + Undo | **Done** | Document, LayerStack, delta-compressed undo (100 states) |
| 2 — Canvas Viewport | Planned | OpenGL tiled renderer, pinch-zoom, Retina |
| 3 — Painting + Undo | Planned | Brush/Pencil/Eraser, pressure, pixel-exact undo |
| 4 — UI Shell | Planned | Layer panel, color panel, dark/light theme, document tabs |
| 5 — Complete Tool Set | Planned | Selection, fill, shapes, text, transform |
| 6 — File I/O + Autosave | Planned | `.zyri` format, PNG/JPEG ICC, PDF, crash recovery |
| 7 — Polish + Packaging | Planned | Signed DMG, preferences, rulers, zoom-to-cursor |

---

## Architecture

```
main.cpp
  └─ ZyriApp  (Application, AppController, SessionManager)
       ├─ ZyriUI       (MainWindow, panels, dialogs, theme)
       │    └─ ZyriCanvas  (CanvasViewport, TileCache, GLCompositor, shaders)
       │         └─ ZyriEngine  (PaintEngine, FillEngine, VectorScene)
       ├─ ZyriTools    (ToolRegistry — Brush, Pencil, Eraser, Fill, Select, Shape, Text, Transform)
       ├─ ZyriHistory  (UndoStack 100-state ring, DeltaCompressor, Commands)
       ├─ ZyriIO       (ZyriFormat .zyri ZIP, PNG/JPEG/PDF handlers, Clipboard)
       ├─ ZyriPreferences
       └─ ZyriCore     (Document, Layer hierarchy, ColorProfile / lcms2)
```

Each subdirectory compiles to a static library. See [docs/architecture.md](docs/architecture.md) for the full design, and [docs/tile-system.md](docs/tile-system.md) / [docs/undo-design.md](docs/undo-design.md) for deep dives.

**Key design points:**
- Canvas uses `QOpenGLWidget` with 256×256 ARGB32-premultiplied tiles. Only dirty tiles are re-uploaded to the GPU.
- Undo snapshots XOR before/after tile data and zlib-deflates the result. A typical sparse brush stroke compresses 256 KB → < 2 KB per tile.
- Color management uses [littlecms](https://www.littlecms.com/) (lcms2) for ICC profile embedding and transforms; Qt's own color management is not used.
- macOS sandbox file access is maintained via security-scoped bookmarks (`BookmarkManager.mm`).
- Tablet pressure events are intercepted in `event()`, not `tabletEvent()` — a Qt 6 / macOS requirement.

---

## Requirements

### All platforms
| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | ≥ 3.25 | |
| C++ compiler | C++20 | Clang 15+, GCC 12+, MSVC 2022 |
| Qt 6 | ≥ 6.5 | Core, Gui, Widgets, OpenGL, OpenGLWidgets, Pdf, PrintSupport, Concurrent, Test |
| lcms2 | any recent | ICC color management |
| zlib | any | Bundled with Qt on most platforms |

### macOS (additional)
| Dependency | Notes |
|------------|-------|
| Xcode Command Line Tools | `xcode-select --install` |
| macOS 13 Ventura or later | Minimum deployment target |

### Optional
| Dependency | CMake flag | Notes |
|------------|-----------|-------|
| poppler-qt6 | `-DZYRI_ENABLE_POPPLER=ON` | Enhanced PDF import; off by default |

---

## Building

### macOS (Homebrew) — recommended

```bash
# 1. Install dependencies
brew install qt@6 little-cms2 zlib

# 2. Clone the repo
git clone https://github.com/yourname/Zyri.git
cd Zyri

# 3. Configure
cmake -B build \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6;/opt/homebrew" \
  -DCMAKE_BUILD_TYPE=Release

# 4. Build
cmake --build build --parallel

# 5. Run
open build/Zyri.app
# or
./build/Zyri.app/Contents/MacOS/Zyri
```

For a **debug build with AddressSanitizer**:

```bash
cmake -B build-debug \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6;/opt/homebrew" \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
```

To enable the optional poppler-based PDF importer:

```bash
brew install poppler   # must have poppler-qt6.pc available
cmake -B build -DZYRI_ENABLE_POPPLER=ON ...
```

### Linux (apt / Ubuntu 24.04)

```bash
# Install dependencies
sudo apt update
sudo apt install \
  cmake ninja-build \
  qt6-base-dev qt6-tools-dev qt6-opengl-dev \
  libqt6pdf6-dev libqt6printsupport6 \
  liblcms2-dev zlib1g-dev

# Configure + build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/Zyri
```

### Windows (MSVC + vcpkg)

```powershell
# Install Qt 6 via the Qt online installer or vcpkg, then:
cmake -B build `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\Zyri.exe
```

> Windows support is a secondary target. Some platform-specific paths (macOS `BookmarkManager`, `MacOSAppearance`) are compiled out automatically.

---

## Running Tests

```bash
cmake --build build --target all
cd build && ctest --output-on-failure
```

Tests are organized by module:

```
tests/
  core/     — Document, LayerStack, BlendMode
  engine/   — PaintEngine, FillEngine, DeltaCompressor
  history/  — UndoStack, PaintTileCommand
  io/       — ZyriFormat round-trip, PNGHandler
```

Each test binary is independent (`QTEST_MAIN` per file) and run under ASan in Debug builds.

---

## Project Structure

```
Zyri/
├── CMakeLists.txt
├── cmake/                    — find modules, compiler options, packaging helpers
├── docs/                     — architecture, tile system, undo design
├── packaging/
│   ├── macos/                — Info.plist.in, entitlements, icon.icns
│   ├── linux/                — .desktop file, AppImage config
│   └── windows/              — WiX / NSIS installer
├── resources/
│   └── resources.qrc         — themes (dark/light .qss), icons, cursors
├── src/
│   ├── main.cpp
│   ├── app/                  — Application, AppController, SessionManager (autosave)
│   ├── canvas/               — QOpenGLWidget viewport, TileCache, GLCompositor, shaders
│   ├── core/                 — Document, Layer hierarchy, ColorProfile (lcms2)
│   ├── engine/
│   │   ├── raster/           — PaintEngine, BrushStamp, FillEngine, SelectionMask
│   │   └── vector/           — VectorScene, VectorObject, PathEditor, VectorRasterizer
│   ├── history/              — UndoStack, DeltaCompressor, all Command types
│   ├── io/                   — IOManager, ZyriFormat, PNG/JPEG/PDF/Clipboard handlers
│   ├── platform/macos/       — BookmarkManager.mm, MacOSAppearance.mm
│   ├── preferences/          — PreferencesStore, PreferencesDialog, KeyboardShortcuts
│   ├── tools/                — ToolBase, ToolRegistry, all tool implementations
│   └── ui/                   — MainWindow, panels, dialogs, toolbars, ThemeManager
└── tests/
```

---

## File Format — `.zyri`

Zyri's native format is a ZIP archive with the following layout:

```
document.zyri  (ZIP)
├── manifest.json        — version, canvas size, DPI, layer order, blend modes
├── thumbnail.png        — 256×256 preview
├── colorprofile.icc     — embedded working color profile
├── tiles/
│   └── <layer-id>/
│       └── <tx>_<ty>.bin    — zlib-compressed ARGB32 tile (256×256 = 256 KB raw)
├── vectors/
│   └── <layer-id>.json      — VectorObject array (path data, style)
└── text/
    └── <layer-id>.html      — TextLayer HTML content
```

Saves are atomic: data is written to a temporary file and renamed over the destination, so a crash during save never corrupts the previous version.

---

## macOS Notes

- **Minimum deployment target:** macOS 13 Ventura
- **Bundle ID:** `com.zyri.app`
- **Sandbox entitlements:** `com.apple.security.files.user-selected.read-write` + `com.apple.security.files.bookmarks.app-scope`
- After signing, notarize with `xcrun notarytool` before distribution. See `packaging/macos/`.
- Dark/light mode changes are detected at runtime via `NSDistributedNotificationCenter` (`AppleInterfaceThemeChangedNotification`).

---

## Contributing

The project is in active early development (Phase 0–1 complete). If you'd like to contribute, please open an issue first to discuss the change.

**Code style:**
- C++20, `[[nodiscard]]` on all query methods
- Qt types for UI code, STL types for data structures (`std::vector`, `std::unique_ptr`)
- No `QObject` inheritance on pure data classes (e.g., `Layer`, `ColorProfile`)
- All I/O on worker threads via `QtConcurrent::run()`

---

## License

TBD

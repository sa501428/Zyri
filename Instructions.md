> Historical product brief. The October 2026 [implementation plan](docs/implementation-plan.md) supersedes the technical prescriptions and release scope below. This file is retained as context, not as a statement of implemented behavior.

Updated technical specifications with your changes:

# Technical Specifications — Local-Only Graphics Editor

## 1. Platform and Technology

* **Primary platform:** **macOS**
* **Secondary platforms:** **Windows** and **Linux**
* **Implementation stack:** **C++ + Qt 6**
* **Application type:** Native desktop application, fully local-only
* **Deployment model:** Offline-capable, no cloud sync, no accounts, no mandatory network access

## 2. Core Product Goals

* Extremely fast startup and low-latency interaction
* Smooth, responsive painting and editing even with multiple layers
* Modern desktop look and feel, not an outdated utility-style UI
* Familiar workflow inspired by Paint, Paint.NET, and lightweight Illustrator-style editing
* Local file storage only

## 3. UI / UX Requirements

* **Visual style:** Modern, polished, desktop-native appearance
* **Framework styling:** Qt 6 with custom theming/styling layer for a cleaner contemporary interface
* **Design goals:**

  * Minimal visual clutter
  * High contrast and readable controls
  * Soft modern panel styling, consistent spacing, refined icons
  * Native-feeling menus, shortcuts, dialogs, drag-and-drop
* **Layout:**

  * Top toolbar and application menu
  * Left vertical tools panel
  * Central canvas workspace
  * Right-side dockable panels for layers, colors, and properties
  * Optional bottom status bar
* **Display support:**

  * Full Retina / high-DPI support on macOS
  * Proper scaling on Windows and Linux high-resolution displays
* **Themes:**

  * Dark mode and light mode
  * Prefer macOS system appearance integration
* **Windowing:**

  * Multi-document tabs or multi-window support
  * Standard macOS behaviors where practical

## 4. macOS-First Behavior

* Native macOS menu bar integration
* Native file open/save panels
* Standard macOS keyboard shortcuts and interaction conventions
* Smooth trackpad support:

  * Pinch-to-zoom
  * Two-finger pan where appropriate
* Proper support for Retina rendering
* App bundle packaging for macOS
* Code signing and notarization support for production release

## 5. Cross-Platform Requirements

* Single shared C++/Qt codebase
* Platform-specific adaptations only where needed for native feel
* Uniform document model and rendering behavior across macOS, Windows, and Linux
* Consistent file compatibility across all platforms
* Cross-platform installer/distribution strategy:

  * macOS: `.app` bundle / signed installer
  * Windows: installer package
  * Linux: AppImage or native package optional

## 6. Performance Requirements

* Startup target: under 2 seconds on a modern Mac
* Input response for brush and tool interaction should feel immediate
* Smooth zooming, panning, and selection movement
* Efficient redraw using dirty-region invalidation
* Heavy operations must not freeze the UI
* Worker-thread processing for export, autosave, rasterization, thumbnail generation, and expensive transforms
* Memory-efficient undo/redo and layer compositing

## 7. Architecture

* **Language:** C++
* **UI framework:** Qt 6 Widgets preferred for a professional desktop tool layout
* **Rendering approach:**

  * GPU-assisted canvas rendering where beneficial
  * CPU fallback for compatibility and deterministic behavior
* **Architecture modules:**

  * UI layer
  * Document model
  * Raster engine
  * Vector subsystem
  * Layer/compositing engine
  * Tool framework
  * File import/export
  * Undo/redo engine
  * Preferences and local settings

## 8. Document and Canvas Features

* New file creation with configurable:

  * Canvas size
  * DPI/PPI
  * Background color or transparency
* Fixed-size canvas for v1
* Multiple open documents
* Transparency checkerboard background
* Zoom controls:

  * Fit to screen
  * 100%
  * Custom zoom percentages
  * Mouse wheel / trackpad pinch zoom

## 9. Drawing and Editing Tools

* Pencil
* Brush
* Eraser
* Fill bucket
* Eyedropper
* Straight line
* Rectangle
* Ellipse
* Rounded rectangle
* Polygon/basic shape tools
* Text tool
* Move tool
* Crop tool

## 10. Selection and Transform Tools

* Rectangular selection
* Elliptical selection
* Freehand/lasso selection
* Optional magic wand / color selection
* Move selected content
* Copy, cut, paste, delete
* Scale
* Rotate
* Flip horizontal/vertical
* Resize canvas
* Resize image

## 11. Layers and Transparency

* Multiple layers per document
* Layer types:

  * Raster
  * Vector
  * Text
* Layer controls:

  * Add/delete/duplicate
  * Rename
  * Reorder
  * Show/hide
  * Lock/unlock
  * Opacity adjustment
* Transparency support throughout
* Basic blend modes:

  * Normal
  * Multiply
  * Screen
  * Overlay

## 12. Vector / Illustrator-Like Basic Features

* Basic vector primitives:

  * Line
  * Rectangle
  * Ellipse
  * Polygon
  * Simple path support desirable
* Fill and stroke
* Stroke width and color
* Object opacity
* Basic transform handles
* Simple object ordering
* Limited node/path editing may be deferred if needed

## 13. Color Features

* Full RGBA support
* Color picker and swatches
* RGB and HSV controls
* Hex input
* Primary and secondary colors
* Anti-aliased rendering for shapes and edges
* Optional gradients in later release

## 14. File Format Support

* **Required import/open:**

  * PNG
  * JPEG
  * PDF
* **Required export/save-as:**

  * PNG
  * JPEG
  * PDF
* **Recommended internal editable format:**

  * Custom native project file for preserving layers, vector objects, text, and settings
* **Format behavior:**

  * PNG preserves transparency
  * JPEG exports flattened output with quality setting
  * PDF exports printable/vector-capable output where feasible

## 15. PDF Handling

* PDF export required
* PDF import supported, at minimum through rasterization
* Preserve simple vector content when feasible, but full Illustrator-level PDF editing is not required
* Canvas-to-page mapping must be predictable and accurate

## 16. Brush and Paint Engine

* Brush settings:

  * Size
  * Hardness
  * Opacity
  * Spacing
* Eraser based on same stroke engine
* Fill tool with contiguous area fill
* Tolerance setting desirable
* Brush stroke rendering optimized for low latency

## 17. Undo / Redo

* Full undo and redo stack
* Minimum 100 history states, configurable
* Efficient history model:

  * Command-based for structural changes
  * Delta/tile-based for raster edits

## 18. Reliability and Local Data Safety

* Autosave and crash recovery
* Atomic save behavior where possible
* Recovery files stored locally only
* No remote backup or synchronization
* Graceful recovery after abnormal shutdown

## 19. Privacy and Security

* No telemetry by default
* No cloud features
* No accounts
* No background data transmission
* All caches, preferences, and temp files remain local

## 20. Modern Desktop Quality Requirements

* Crisp iconography
* Consistent spacing and typography
* Smooth panel docking and resizing
* Responsive hover/selection states
* Clean contextual controls and properties panels
* Professional creative-tool feel rather than a basic utility appearance

## 21. Recommended v1 Scope

* C++ / Qt 6 native desktop app
* macOS-first polished UI
* Windows/Linux compatible build targets
* Raster tools, text, shapes, layers, opacity
* Basic vector shapes and transforms
* PNG/JPEG/PDF support
* Fast canvas interaction
* Undo/redo
* Autosave/recovery
* Dark and light modern UI


## 22. Potential Issues & Bottlenecks
Qt 6 Widgets vs. Rendering: While "Qt 6 Widgets" is great for panels/UI, using them for a high-performance painting canvas can be slow. You should explicitly specify QQuickWidget or a Metal/Vulkan/OpenGL viewport for the canvas to ensure the "extremely fast" and "smooth" goals are met on Retina displays. * Vector/Raster Hybrid Complexity: Section 11 mentions Raster, Vector, and Text layers. Handling the coordinate systems and scaling between these (e.g., zooming into a vector shape on a fixed-size raster canvas) is non-trivial. You need a clear Internal Scene Graph strategy.

PDF "Preservation": Section 15 suggests preserving vector content on PDF export. Without a dedicated vector engine (like Cairo or Skia), exporting a mix of raster layers and vector shapes to a layered PDF is a high-effort task for a v1.

## 23. Missing Requirements (Recommended Additions)
Input Handling:

Graphics Tablet Support: Wacom/Apple Pencil (via Sidecar) pressure sensitivity is vital for any tool with a "Brush engine."

Keyboard Modifier Consistency: Explicitly define Spacebar for panning and Cmd/Ctrl for snapping/cloning.

Canvas Usability:

Rotation: Modern editors allow canvas rotation (not just image rotation) for easier drawing angles.

Rulers & Guides: Essential for "Illustrator-style" alignment.

Data Safety:

Scratch Disk Management: For high-res multi-layer files, RAM will fill up. You need a strategy for paging tile data to local disk.

MacOS Specifics:

Permissions: You’ll need a section for macOS Sandboxing and "Files and Folders" permissions, which affects how "Local-Only" apps access user directories.

## 24. Suggested "Quick Wins" for v1
Clipboard Image Support: Being able to "New from Clipboard" or paste an image directly as a new layer.

Export Scaling: A simple "Export at 2x" toggle for web/UI designers.

## 25. The "Retina" Performance Gap

You mentioned Retina support and smooth painting. In a Qt 6 Widgets environment, drawing a 4000x3000px canvas at 2x scale (Retina) requires pushing $4 \times$ the pixels. If you rely on standard QWidget::paintEvent, the UI thread will likely choke during fast brush strokes.Fix: You need a Tiled Rendering system. Instead of redrawing the whole canvas, only update $256 \times 256$ pixel "tiles" that intersect with the brush stroke.

## 26. Sandbox & File System Friction (macOS)

Since you want Native file panels and Local-only storage, macOS App Sandboxing becomes a major hurdle. If a user drags an image from their Desktop into the app, or tries to "Save As" to an external drive, the app may lose permission to access those files after a restart unless you implement Security-Scoped Bookmarks. Without this, your "Local-Only" promise breaks when users can't reopen their own files.

## 27. The "Infinite" Undo Memory Leak

A 100-state history for a multi-layered, high-res document can easily consume 16GB+ of RAM.The Issue: If you store full-frame snapshots for every undo state, the app will crash on older Macs.Missing Spec: You need a Delta-Compression or Swap-to-Disk strategy for the Undo Engine to keep the memory footprint low while maintaining that "fast" feel.

## 28. Color Management (ICC Profiles)

The spec mentions Full RGBA support, but misses Color Profiles.The Problem: If a user saves a PNG on a Mac (P3 Display) and opens it on a Windows machine (sRGB), the colors will look washed out or "wrong."Missing Requirement: You need to specify that the app will embed ICC profiles in exported JPEGs/PNGs to ensure professional "Modern Desktop Quality".

## 29. Vector-to-Raster "Ghosting"

You want Vector layers and Raster tools like the Fill Bucket.The Conflict: If a user draws a vector circle (perfectly sharp) and then tries to use the Fill Bucket (raster-based) next to it, they often get a "white halo" because of anti-aliasing.Missing Spec: A Selection Expansion/Bleed setting for raster tools to account for vector anti-aliasing.
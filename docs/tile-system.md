# Zyri tile and coordinate design

Implementation update: see [implementation status](implementation-status.md) for delivered behavior and departures from this design. The original planning text below is retained as context.

Status: proposed replacement for the prototype. See the [audit](code-audit.md) for current defects.

## Coordinate contract

Keep three coordinate spaces explicit:

| Space | Unit | Uses |
| --- | --- | --- |
| Document | Image pixel, fractional for input and vector geometry | Painting, objects, selection, export |
| View | Device-independent widget unit | Pointer events, pan, zoom, overlays |
| Framebuffer | Physical output pixel | GPU viewport and render targets |

Define the view transform as:

`viewPoint = viewportCenter + zoom × rotation × (documentPoint - documentCenter)`

DPR is absent from that mapping. At the rendering boundary, convert view coordinates to framebuffer pixels using DPR exactly once. Pointer-to-document conversion uses the inverse view transform.

At 100%, one document pixel occupies one logical widget unit. Provide a separate actual-pixel view command if one document pixel per physical display pixel is desired. PPI affects physical output size, not zoom or stored image dimensions.

To zoom around a pointer, compute the document point under it, change zoom, then solve for the new center that preserves that point. Pan uses the inverse rotation and zoom. Define signs once and test them.

Test round trips, cursor anchoring, panning direction, viewport edges, rotation, and DPR values 1, 1.25, 1.5, and 2. A 4096×4096 document stays 4096×4096 on a Retina display.

## Storage contract

Start with 256×256 tiles as a provisional default. Compare 128, 256, and 512 in benchmarks before freezing file-format assumptions. Store tile dimensions in the native format.

A tile has an explicit pixel format, immutable content identity, and revision association. Use canonical RGBA8 premultiplied storage for the first release; CPU adapters may convert internal representations without changing the document contract.

Missing tiles represent fully transparent pixels. Reads return shared immutable storage or a transparent sentinel, without allocating or copying. First mutation within a transaction makes a writable copy; subsequent dabs reuse it. Commit freezes the buffer. Undo stores immutable references or validated payloads and restores absence exactly.

Canvas bounds are finite. Use floor division for negative coordinates, clip edits to the canvas, and zero unused pixels in edge tiles. Normal painting must not allocate arbitrary off-canvas tiles. Canvas resize/crop behavior must explicitly decide whether pixels are discarded; the initial implementation discards cropped pixels through an undoable transaction.

Use checked arithmetic for dimensions, tile count, stride, and allocation size. Reject non-finite and overflowing requests before allocation.

## Cache identity and invalidation

For raw layer textures, identity includes:

`documentId + layerId + tileCoordinate + contentRevision + pixelFormat`

For derived composite tiles, identity includes document/session identity, affected content revisions, layer ordering and properties, output scale, and rendering/color settings. A monotonically increasing render generation may invalidate composites conservatively at first; structured changes later limit work to affected tiles.

A document-wide revision alone is safe but can discard too much cache. A coordinate alone is unsafe. Never rely on pointer addresses as persistent identity.

Invalidation rules:

- Raster edit: affected tile regions.
- Shape/text edit: union of old/new bounds plus stroke/filter support.
- Opacity/visibility/blend change: affected layer bounds and all dependent composites.
- Layer order change: composites influenced by reordered layers.
- Document switch or destruction: namespace separation and eviction.
- Display conversion change: presentation caches, not stored document pixels.

Derived textures and rasterized vectors are disposable. CPU tiles and editable objects remain authoritative.

## Rendering policy

Start with a CPU composite tile cache and explicit visible-region queries. During painting, recompute changed composite tiles; during panning, reuse valid composites. Upload only changed visible tiles to the accelerated viewport.

Do not confuse reduced uploads with partial framebuffer preservation. A viewport can redraw visible cached quads every frame while avoiding expensive recomposition. Culling, tile reuse, and dirty updates are application responsibilities.

For minification, generate derived lower-resolution composites rather than sampling one pixel from many full-resolution tiles. Handle tile borders with filter-aware gutters or neighbor sampling. Test seams, transparent edges, and zoomed-out appearance. Choose nearest-neighbor at high pixel-editing zoom and a documented filtering policy elsewhere.

CPU export accepts arbitrary bounded regions/scales and uses the same compositing semantics. Large exports run in chunks where the codec permits them and check peak scratch memory before beginning.

## Memory accounting

For RGBA8, each 256×256 tile consumes 262,144 bytes, or 256 KiB, before allocator overhead.

| Allocation | Raw size |
| --- | --- |
| One fully allocated 4096×4096 layer | 64 MiB |
| Ten such layers | 640 MiB |
| One fully allocated 8192×8192 layer | 256 MiB |
| Ten such layers | 2.5 GiB |

Sparse layers can use much less. Compression is data-dependent and must not be used as a hard bound.

Start with configurable provisional caps of 256 MiB for resident history payloads, 128 MiB for CPU derived caches, and 128 MiB for GPU caches. These are tuning values for the reference workload, not guarantees for every computer. Account for shared storage once and include active transactions, pending snapshots, decoded images, and worker scratch in peak-memory reporting.

Evict derived caches first. Spill eligible history according to its separate budget. Reject or cancel edits that cannot safely fit the supported document budget; never silently omit tiles. Full document tile paging is deferred until tested workloads require it.

#pragma once

#include <QByteArray>

namespace Zyri {

// Compresses a tile-level undo delta using XOR + zlib deflate.
//
// For a typical brush stroke that modifies <5% of a 256x256 ARGB32 tile the
// XOR buffer is ~95% zero, which deflates to <2 KB — keeping the 100-state
// undo ring under ~50 MB even for 4K canvases.
class DeltaCompressor
{
public:
    // Compute a compressed delta from before → after.
    // Both arrays must be the same size (kTileSize * kTileSize * 4 bytes).
    [[nodiscard]] static QByteArray compress(const QByteArray& before,
                                              const QByteArray& after);

    // Apply a compressed delta to a 'before' snapshot to reconstruct 'after'.
    [[nodiscard]] static QByteArray apply(const QByteArray& before,
                                           const QByteArray& delta);
};

} // namespace Zyri

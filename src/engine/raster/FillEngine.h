#pragma once
#include <QColor>
#include <QImage>
#include <QPoint>

namespace Zyri {
class SelectionMask;

class FillEngine {
public:
    // Flood-fill `image` starting at `seed`, replacing pixels within `tolerance`
    // of the seed color with `fillColor`. The fill is constrained to `mask` if provided.
    static void fill(QImage& image,
                     const QPoint& seed,
                     const QColor& fillColor,
                     int tolerance = 32,
                     const SelectionMask* mask = nullptr);
};
} // namespace Zyri

#pragma once
#include <QImage>
#include <QRect>

namespace Zyri {

// An 8-bit alpha selection mask (0=unselected, 255=fully selected).
// Backed by a QImage in Format_Grayscale8.
class SelectionMask {
public:
    SelectionMask() = default;
    explicit SelectionMask(const QSize& size);

    [[nodiscard]] bool     isEmpty()  const { return m_mask.isNull(); }
    [[nodiscard]] QSize    size()     const { return m_mask.size(); }
    [[nodiscard]] QImage   image()    const { return m_mask; }

    // Returns the selection value [0,255] at pixel (x,y).
    [[nodiscard]] uint8_t at(int x, int y) const;

    // Set the entire mask from a grayscale image.
    void setImage(const QImage& img);

    // Morphological dilation by `pixels` (expands the selection edge).
    // Used to add bleed margin before raster fill near vector edges.
    void expand(int pixels);

    // Select all.
    void selectAll();

    // Deselect all.
    void clear();

private:
    QImage m_mask; // Format_Grayscale8
};

} // namespace Zyri

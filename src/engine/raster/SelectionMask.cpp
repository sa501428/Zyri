#include "SelectionMask.h"
#include <algorithm>
#include <cstring>

namespace Zyri {

SelectionMask::SelectionMask(const QSize& size)
    : m_mask(size, QImage::Format_Grayscale8)
{
    m_mask.fill(0);
}

uint8_t SelectionMask::at(int x, int y) const
{
    if (m_mask.isNull() || x < 0 || y < 0 || x >= m_mask.width() || y >= m_mask.height())
        return 0;
    return *reinterpret_cast<const uint8_t*>(m_mask.constScanLine(y) + x);
}

void SelectionMask::setImage(const QImage& img)
{
    m_mask = img.convertToFormat(QImage::Format_Grayscale8);
}

void SelectionMask::expand(int pixels)
{
    if (m_mask.isNull() || pixels <= 0) return;

    // Simple box-dilation: run a max-filter with a square kernel.
    QImage result = m_mask.copy();
    const int w = m_mask.width(), h = m_mask.height();

    for (int y = 0; y < h; ++y) {
        auto* dst = reinterpret_cast<uint8_t*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            uint8_t maxVal = 0;
            for (int dy = -pixels; dy <= pixels; ++dy) {
                int ny = y + dy;
                if (ny < 0 || ny >= h) continue;
                const auto* src = reinterpret_cast<const uint8_t*>(m_mask.constScanLine(ny));
                for (int dx = -pixels; dx <= pixels; ++dx) {
                    int nx = x + dx;
                    if (nx >= 0 && nx < w)
                        maxVal = std::max(maxVal, src[nx]);
                }
            }
            dst[x] = maxVal;
        }
    }
    m_mask = result;
}

void SelectionMask::selectAll()
{
    if (!m_mask.isNull())
        m_mask.fill(255);
}

void SelectionMask::clear()
{
    if (!m_mask.isNull())
        m_mask.fill(0);
}

} // namespace Zyri

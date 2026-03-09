#include "BlendMode.h"

namespace Zyri {

static inline uint8_t clamp8(int v) noexcept
{
    return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
}

static inline uint8_t mulByte(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>((static_cast<int>(a) * b + 127) / 255);
}

uint32_t compositePixel(uint32_t src, uint32_t dst, BlendMode mode) noexcept
{
    const uint8_t sa = (src >> 24) & 0xFF;
    const uint8_t sr = (src >> 16) & 0xFF;
    const uint8_t sg = (src >>  8) & 0xFF;
    const uint8_t sb = (src      ) & 0xFF;

    const uint8_t da = (dst >> 24) & 0xFF;
    const uint8_t dr = (dst >> 16) & 0xFF;
    const uint8_t dg = (dst >>  8) & 0xFF;
    const uint8_t db = (dst      ) & 0xFF;

    uint8_t br, bg, bb;
    switch (mode) {
    case BlendMode::Multiply:
        br = mulByte(sr, dr);
        bg = mulByte(sg, dg);
        bb = mulByte(sb, db);
        break;
    case BlendMode::Screen:
        br = clamp8(sr + dr - mulByte(sr, dr));
        bg = clamp8(sg + dg - mulByte(sg, dg));
        bb = clamp8(sb + db - mulByte(sb, db));
        break;
    case BlendMode::Overlay:
        br = dr < 128 ? mulByte(2 * dr, sr) : clamp8(255 - 2 * mulByte(255 - dr, 255 - sr));
        bg = dg < 128 ? mulByte(2 * dg, sg) : clamp8(255 - 2 * mulByte(255 - dg, 255 - sg));
        bb = db < 128 ? mulByte(2 * db, sb) : clamp8(255 - 2 * mulByte(255 - db, 255 - sb));
        break;
    case BlendMode::Normal:
    default:
        br = sr; bg = sg; bb = sb;
        break;
    }

    // Porter-Duff "src over dst" alpha compositing
    const int oa = sa + mulByte(da, 255 - sa);
    if (oa == 0) return 0;
    const uint8_t or_ = clamp8((sa * br + mulByte(da, 255 - sa) * dr) / oa);
    const uint8_t og  = clamp8((sa * bg + mulByte(da, 255 - sa) * dg) / oa);
    const uint8_t ob  = clamp8((sa * bb + mulByte(da, 255 - sa) * db) / oa);

    return (static_cast<uint32_t>(oa) << 24) |
           (static_cast<uint32_t>(or_) << 16) |
           (static_cast<uint32_t>(og)  <<  8) |
           (static_cast<uint32_t>(ob));
}

} // namespace Zyri

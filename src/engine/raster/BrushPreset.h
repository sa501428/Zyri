#pragma once

namespace Zyri {

struct BrushPreset {
    float size      = 20.0f;  // diameter in canvas pixels
    float hardness  = 0.8f;   // 0=fully soft, 1=hard edge
    float opacity   = 1.0f;   // stroke alpha
    float spacing   = 0.25f;  // stamp interval as fraction of diameter
    bool  pressureSize    = true;
    bool  pressureOpacity = false;
};

} // namespace Zyri

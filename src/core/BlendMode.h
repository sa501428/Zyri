#pragma once

#include <cstdint>

namespace Zyri {

enum class BlendMode : uint8_t {
    Normal   = 0,
    Multiply = 1,
    Screen   = 2,
    Overlay  = 3,
};

// Composite src over dst using the given blend mode.
// All values are premultiplied ARGB in [0, 255].
uint32_t compositePixel(uint32_t src, uint32_t dst, BlendMode mode) noexcept;

} // namespace Zyri

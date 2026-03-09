#pragma once
// Per-tool option structs (Plain Old Data).
// Used to pass settings from ToolOptionsBar widgets to the tool implementations.

namespace Zyri {

struct BrushToolOptions {
    float size     = 20.0f;
    float hardness = 0.8f;
    float opacity  = 1.0f;
    float spacing  = 0.25f;
    bool  pressureSize    = true;
    bool  pressureOpacity = false;
};

struct FillToolOptions {
    int  tolerance  = 32;
    bool sampleAll  = false; // sample all visible layers vs. active layer only
    int  bleedPixels = 1;    // selection expansion before fill (anti-ghosting)
};

struct ShapeToolOptions {
    float strokeWidth = 1.0f;
    bool  filled      = true;
    bool  stroked     = true;
    bool  toVector    = true; // false = rasterize immediately
};

struct TextToolOptions {
    QString fontFamily = QStringLiteral("Helvetica Neue");
    float   fontSize   = 18.0f;
    bool    bold       = false;
    bool    italic     = false;
};

} // namespace Zyri

#version 330 core

in  vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uLayerTex;
uniform float     uOpacity;

void main()
{
    // Tile data is ARGB32_Premultiplied.  Scale both rgb and alpha by opacity;
    // hardware blend (GL_ONE, GL_ONE_MINUS_SRC_ALPHA) then completes Porter-Duff
    // src-over.  Per-layer blend modes (Multiply, Screen, Overlay, ...) will be
    // added in Phase 3 via FBO-based compositing.
    fragColor = texture(uLayerTex, vTexCoord) * uOpacity;
}

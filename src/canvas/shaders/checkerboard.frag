#version 330 core

in  vec2 vFragPos;
out vec4 fragColor;

uniform vec2  uViewportSize; // physical pixels
uniform float uTileSize;     // checker square size in physical pixels (e.g. 16)

void main()
{
    // Convert from NDC to pixel coordinates.
    vec2 pixel = (vFragPos * 0.5 + 0.5) * uViewportSize;
    ivec2 cell = ivec2(pixel / uTileSize);
    bool even = (cell.x + cell.y) % 2 == 0;
    fragColor = even ? vec4(0.78, 0.78, 0.78, 1.0)
                     : vec4(0.62, 0.62, 0.62, 1.0);
}

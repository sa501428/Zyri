#version 330 core

in  vec2 vCanvasPos;
out vec4 fragColor;

// Size of one checker square in canvas pixels.  At zoom 1 this is 16 screen
// pixels; at zoom 2 it becomes 32 screen pixels, etc., which is the desired
// canvas-relative transparency indicator behaviour.
uniform float uCheckerSize;

void main()
{
    ivec2 cell = ivec2(floor(vCanvasPos / uCheckerSize));
    bool  even = ((cell.x + cell.y) & 1) == 0;
    fragColor  = even ? vec4(0.78, 0.78, 0.78, 1.0)
                      : vec4(0.62, 0.62, 0.62, 1.0);
}

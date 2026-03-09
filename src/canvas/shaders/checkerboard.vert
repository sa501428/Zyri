#version 330 core

// Each vertex carries both its NDC screen position (computed by the CPU from
// the view transform) and the corresponding canvas-space pixel coordinate.
// Interpolating canvas coords in the fragment shader gives a checker pattern
// that is tied to the canvas and therefore scales correctly with zoom.
layout(location = 0) in vec2 aPos;        // NDC position
layout(location = 1) in vec2 aCanvasPos;  // canvas pixel coordinate

out vec2 vCanvasPos;

void main()
{
    vCanvasPos  = aCanvasPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

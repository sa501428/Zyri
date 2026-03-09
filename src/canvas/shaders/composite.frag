#version 330 core

in  vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uLayerTex;
uniform sampler2D uDstTex;
uniform int       uBlendMode;  // 0=Normal, 1=Multiply, 2=Screen, 3=Overlay
uniform float     uOpacity;

vec3 blendMultiply(vec3 src, vec3 dst) { return src * dst; }
vec3 blendScreen(vec3 src, vec3 dst)   { return src + dst - src * dst; }
vec3 blendOverlay(vec3 src, vec3 dst)
{
    return mix(2.0 * src * dst,
               1.0 - 2.0 * (1.0 - src) * (1.0 - dst),
               step(0.5, dst));
}

void main()
{
    vec4 src = texture(uLayerTex, vTexCoord);
    vec4 dst = texture(uDstTex,   vTexCoord);

    src.a *= uOpacity;

    vec3 blended;
    if      (uBlendMode == 1) blended = blendMultiply(src.rgb, dst.rgb);
    else if (uBlendMode == 2) blended = blendScreen(src.rgb, dst.rgb);
    else if (uBlendMode == 3) blended = blendOverlay(src.rgb, dst.rgb);
    else                      blended = src.rgb; // Normal

    // Porter-Duff src-over
    float outA = src.a + dst.a * (1.0 - src.a);
    vec3  outC = (src.a * blended + dst.a * (1.0 - src.a) * dst.rgb)
                 / max(outA, 0.0001);

    fragColor = vec4(outC, outA);
}

#version 440

layout(std140, binding = 0) uniform Ubo {
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 directionalLightColor;
    vec4 directionalLightDirection;
    vec4 fogColor;
    float fogNear;
    float fogFar;
    float opacity;
    float _pad;
} ubo;

layout(location = 0) in vec3 vColor;
layout(location = 1) in float vAlpha;
layout(location = 2) in vec3 vNormalView;
layout(location = 3) in vec3 vPosView;
layout(location = 4) in vec3 vLight;

layout(location = 0) out vec4 fragColor;

void main() {
    // Write explicit window depth from the true view-space surface point, using
    // the SAME formula as sphereimposter.frag. The sphere imposter writes
    // gl_FragDepth manually; if the cylinder relied on fixed-function depth
    // instead, the two would sit on different absolute depth scales (Metal's
    // native NDC z is [0,1], no clip-space correction is applied), so cylinder
    // caps buried inside a sphere would wrongly win the depth test and render in
    // front of the ball. Writing depth the same way keeps both on one scale.
    vec4 clipPos = ubo.projectionMatrix * vec4(vPosView, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    gl_FragDepth = 0.5 * ndcDepth + 0.5;

    vec3 norm = normalize(vNormalView);
    // Two-sided lighting so interior-facing caps/edges are not black.
    float dotProduct = abs(dot(norm, vLight));
    vec3 lightWeight = ubo.directionalLightColor.xyz * max(dotProduct, 0.0);
    vec3 col = lightWeight * vColor;

    fragColor = vec4(col, vAlpha * vAlpha);

    if (ubo.fogNear != ubo.fogFar) {
        float depth = -vPosView.z;
        float fogFactor = smoothstep(ubo.fogNear, ubo.fogFar, depth);
        fragColor = mix(fragColor, vec4(ubo.fogColor.xyz, fragColor.w), fogFactor);
    }
}

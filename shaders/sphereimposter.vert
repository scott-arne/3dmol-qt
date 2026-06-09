#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(std140, binding = 0) uniform Ubo {
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 directionalLightColor;      // xyz used
    vec4 directionalLightDirection;  // xyz used
    vec4 fogColor;                   // xyz used
    float fogNear;
    float fogFar;
    float opacity;
    float _pad;
} ubo;

layout(location = 0) out vec2 mapping;
layout(location = 1) out vec4 vColor;
layout(location = 2) out float rval;
layout(location = 3) out vec3 vLight;
layout(location = 4) out vec3 vCenter;

void main() {
    vColor = color;
    vec4 mvPosition = ubo.modelViewMatrix * vec4(position, 1.0);
    vCenter = mvPosition.xyz;
    vec4 projPosition = ubo.projectionMatrix * mvPosition;
    vec4 adjust = ubo.projectionMatrix * vec4(normal, 0.0);
    adjust.z = 0.0;
    adjust.w = 0.0;
    vec4 lDirection = ubo.viewMatrix * vec4(ubo.directionalLightDirection.xyz, 0.0);
    vLight = normalize(lDirection.xyz);
    mapping = normal.xy;
    rval = abs(normal.x);
    gl_Position = projPosition + adjust;
}

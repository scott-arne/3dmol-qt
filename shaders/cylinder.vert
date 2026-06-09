#version 440

// Unit cylinder mesh attributes (binding 0).
layout(location = 0) in vec3 position;   // unit cylinder, z in [0,1], unit radius
layout(location = 1) in vec3 normal;     // radial/axial normal of unit cylinder

// Per-instance attributes (binding 1).
layout(location = 2) in vec3 instStart;  // bond half start (world)
layout(location = 3) in vec3 instEnd;    // bond half end (world)
layout(location = 4) in float instRadius;
layout(location = 5) in vec4 instColor;  // rgba

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

layout(location = 0) out vec3 vColor;
layout(location = 1) out float vAlpha;
layout(location = 2) out vec3 vNormalView;  // view-space normal
layout(location = 3) out vec3 vPosView;     // view-space position
layout(location = 4) out vec3 vLight;

void main() {
    vColor = instColor.rgb;
    vAlpha = instColor.a;

    // Build an orthonormal basis with w = bond axis direction.
    vec3 axis = instEnd - instStart;
    float len = length(axis);
    vec3 w = (len > 0.0) ? axis / len : vec3(0.0, 0.0, 1.0);
    // Pick a helper not parallel to w to avoid a degenerate cross product.
    vec3 helper = (abs(w.z) < 0.99) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 u = normalize(cross(helper, w));
    vec3 v = cross(w, u);
    mat3 basis = mat3(u, v, w);  // columns u,v,w; maps local +Z -> w

    // Scale unit cylinder: x,y by radius, z by bond length.
    vec3 localScaled = vec3(position.x * instRadius,
                            position.y * instRadius,
                            position.z * len);
    vec3 worldPos = basis * localScaled + instStart;

    // Rotate the unit normal into world space (radial normals have z=0, axial
    // normals have xy=0, so rotation by the basis keeps them correct).
    vec3 worldNormal = normalize(basis * normal);

    vec4 posView = ubo.modelViewMatrix * vec4(worldPos, 1.0);
    vPosView = posView.xyz;
    vNormalView = normalize((ubo.modelViewMatrix * vec4(worldNormal, 0.0)).xyz);

    vec4 lDirection = ubo.viewMatrix * vec4(ubo.directionalLightDirection.xyz, 0.0);
    vLight = normalize(lDirection.xyz);

    gl_Position = ubo.projectionMatrix * posView;
}

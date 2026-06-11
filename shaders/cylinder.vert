#version 440

// Billboard base vertex (binding 0): which endpoint to anchor + lateral sign.
layout(location = 0) in vec2 corner;  // x = endpoint_sel (+1=p1,-1=p2), y = corner_sign

// Per-instance (binding 1): the half-bond.
layout(location = 1) in vec3 instStart;   // p1 (world)
layout(location = 2) in vec3 instEnd;     // p2 (world)
layout(location = 3) in float instRadius;
layout(location = 4) in vec4 instColor;   // rgba

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

layout(location = 0) out vec3 vColor;
layout(location = 1) out float vAlpha;
layout(location = 2) out vec3 vP1;        // view-space cylinder start
layout(location = 3) out vec3 vP2;        // view-space cylinder end
layout(location = 4) out float vR;        // radius
layout(location = 5) out vec3 vCpos;      // view-space billboard fragment pos
layout(location = 6) out vec3 vLight;     // view-space light dir

void main() {
    vColor = instColor.rgb;
    vAlpha = instColor.a;
    vR = instRadius;

    vec4 pt = ubo.modelViewMatrix * vec4(instStart, 1.0);  // p1 in view
    vec4 to = ubo.modelViewMatrix * vec4(instEnd, 1.0);    // p2 in view
    vP1 = pt.xyz;
    vP2 = to.xyz;
    vec3 axis = to.xyz - pt.xyz;

    // Anchor the billboard at the endpoint this vertex selects (3dmol: nearer
    // point is billboarded; we use the per-vertex endpoint_sel which 3dmol
    // encodes via the color-blue sign). corner.x = +1 -> p1, -1 -> p2.
    vec4 mvPosition = (corner.x >= 0.0) ? pt : to;
    float mult = (corner.x >= 0.0) ? 1.1 : -1.1;  // slop, sign per endpoint

    bool isperspective = (ubo.projectionMatrix[3][3] == 0.0);
    vec3 n = normalize(mvPosition.xyz);
    if (isperspective) {
        // Project the anchor onto the camera-facing plane through the endpoint.
        vec3 anchor = (corner.x >= 0.0) ? vP1 : vP2;
        vec3 pnorm = normalize(anchor);
        float t = dot(mvPosition.xyz - anchor, n) / dot(pnorm, n);
        mvPosition.xyz = anchor + t * pnorm;
    } else {
        mvPosition.xyz = (corner.x >= 0.0) ? vP1 : vP2;
    }

    // Push the corner outward by radius along two screen-perpendicular axes.
    // corner.y selects the lateral direction (3dmol uses the radius sign).
    vec3 rdir = isperspective ? mvPosition.xyz : vec3(0.0, 0.0, -1.0);
    vec3 cr = normalize(cross(rdir, axis)) * (instRadius * corner.y);
    vec3 doublecr = normalize(cross(rdir, cr)) * instRadius;
    mvPosition.xyz += mult * (cr + doublecr);

    vCpos = mvPosition.xyz;
    gl_Position = ubo.projectionMatrix * vec4(mvPosition.xyz, 1.0);

    vec4 lDirection = ubo.viewMatrix * vec4(ubo.directionalLightDirection.xyz, 0.0);
    vLight = normalize(lDirection.xyz);
}

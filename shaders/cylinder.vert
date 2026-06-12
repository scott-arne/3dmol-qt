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
    mat4 clipCorrection;
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

    // Build an oriented billboard rectangle that fully encloses the capsule's
    // screen silhouette (body + both hemispherical end caps). Anchor each vertex
    // at the endpoint it selects (corner.x = +1 -> p1, -1 -> p2), then push it
    // OUTWARD along the bond axis (away from the other endpoint) so the quad
    // extends past each cap, and laterally by the radius.
    //
    // The original 3dmol shader relied on a camera-plane projection of the
    // anchor to enlarge the quad; in this port that projection collapsed to a
    // no-op (the anchor maps to itself), so the quad never extended past the
    // endpoints and the far half of each hemispherical cap was clipped, leaving
    // a background hole at bond joints. Pushing along the axis instead covers
    // the whole capsule for every orientation.
    vec3 anchor = (corner.x >= 0.0) ? vP1 : vP2;
    bool isperspective = (ubo.projectionMatrix[3][3] == 0.0);

    // View-space ray direction toward the anchor (perspective) or straight back
    // (orthographic); the lateral axis is perpendicular to both the ray and the
    // bond axis, matching the frag-shader ray setup.
    vec3 rdir = isperspective ? normalize(anchor) : vec3(0.0, 0.0, -1.0);
    vec3 axisDir = normalize(axis);
    vec3 latCross = cross(rdir, axisDir);
    // When the bond points straight at the camera, rdir ∥ axisDir and the cross
    // product vanishes; fall back to an arbitrary axis-perpendicular so the
    // billboard still covers the end-on circular silhouette instead of collapsing.
    vec3 lateral = (dot(latCross, latCross) < 1e-12)
        ? normalize(cross(axisDir, vec3(0.0, 1.0, 0.0)) + cross(axisDir, vec3(1.0, 0.0, 0.0)))
        : normalize(latCross);

    // Outward sign along the axis: -1 at p1, +1 at p2. Lateral sign combines the
    // corner sign with the endpoint so vertices 0,1,2,3 trace the rectangle
    // perimeter (not a bow-tie) under the existing {0,1,2, 2,3,0} index order.
    float axialSign = (corner.x >= 0.0) ? -1.0 : 1.0;
    float lateralSign = corner.y * corner.x;
    float slop = 1.1;  // small margin so rasterized fragments fully cover the cap

    vec4 mvPosition = vec4(anchor, 1.0);
    mvPosition.xyz += axialSign * axisDir * (instRadius * slop);
    mvPosition.xyz += lateralSign * lateral * (instRadius * slop);

    vCpos = mvPosition.xyz;
    // clipCorrection maps GL clip space into the backend's clip volume so the
    // hardware clipper keeps the front half of the frustum (z < 0 in GL terms).
    gl_Position = ubo.clipCorrection * ubo.projectionMatrix * vec4(mvPosition.xyz, 1.0);

    // View-space headlight. The imposter's surface normals are reconstructed in
    // view space and always face the camera, so the light must be fixed relative
    // to the camera too -- otherwise orbiting swings the light behind the
    // camera-facing surface and the molecule goes black (max(dot,0) == 0) at 180
    // degrees. Use the light direction directly in view space rather than
    // rotating it by the view matrix (which would make it world-fixed).
    vLight = normalize(ubo.directionalLightDirection.xyz);
}

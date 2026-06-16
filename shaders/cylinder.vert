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

    // Build a SCREEN-FACING billboard rectangle that fully encloses the
    // capsule's screen silhouette (body + both hemispherical end caps). Each
    // hemisphere cap projects to a disc of radius r in the screen plane no
    // matter how the bond is tilted, so the quad must be expanded in the screen
    // plane (perpendicular to the view ray), NOT along the 3D bond axis.
    //
    // Pushing along the 3D axis (an earlier approach) foreshortens as the bond
    // tilts toward the camera: the axis's on-screen length shrinks while the cap
    // disc does not, so the quad stopped covering the near cap and it rendered as
    // a flat slice. Building the quad from screen-plane axes fixes that for every
    // orientation.
    vec3 anchor = (corner.x >= 0.0) ? vP1 : vP2;
    bool isperspective = (ubo.projectionMatrix[3][3] == 0.0);

    // View ray toward the anchor (perspective) or straight back (orthographic).
    vec3 rdir = isperspective ? normalize(anchor) : vec3(0.0, 0.0, -1.0);
    vec3 axisDir = normalize(axis);
    // Lateral: in the screen plane (perp to the ray) and perp to the bond axis.
    vec3 latCross = cross(rdir, axisDir);
    // When the bond points straight at the camera, rdir is parallel to axisDir
    // and the cross product vanishes; fall back to an arbitrary screen axis so
    // the billboard still covers the end-on circular silhouette.
    bool degenerate = dot(latCross, latCross) < 1e-12;
    // Fallback for an end-on bond (axis parallel to the view ray): cross rdir
    // with whichever basis axis is LEAST parallel to it (smallest |dot|), giving
    // a single guaranteed-nonzero vector to normalize. The previous "sum of two
    // crosses" could cancel to zero (e.g. rdir ~ (a,a,0)) and normalize a zero.
    vec3 fallbackAxis;
    {
        float fx = abs(rdir.x);
        float fy = abs(rdir.y);
        float fz = abs(rdir.z);
        if (fx <= fy && fx <= fz) {
            fallbackAxis = vec3(1.0, 0.0, 0.0);
        } else if (fy <= fz) {
            fallbackAxis = vec3(0.0, 1.0, 0.0);
        } else {
            fallbackAxis = vec3(0.0, 0.0, 1.0);
        }
    }
    vec3 lateral = degenerate
        ? normalize(cross(rdir, fallbackAxis))
        : normalize(latCross);
    // Screen-projected bond axis: perp to the ray AND perp to lateral, i.e. the
    // on-screen direction the bond runs. In the degenerate (end-on) case the bond
    // has no on-screen direction; cross(lateral, rdir) is now well-defined (lateral
    // is a valid nonzero perpendicular). Stays unit length regardless of tilt, so
    // pushing by r along it always clears the cap disc on screen (no foreshortening).
    vec3 axisScreen = normalize(cross(lateral, rdir));

    // Outward sign along the (screen) axis: -1 at p1, +1 at p2. Lateral sign
    // combines the corner sign with the endpoint so vertices 0,1,2,3 trace the
    // rectangle perimeter (not a bow-tie) under the {0,1,2, 2,3,0} index order.
    float axialSign = (corner.x >= 0.0) ? -1.0 : 1.0;
    float lateralSign = corner.y * corner.x;
    float slop = 1.1;  // small margin so rasterized fragments fully cover the cap

    vec4 mvPosition = vec4(anchor, 1.0);
    mvPosition.xyz += axialSign * axisScreen * (instRadius * slop);
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

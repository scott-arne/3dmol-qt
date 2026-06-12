#version 440

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

layout(location = 0) in vec3 vColor;
layout(location = 1) in float vAlpha;
layout(location = 2) in vec3 vP1;
layout(location = 3) in vec3 vP2;
layout(location = 4) in float vR;
layout(location = 5) in vec3 vCpos;
layout(location = 6) in vec3 vLight;

layout(location = 0) out vec4 fragColor;

void main() {
    // Ray from the billboard fragment toward/along the view direction.
    vec3 p = vCpos;
    vec3 v = vec3(0.0, 0.0, -1.0);  // orthographic ray dir
    if (ubo.projectionMatrix[3][3] == 0.0) {
        v = normalize(vCpos);       // perspective ray dir
    }

    vec3 pa = vP1;                  // cylinder start
    vec3 va = normalize(vP2 - vP1); // cylinder axis
    float r = vR;

    // Ray vs infinite cylinder (quadratic).
    vec3 tmp1 = v - (dot(v, va) * va);
    vec3 deltap = p - pa;
    float A = dot(tmp1, tmp1);
    if (A == 0.0) discard;
    vec3 tmp2 = deltap - (dot(deltap, va) * va);
    float B = 2.0 * dot(tmp1, tmp2);
    float C = dot(tmp2, tmp2) - r * r;
    float det = (B * B) - (4.0 * A * C);
    if (det < 0.0) discard;
    float sqrtDet = sqrt(det);
    float posT = (-B + sqrtDet) / (2.0 * A);
    float negT = (-B - sqrtDet) / (2.0 * A);
    float intersectionT = min(posT, negT);
    vec3 qi = p + v * intersectionT;

    float dotp1 = dot(va, qi - vP1);
    float dotp2 = dot(va, qi - vP2);
    vec3 norm;
    if (dotp1 < 0.0 || dotp2 > 0.0) {
        // Past an end: ray vs hemisphere cap sphere at that endpoint.
        vec3 cp = (dotp1 < 0.0) ? vP1 : vP2;
        vec3 diff = p - cp;
        A = dot(v, v);
        B = dot(diff, v) * 2.0;
        C = dot(diff, diff) - r * r;
        det = (B * B) - (4.0 * C);
        if (det < 0.0) discard;
        sqrtDet = sqrt(det);
        posT = (-B + sqrtDet) / 2.0;
        negT = (-B - sqrtDet) / 2.0;
        float t = min(posT, negT);
        qi = p + v * t;
        norm = normalize(qi - cp);
    } else {
        norm = normalize(qi - (dotp1 * va + vP1));
    }

    // Depth from the true hit point, Metal-correct (matches sphereimposter.frag).
    vec4 clipPos = ubo.projectionMatrix * vec4(qi, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    gl_FragDepth = 0.5 * ndcDepth + 0.5;

    // One-sided Lambert + fog + squared alpha, identical to sphereimposter.frag.
    float dotProduct = max(dot(norm, vLight), 0.0);
    vec3 lightWeight = ubo.directionalLightColor.xyz * dotProduct;
    vec3 col = lightWeight * vColor;

    fragColor = vec4(col, vAlpha * vAlpha);

    if (ubo.fogNear != ubo.fogFar) {
        float depth = -qi.z;
        float fogFactor = smoothstep(ubo.fogNear, ubo.fogFar, depth);
        fragColor = mix(fragColor, vec4(ubo.fogColor.xyz, fragColor.w), fogFactor);
    }
}

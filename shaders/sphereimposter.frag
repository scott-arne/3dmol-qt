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

layout(location = 0) in vec2 mapping;
layout(location = 1) in vec4 vColor;
layout(location = 2) in float rval;
layout(location = 3) in vec3 vLight;
layout(location = 4) in vec3 vCenter;

layout(location = 0) out vec4 fragColor;

void main() {
    float lensqr = dot(mapping, mapping);
    float rsqr = rval * rval;
    if (lensqr > rsqr) {
        discard;
    }
    float z = sqrt(rsqr - lensqr);
    vec3 cameraPos = vCenter + vec3(mapping.x, mapping.y, z);
    vec4 clipPos = ubo.projectionMatrix * vec4(cameraPos, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    // Vulkan/RHI clip space depth range is [0,1]; map NDC z to window depth.
    gl_FragDepth = 0.5 * ndcDepth + 0.5;

    vec3 norm = normalize(vec3(mapping.x, mapping.y, z));
    float dotProduct = dot(norm, vLight);
    vec3 directionalLightWeighting = vec3(max(dotProduct, 0.0));
    vec3 lightWeight = ubo.directionalLightColor.xyz * directionalLightWeighting;
    vec3 col = lightWeight * vColor.rgb;

    fragColor = vec4(col, vColor.a);

    if (ubo.fogNear != ubo.fogFar) {
        float depth = -cameraPos.z;
        float fogFactor = smoothstep(ubo.fogNear, ubo.fogFar, depth);
        fragColor = mix(fragColor, vec4(ubo.fogColor.xyz, fragColor.w), fogFactor);
    }
}

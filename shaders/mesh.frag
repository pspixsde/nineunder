#version 450

layout(set = 0, binding = 0) uniform FrameConstants {
    mat4 viewProj;
    mat4 view;
    vec4 cameraPos;
    vec4 lightDir;
    vec4 lightColor;
    vec4 fogColor;
    vec4 ambientColor;
} uFrame;

layout(set = 1, binding = 0) uniform sampler2D uBaseColor;
layout(set = 1, binding = 1) uniform sampler2D uNormalMap;
layout(set = 1, binding = 2) uniform sampler2D uOrm;
layout(set = 1, binding = 3) uniform sampler2D uEmissive;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
    float metallic;
    float roughness;
    float useBaseColorMap;
    float useNormalMap;
    vec4 emissive;
} uPush;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;
layout(location = 4) in vec3 vTangent;
layout(location = 5) in float vTangentW;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = vColor.rgb;
    if (uPush.useBaseColorMap > 0.5) {
        albedo *= texture(uBaseColor, vUV).rgb;
    }

    float metallic = uPush.metallic;
    float roughness = uPush.roughness;
    // Occlusion from ORM.R when present (default texture is white → no change)
    float ao = texture(uOrm, vUV).r;

    vec3 N = normalize(vNormal);
    if (uPush.useNormalMap > 0.5) {
        vec3 T = normalize(vTangent - N * dot(N, vTangent));
        vec3 B = cross(N, T) * vTangentW;
        mat3 TBN = mat3(T, B, N);
        vec3 nTex = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
        N = normalize(TBN * nTex);
    }

    vec3 L = normalize(-uFrame.lightDir.xyz);
    float ndotl = max(dot(N, L), 0.0);
    float wrap = ndotl * 0.85 + 0.15;

    vec3 lit = albedo * (uFrame.ambientColor.rgb * ao + uFrame.lightColor.rgb * wrap);
    lit += uPush.emissive.rgb + texture(uEmissive, vUV).rgb;

    vec3 V = normalize(uFrame.cameraPos.xyz - vWorldPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), mix(8.0, 64.0, 1.0 - roughness));
    lit += uFrame.lightColor.rgb * spec * mix(0.04, 0.35, metallic);

    float dist = length(uFrame.cameraPos.xyz - vWorldPos);
    float fogFactor = 1.0 - exp(-uFrame.fogColor.w * dist);
    lit = mix(lit, uFrame.fogColor.rgb, clamp(fogFactor, 0.0, 1.0));

    lit = lit / (lit + vec3(1.0));
    outColor = vec4(lit, 1.0);
}

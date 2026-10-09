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

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
    float metallic;
    float roughness;
    float pad0;
    float pad1;
    vec4 emissive;
} uPush;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uFrame.lightDir.xyz);
    float ndotl = max(dot(N, L), 0.0);
    float wrap = ndotl * 0.85 + 0.15;

    vec3 albedo = vColor.rgb;
    vec3 lit = albedo * (uFrame.ambientColor.rgb + uFrame.lightColor.rgb * wrap);
    lit += uPush.emissive.rgb;

    // Simple specular (roughness-aware)
    vec3 V = normalize(uFrame.cameraPos.xyz - vWorldPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), mix(8.0, 64.0, 1.0 - uPush.roughness));
    lit += uFrame.lightColor.rgb * spec * mix(0.04, 0.35, uPush.metallic);

    float dist = length(uFrame.cameraPos.xyz - vWorldPos);
    float fogFactor = 1.0 - exp(-uFrame.fogColor.w * dist);
    lit = mix(lit, uFrame.fogColor.rgb, clamp(fogFactor, 0.0, 1.0));

    // Mild tonemap
    lit = lit / (lit + vec3(1.0));
    outColor = vec4(lit, 1.0);
}

#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(set = 0, binding = 0) uniform FrameConstants {
    mat4 viewProj;
    mat4 view;
    vec4 cameraPos;
    vec4 lightDir;
    vec4 lightColor;
    vec4 fogColor;     // rgb + density in w
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

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out vec4 vColor;

void main() {
    vec4 world = uPush.model * vec4(inPosition, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(uPush.model) * inNormal;
    vUV = inUV;
    vColor = inColor * uPush.baseColor;
    gl_Position = uFrame.viewProj * world;
}

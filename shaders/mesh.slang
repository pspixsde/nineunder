// Slang authoring source (mirrors mesh.vert / mesh.frag).
// Compile with: slangc mesh.slang -target spirv -stage vertex -entry vertexMain -o mesh.vert.spv
//               slangc mesh.slang -target spirv -stage fragment -entry fragmentMain -o mesh.frag.spv
// Fallback path uses GLSL + glslangValidator (see tools/compile_shaders.ps1).

struct FrameConstants {
    float4x4 viewProj;
    float4x4 view;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 fogColor;
    float4 ambientColor;
};

struct PushConstants {
    float4x4 model;
    float4 baseColor;
    float metallic;
    float roughness;
    float pad0;
    float pad1;
    float4 emissive;
};

[[vk::binding(0, 0)]] ConstantBuffer<FrameConstants> uFrame;
[[vk::push_constant]] ConstantBuffer<PushConstants> uPush;

struct VsOut {
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 uv : UV;
    float4 color : COLOR;
    float4 pos : SV_Position;
};

[shader("vertex")]
VsOut vertexMain(
    float3 position : POSITION,
    float3 normal : NORMAL,
    float2 uv : UV,
    float4 color : COLOR)
{
    VsOut o;
    float4 world = mul(uPush.model, float4(position, 1.0));
    o.worldPos = world.xyz;
    o.normal = mul((float3x3)uPush.model, normal);
    o.uv = uv;
    o.color = color * uPush.baseColor;
    o.pos = mul(uFrame.viewProj, world);
    return o;
}

[shader("fragment")]
float4 fragmentMain(VsOut i) : SV_Target
{
    float3 N = normalize(i.normal);
    float3 L = normalize(-uFrame.lightDir.xyz);
    float ndotl = max(dot(N, L), 0.0);
    float wrap = ndotl * 0.85 + 0.15;

    float3 albedo = i.color.rgb;
    float3 lit = albedo * (uFrame.ambientColor.rgb + uFrame.lightColor.rgb * wrap);
    lit += uPush.emissive.rgb;

    float3 V = normalize(uFrame.cameraPos.xyz - i.worldPos);
    float3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), lerp(8.0, 64.0, 1.0 - uPush.roughness));
    lit += uFrame.lightColor.rgb * spec * lerp(0.04, 0.35, uPush.metallic);

    float dist = length(uFrame.cameraPos.xyz - i.worldPos);
    float fogFactor = 1.0 - exp(-uFrame.fogColor.w * dist);
    lit = lerp(lit, uFrame.fogColor.rgb, saturate(fogFactor));
    lit = lit / (lit + 1.0);
    return float4(lit, 1.0);
}

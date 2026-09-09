#include "crypt_shared.h"

[[vk::push_constant]] ConstantBuffer<EntityRoot> entity;

// Quad topology and UV layout adapted from the cube example (0, 1, 2, 2, 3, 0)
static const float2 positions[6] = {
    float2(-0.5f, -0.5f),
    float2( 0.5f, -0.5f),
    float2( 0.5f,  0.5f),
    float2( 0.5f,  0.5f),
    float2(-0.5f,  0.5f),
    float2(-0.5f, -0.5f),
};

static const float2 uvs[6] = {
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 color : COLOR0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex_index : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(positions[vertex_index] * entity.scale + entity.position, 0.0f, 1.0f);
    output.uv = uvs[vertex_index];
    output.color = entity.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    // Procedural edge shading: beveled border based on UV distance to edges
    const float2 border = smoothstep(0.0f, 0.15f, min(input.uv, 1.0f - input.uv));
    const float edge = border.x * border.y;
    return float4(input.color * (0.50f + 0.50f * edge), 1.0f);
}

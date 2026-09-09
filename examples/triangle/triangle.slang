#include "triangle_shared.h"

[[vk::push_constant]] ConstantBuffer<PlayerRoot> player;

static const float2 positions[3] = {
    float2( 0.0f, -0.15f),
    float2( 0.12f, 0.15f),
    float2(-0.12f, 0.15f),
};

static const float3 colors[3] = {
    float3(1.0f, 0.85f, 0.30f),
    float3(0.9f, 0.45f, 0.10f),
    float3(0.9f, 0.45f, 0.10f),
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex_index : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(positions[vertex_index] + player.position, 0.0f, 1.0f);
    output.color = colors[vertex_index];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    return float4(input.color, 1.0f);
}

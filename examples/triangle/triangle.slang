#include "triangle_shared.h"

[[vk::push_constant]] ConstantBuffer<EntityRoot> entity;

static const float2 positions[3] = {
    float2( 0.0f, -0.15f),
    float2( 0.12f, 0.15f),
    float2(-0.12f, 0.15f),
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
    output.position = float4(positions[vertex_index] + entity.position, 0.0f, 1.0f);
    output.color = entity.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    return float4(input.color, 1.0f);
}

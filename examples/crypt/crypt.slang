#include "crypt_shared.h"

[[vk::push_constant]] ConstantBuffer<CryptRootArguments> root;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex_id : SV_VertexID)
{
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    VertexOutput output;
    output.position = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    output.uv = uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    float2 screen_pos = (input.position.xy - 0.5f * root.resolution) / min(root.resolution.x, root.resolution.y);

    // Crypt room dimensions
    const float2 room_half = float2(0.80f, 0.56f);
    const float2 d_wall = abs(screen_pos) - room_half;
    const float wall_dist = max(d_wall.x, d_wall.y);

    // Stone tile floor with mortar lines
    const float2 tile_uv = screen_pos * 9.0f;
    const float2 tile_id = floor(tile_uv);
    const float2 tile_frac = frac(tile_uv);
    const float mortar = smoothstep(0.0f, 0.06f, min(tile_frac.x, 1.0f - tile_frac.x)) *
                         smoothstep(0.0f, 0.06f, min(tile_frac.y, 1.0f - tile_frac.y));
    const float tile_hash = frac(sin(dot(tile_id, float2(12.9898f, 78.233f))) * 43758.5453f);
    float3 floor_color = float3(0.14f, 0.15f, 0.17f) * (0.8f + 0.35f * tile_hash) * (0.45f + 0.55f * mortar);

    // Edge shadow near walls
    floor_color *= smoothstep(0.0f, -0.12f, wall_dist);

    // Central ancient sarcophagus
    const float2 d_sarc = abs(screen_pos) - float2(0.15f, 0.24f);
    const float sarc_dist = max(d_sarc.x, d_sarc.y);
    if (sarc_dist <= 0.0f)
    {
        const float cross_x = abs(screen_pos.x);
        const float cross_y = abs(screen_pos.y);
        const bool cross = (cross_x < 0.02f && cross_y < 0.16f) || (cross_y < 0.02f && cross_x < 0.08f);
        const float rim = smoothstep(-0.02f, 0.0f, sarc_dist);
        const float3 tomb_base = cross ? float3(0.28f, 0.26f, 0.24f) : float3(0.18f, 0.17f, 0.16f);
        floor_color = lerp(tomb_base, float3(0.25f, 0.24f, 0.22f), rim);
    }
    else if (sarc_dist < 0.06f)
    {
        floor_color *= (smoothstep(0.0f, 0.06f, sarc_dist) * 0.6f + 0.4f);
    }

    // Outer masonry walls
    if (wall_dist > 0.0f)
    {
        const float2 wall_tile = screen_pos * float2(10.0f, 16.0f);
        const float brick = smoothstep(0.0f, 0.08f, abs(frac(wall_tile.y) - 0.5f) * 2.0f);
        const float3 wall_color = float3(0.07f, 0.07f, 0.09f) * (0.7f + 0.3f * brick);
        floor_color = lerp(floor_color, wall_color, smoothstep(0.0f, 0.015f, wall_dist));
    }

    // Dynamic candlelight
    const float3 ambient = float3(0.02f, 0.025f, 0.038f);
    float3 candle_light = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 4; ++i)
    {
        const bool is_lit = (root.lit_mask & (1u << i)) != 0;
        if (is_lit)
        {
            const float flicker = 0.90f + 0.10f * sin(root.time * 8.0f + float(i) * 2.3f)
                                        + 0.05f * cos(root.time * 13.0f - float(i) * 1.7f);
            const float d = length(screen_pos - root.candle_positions[i]);
            const float atten = (1.0f / (1.0f + d * d * 18.0f)) * flicker;
            candle_light += float3(1.0f, 0.62f, 0.26f) * atten;
        }
    }

    // Faint player presence
    const float d_player = length(screen_pos - root.player_pos);
    const float player_presence = 0.035f / (1.0f + d_player * d_player * 25.0f);
    const float3 total_light = ambient + candle_light + float3(0.3f, 0.4f, 0.6f) * player_presence;

    float3 scene = floor_color * total_light;

    // Pedestals, candles, flames, smoke
    for (int i = 0; i < 4; ++i)
    {
        const float2 candle_pos = root.candle_positions[i];
        const float d_c = length(screen_pos - candle_pos);
        const bool is_lit = (root.lit_mask & (1u << i)) != 0;

        // Stone pedestal
        if (d_c < 0.045f)
        {
            const float rim = smoothstep(0.035f, 0.045f, d_c);
            const float3 ped_col = lerp(float3(0.22f, 0.21f, 0.20f), float3(0.12f, 0.11f, 0.10f), rim);
            scene = ped_col * (total_light * 1.2f + 0.1f);
        }

        // Candle wax
        if (d_c < 0.018f)
        {
            scene = float3(0.82f, 0.78f, 0.70f) * (total_light + 0.2f);
        }

        if (is_lit)
        {
            // Flame core
            if (d_c < 0.024f)
            {
                const float core = smoothstep(0.024f, 0.005f, d_c);
                scene = lerp(scene, float3(1.8f, 1.3f, 0.6f), core);
            }
        }
        else
        {
            // Darkened wick
            if (d_c < 0.006f)
            {
                scene = float3(0.06f, 0.06f, 0.06f);
            }

            // Rising smoke wisp (Y goes down in screen coordinates; up is candle_pos.y - screen_pos.y)
            const float sy = candle_pos.y - screen_pos.y;
            const float sx = screen_pos.x - candle_pos.x - sin(sy * 35.0f + root.time * 4.0f) * 0.007f;
            if (sy > 0.006f && sy < 0.065f && abs(sx) < 0.005f)
            {
                const float smoke_alpha = (1.0f - sy / 0.065f) * 0.45f;
                scene += float3(0.22f, 0.24f, 0.28f) * smoke_alpha;
            }
        }

        // Interactive pulsing proximity halo
        if (root.active_candle == i)
        {
            const float ring = abs(d_c - 0.062f);
            if (ring < 0.008f)
            {
                const float pulse = sin(root.time * 7.0f) * 0.5f + 0.5f;
                const float3 ring_color = is_lit ? float3(1.0f, 0.82f, 0.35f) : float3(0.4f, 0.7f, 1.0f);
                scene += ring_color * smoothstep(0.008f, 0.0f, ring) * (0.6f + 0.4f * pulse);
            }
        }
    }

    // Wanderer / Player figure
    if (d_player < 0.045f)
    {
        const float rim = smoothstep(0.035f, 0.045f, d_player);
        float3 cloak = lerp(float3(0.18f, 0.20f, 0.25f), float3(0.08f, 0.09f, 0.12f), rim);
        if (d_player < 0.022f)
            cloak *= 0.4f;
        scene = cloak * (total_light * 1.5f + 0.25f);
    }

    // Vignette
    const float vig = 1.0f - dot(screen_pos, screen_pos) * 0.45f;
    scene *= max(0.0f, vig);

    return float4(scene, 1.0f);
}

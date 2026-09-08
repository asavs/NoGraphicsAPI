#include "crypt_shared.h"
#include "controls.hpp"
#include "example_support.hpp"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

using namespace gpu;

namespace
{

constexpr uint32 width = 800;
constexpr uint32 height = 600;
constexpr float player_speed = 0.65f;
constexpr float player_radius = 0.045f;
constexpr float room_half_x = 0.75f;
constexpr float room_half_y = 0.52f;
constexpr float sarc_half_x = 0.16f;
constexpr float sarc_half_y = 0.25f;

constexpr float2 candle_positions[4] = {
    { .x = -0.48f, .y = -0.36f },
    { .x =  0.48f, .y = -0.36f },
    { .x = -0.48f, .y =  0.36f },
    { .x =  0.48f, .y =  0.36f },
};

void resolve_collision(float& x, float& y) noexcept
{
    // Clamp to room walls
    const float min_x = -room_half_x + player_radius;
    const float max_x =  room_half_x - player_radius;
    const float min_y = -room_half_y + player_radius;
    const float max_y =  room_half_y - player_radius;

    if (x < min_x) x = min_x;
    if (x > max_x) x = max_x;
    if (y < min_y) y = min_y;
    if (y > max_y) y = max_y;

    // Sarcophagus AABB push-out
    const float box_min_x = -sarc_half_x - player_radius;
    const float box_max_x =  sarc_half_x + player_radius;
    const float box_min_y = -sarc_half_y - player_radius;
    const float box_max_y =  sarc_half_y + player_radius;

    if (x > box_min_x && x < box_max_x && y > box_min_y && y < box_max_y)
    {
        const float d_left   = x - box_min_x;
        const float d_right  = box_max_x - x;
        const float d_bottom = y - box_min_y;
        const float d_top    = box_max_y - y;

        if (d_left <= d_right && d_left <= d_bottom && d_left <= d_top)
            x = box_min_x;
        else if (d_right <= d_bottom && d_right <= d_top)
            x = box_max_x;
        else if (d_bottom <= d_top)
            y = box_min_y;
        else
            y = box_max_y;
    }
}

} // namespace

int main()
{
    void* window = open_example_window("NoGraphicsAPI crypt", width, height);
    Device* device = create_device({.window = window, .swapchain_format = Format::bgra8_srgb}).device;

    if (!window || !device)
    {
        destroy_device(device);
        close_example_window(window);
        return 1;
    }

    printf("Using %s\n", get_device_caps(device).device_name);

    const Span<uint32> vertex_spirv = read_spirv(NOGRAPHICSAPI_CRYPT_VERTEX_SPV_PATH);
    const Span<uint32> fragment_spirv = read_spirv(NOGRAPHICSAPI_CRYPT_FRAGMENT_SPV_PATH);
    PSO* crypt_pso = create_graphics_pso(device, {
        .vertex_spirv = vertex_spirv,
        .fragment_spirv = fragment_spirv,
        .color_targets = { { .format = Format::bgra8_srgb } }
    });
    free(fragment_spirv.data);
    free(vertex_spirv.data);

    TimelinePoint latest_completion{ .semaphore = create_timeline_semaphore(device) };

    game::InputState input_state{};
    float player_x = 0.0f;
    float player_y = 0.40f;
    uint32 lit_mask = 0b1111; // All 4 candles start lit

    double prev_time = example_time_seconds();

    while (pump_example_window(window))
    {
        const double current_time = example_time_seconds();
        float dt = static_cast<float>(current_time - prev_time);
        if (dt > 0.1f) dt = 0.1f;
        prev_time = current_time;

        const game::PlayerInput input = input_state.poll(window);

        // Update player position
        player_x += input.move_x * player_speed * dt;
        player_y += input.move_y * player_speed * dt;
        resolve_collision(player_x, player_y);

        // Detect nearest candle for interaction
        int32 active_candle = -1;
        float min_dist = 0.20f;
        for (int i = 0; i < 4; ++i)
        {
            const float dx = player_x - candle_positions[i].x;
            const float dy = player_y - candle_positions[i].y;
            const float dist = sqrtf(dx * dx + dy * dy);
            if (dist < min_dist)
            {
                min_dist = dist;
                active_candle = i;
            }
        }

        // Toggle candle state on edge-triggered interact
        if (active_candle >= 0 && input.interact)
        {
            lit_mask ^= (1u << active_candle);
        }

        const SwapchainFrame frame = acquire(device);
        if (!frame.render_view)
            continue;

        CommandBuffer* commands = begin_commands(device);
        begin_render_pass(commands, {
            .colors = { { .render_view = frame.render_view, .load = LoadOp::clear } },
        });

        bind_pso(commands, crypt_pso);

        const CryptRootArguments root{
            .candle_positions = { candle_positions[0], candle_positions[1], candle_positions[2], candle_positions[3] },
            .player_pos = { .x = player_x, .y = player_y },
            .resolution = { .x = static_cast<float>(frame.extent.x), .y = static_cast<float>(frame.extent.y) },
            .time = static_cast<float>(current_time),
            .lit_mask = lit_mask,
            .active_candle = active_candle,
        };

        draw(commands, root, 3);
        end_render_pass(commands);

        latest_completion.value++;
        submit_and_present(device, { commands }, latest_completion);
    }

    wait_idle(device);

    destroy_timeline_semaphore(latest_completion.semaphore);
    destroy_pso(crypt_pso);
    destroy_device(device);
    close_example_window(window);
    return 0;
}

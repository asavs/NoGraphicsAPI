#include "crypt_shared.h"
#include "example_support.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace gpu;

namespace
{

struct GameWorld
{
    float2 player_pos{ 0.0f, 0.0f };
    float2 candle_pos{ 0.35f, -0.20f };
    bool candle_lit = true;

    bool is_near_candle() const
    {
        const float dx = player_pos.x - candle_pos.x;
        const float dy = player_pos.y - candle_pos.y;
        return (dx * dx + dy * dy) < (0.22f * 0.22f);
    }

    void step(float move_x, float move_y, bool interact, float dt)
    {
        constexpr float speed = 1.0f;
        player_pos.x += move_x * speed * dt;
        player_pos.y += move_y * speed * dt;

        if (player_pos.x < -0.65f)
            player_pos.x = -0.65f;
        if (player_pos.x > 0.65f)
            player_pos.x = 0.65f;
        if (player_pos.y < -0.43f)
            player_pos.y = -0.43f;
        if (player_pos.y > 0.43f)
            player_pos.y = 0.43f;

        if (is_near_candle() && interact)
            candle_lit = !candle_lit;
    }
};

constexpr EntityRoot room_quads[5] = {
    { .position = { 0.00f, 0.00f }, .scale = { 1.40f, 1.00f }, .color = { 0.10f, 0.10f, 0.13f } },
    { .position = { 0.00f, -0.52f }, .scale = { 1.48f, 0.06f }, .color = { 0.18f, 0.18f, 0.22f } },
    { .position = { 0.00f, 0.52f }, .scale = { 1.48f, 0.06f }, .color = { 0.18f, 0.18f, 0.22f } },
    { .position = { -0.72f, 0.00f }, .scale = { 0.06f, 1.10f }, .color = { 0.18f, 0.18f, 0.22f } },
    { .position = { 0.72f, 0.00f }, .scale = { 0.06f, 1.10f }, .color = { 0.18f, 0.18f, 0.22f } },
};

} // namespace

int main(int argc, char** argv)
{
    const bool headless = (argc > 1 && strcmp(argv[1], "--headless") == 0);

    constexpr uint32 width = 800;
    constexpr uint32 height = 600;

    void* window = headless ? nullptr : open_example_window("crypt example", width, height);
    Device* device = create_device({ .window = window, .swapchain_format = Format::bgra8_srgb }).device;

    if ((!headless && !window) || !device)
    {
        destroy_device(device);
        close_example_window(window);
        return 1;
    }

    printf("Using %s%s\n", get_device_caps(device).device_name, headless ? " (Headless Mode)" : "");

    const Span<uint32> vertex_spirv = read_spirv(NOGRAPHICSAPI_VERTEX_SPV_PATH);
    const Span<uint32> fragment_spirv = read_spirv(NOGRAPHICSAPI_FRAGMENT_SPV_PATH);
    PSO* pso = create_graphics_pso(
        device,
        {
            .vertex_spirv = vertex_spirv,
            .fragment_spirv = fragment_spirv,
            .color_targets = { { .format = Format::bgra8_srgb } },
        }
    );
    free(fragment_spirv.data);
    free(vertex_spirv.data);

    TimelinePoint latest_completion{ .semaphore = create_timeline_semaphore(device) };

    GameWorld world;

    if (headless)
    {
        constexpr int tick_count = 100000;
        const double t0 = example_time_seconds();
        for (int i = 0; i < tick_count; ++i)
        {
            const bool interact = (i == 50000);
            world.step(0.7f, -0.4f, interact, 0.00001f);
        }
        const double t1 = example_time_seconds();
        const double total_ms = (t1 - t0) * 1000.0;
        const double ticks_per_sec = (t1 - t0) > 0.0 ? (double(tick_count) / (t1 - t0)) : 0.0;

        printf("[Crypt 2D Headless Runner]\n");
        printf("Simulated %d ticks in %.2f ms (%.0f ticks/sec)\n", tick_count, total_ms, ticks_per_sec);
        printf("Player: (%.3f, %.3f) | Candle: %s\n", world.player_pos.x, world.player_pos.y, world.candle_lit ? "Lit" : "Extinguished");

        wait_idle(device);
        destroy_timeline_semaphore(latest_completion.semaphore);
        destroy_pso(pso);
        destroy_device(device);
        return 0;
    }

    bool was_enter_down = false;
    bool was_f11_down = false;
    double prev_time = example_time_seconds();

    while (pump_example_window(window))
    {
        const double current_time = example_time_seconds();
        float dt = static_cast<float>(current_time - prev_time);
        if (dt > 0.1f)
            dt = 0.1f;
        prev_time = current_time;

        float move_x = 0.0f;
        float move_y = 0.0f;
        if (example_key_down(window, 'W') || example_key_down(window, 0x26 /* VK_UP */))
            move_y -= 1.0f;
        if (example_key_down(window, 'S') || example_key_down(window, 0x28 /* VK_DOWN */))
            move_y += 1.0f;
        if (example_key_down(window, 'A') || example_key_down(window, 0x25 /* VK_LEFT */))
            move_x -= 1.0f;
        if (example_key_down(window, 'D') || example_key_down(window, 0x27 /* VK_RIGHT */))
            move_x += 1.0f;

        const bool enter_down = example_key_down(window, 0x0D /* VK_RETURN */);
        const bool interact = enter_down && !was_enter_down;
        was_enter_down = enter_down;

        const bool f11_down = example_key_down(window, 0x7A /* VK_F11 */);
        if (f11_down && !was_f11_down)
            example_toggle_fullscreen(window);
        was_f11_down = f11_down;

        world.step(move_x, move_y, interact, dt);

        const bool in_range = world.is_near_candle();
        float3 candle_color;
        if (world.candle_lit)
            candle_color = in_range ? float3{ 1.20f, 0.95f, 0.50f } : float3{ 1.00f, 0.75f, 0.25f };
        else
            candle_color = in_range ? float3{ 0.40f, 0.20f, 0.15f } : float3{ 0.18f, 0.18f, 0.22f };

        const EntityRoot candle{
            .position = world.candle_pos,
            .scale = { 0.06f, 0.10f },
            .color = candle_color,
        };

        const EntityRoot player{
            .position = world.player_pos,
            .scale = { 0.10f, 0.14f },
            .color = { 0.30f, 0.80f, 1.0f },
        };

        const SwapchainFrame frame = acquire(device);
        if (!frame.render_view)
            continue;

        CommandBuffer* commands = begin_commands(device);
        begin_render_pass(
            commands,
            {
                .colors = { { .render_view = frame.render_view, .load = LoadOp::clear, .clear = { 0.06f, 0.06f, 0.10f, 1.0f } } },
            }
        );

        bind_pso(commands, pso);

        for (uint32 i = 0; i < 5; ++i)
            draw(commands, room_quads[i], 6);

        draw(commands, candle, 6);
        draw(commands, player, 6);

        end_render_pass(commands);

        latest_completion.value++;
        submit_and_present(device, { commands }, latest_completion);
    }

    wait_idle(device);

    destroy_timeline_semaphore(latest_completion.semaphore);
    destroy_pso(pso);

    destroy_device(device);
    close_example_window(window);
    return 0;
}

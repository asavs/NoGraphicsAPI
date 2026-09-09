#include "example_support.hpp"
#include "triangle_shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace gpu;

int main(int argc, char** argv)
{
    const bool headless = (argc > 1 && strcmp(argv[1], "--headless") == 0);

    constexpr uint32 width = 800;
    constexpr uint32 height = 600;

    void* window = headless ? nullptr : open_example_window("Sol Eremus 2D", width, height);
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
    PSO* triangle_pso =
        create_graphics_pso(device, { .vertex_spirv = vertex_spirv, .fragment_spirv = fragment_spirv, .color_targets = { { .format = Format::bgra8_srgb } } });
    free(fragment_spirv.data);
    free(vertex_spirv.data);

    TimelinePoint latest_completion{ .semaphore = create_timeline_semaphore(device) };

    EntityRoot player{
        .position = { 0.0f, 0.0f },
        .scale = { 0.10f, 0.14f },
        .color = { 0.30f, 0.80f, 1.0f },
    };

    constexpr float2 candle_pos{ 0.35f, -0.20f };
    bool candle_lit = true;
    bool was_enter_down = false;

    constexpr float speed = 1.0f;
    double prev_time = example_time_seconds();
    if (headless)
    {
        constexpr int tick_count = 10000;
        const double t0 = example_time_seconds();
        for (int i = 0; i < tick_count; ++i)
        {
            player.position.x += 0.00005f;
            const float dx = player.position.x - candle_pos.x;
            const float dy = player.position.y - candle_pos.y;
            const bool in_range = (dx * dx + dy * dy) < (0.22f * 0.22f);
            if (in_range && i == 5000)
                candle_lit = !candle_lit;
        }
        const double t1 = example_time_seconds();
        const double total_ms = (t1 - t0) * 1000.0;
        const double ticks_per_sec = double(tick_count) / (t1 - t0);

        printf("[Sol Eremus 2D Headless Runner]\n");
        printf("Simulated %d ticks in %.2f ms (%.0f ticks/sec)\n", tick_count, total_ms, ticks_per_sec);
        printf("Player: (%.3f, %.3f) | Candle: %s\n", player.position.x, player.position.y, candle_lit ? "Lit" : "Extinguished");

        wait_idle(device);
        destroy_timeline_semaphore(latest_completion.semaphore);
        destroy_pso(triangle_pso);
        destroy_device(device);
        return 0;
    }


    while (pump_example_window(window))
    {
        const double current_time = example_time_seconds();
        float dt = static_cast<float>(current_time - prev_time);
        if (dt > 0.1f)
            dt = 0.1f;
        prev_time = current_time;

        if (example_key_down(window, 'W') || example_key_down(window, 0x26 /* VK_UP */))
            player.position.y -= speed * dt;
        if (example_key_down(window, 'S') || example_key_down(window, 0x28 /* VK_DOWN */))
            player.position.y += speed * dt;
        if (example_key_down(window, 'A') || example_key_down(window, 0x25 /* VK_LEFT */))
            player.position.x -= speed * dt;
        if (example_key_down(window, 'D') || example_key_down(window, 0x27 /* VK_RIGHT */))
            player.position.x += speed * dt;

        if (player.position.x < -0.85f)
            player.position.x = -0.85f;
        if (player.position.x > 0.85f)
            player.position.x = 0.85f;
        if (player.position.y < -0.85f)
            player.position.y = -0.85f;
        if (player.position.y > 0.85f)
            player.position.y = 0.85f;
        const float dx = player.position.x - candle_pos.x;
        const float dy = player.position.y - candle_pos.y;
        const bool in_range = (dx * dx + dy * dy) < (0.22f * 0.22f);

        const bool enter_down = example_key_down(window, 0x0D /* VK_RETURN */);
        if (in_range && enter_down && !was_enter_down)
            candle_lit = !candle_lit;
        was_enter_down = enter_down;
        static bool was_f11_down = false;
        const bool f11_down = example_key_down(window, 0x7A /* VK_F11 */);
        if (f11_down && !was_f11_down)
            example_toggle_fullscreen(window);
        was_f11_down = f11_down;


        float3 candle_color;
        if (candle_lit)
            candle_color = in_range ? float3{ 1.20f, 0.95f, 0.50f } : float3{ 1.00f, 0.75f, 0.25f };
        else
            candle_color = in_range ? float3{ 0.40f, 0.20f, 0.15f } : float3{ 0.18f, 0.18f, 0.22f };

        const EntityRoot candle{
            .position = candle_pos,
            .scale = { 0.06f, 0.10f },
            .color = candle_color,
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

        bind_pso(commands, triangle_pso);

        // Draw candle (amber)
        draw(commands, candle, 6);

        // Draw player (cyan)
        draw(commands, player, 6);

        end_render_pass(commands);

        latest_completion.value++;
        submit_and_present(device, { commands }, latest_completion);
    }

    wait_idle(device);

    destroy_timeline_semaphore(latest_completion.semaphore);
    destroy_pso(triangle_pso);

    destroy_device(device);
    close_example_window(window);
    return 0;
}

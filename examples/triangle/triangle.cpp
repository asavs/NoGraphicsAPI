#include "triangle_shared.h"
#include "example_support.hpp"

#include <stdio.h>
#include <stdlib.h>

using namespace gpu;

int main()
{
    constexpr uint32 width = 800;
    constexpr uint32 height = 600;

    void* window = open_example_window("Sol Eremus 2D", width, height);
    Device* device = create_device({.window = window, .swapchain_format = Format::bgra8_srgb}).device;

    if (!window || !device)
    {
        destroy_device(device);
        close_example_window(window);
        return 1;
    }

    printf("Using %s\n", get_device_caps(device).device_name);

    const Span<uint32> vertex_spirv = read_spirv(NOGRAPHICSAPI_VERTEX_SPV_PATH);
    const Span<uint32> fragment_spirv = read_spirv(NOGRAPHICSAPI_FRAGMENT_SPV_PATH);
    PSO* triangle_pso = create_graphics_pso(device, {
        .vertex_spirv = vertex_spirv,
        .fragment_spirv = fragment_spirv,
        .color_targets = { { .format = Format::bgra8_srgb } }
    });
    free(fragment_spirv.data);
    free(vertex_spirv.data);

    TimelinePoint latest_completion{ .semaphore = create_timeline_semaphore(device) };

    EntityRoot player{
        .position = { 0.0f, 0.0f },
        .color = { 0.30f, 0.80f, 1.0f },
    };

    const EntityRoot candle{
        .position = { 0.35f, -0.20f },
        .color = { 1.0f, 0.75f, 0.25f },
    };

    constexpr float speed = 1.0f;
    double prev_time = example_time_seconds();

    while (pump_example_window(window))
    {
        const double current_time = example_time_seconds();
        float dt = static_cast<float>(current_time - prev_time);
        if (dt > 0.1f) dt = 0.1f;
        prev_time = current_time;

        if (example_key_down(window, 'W') || example_key_down(window, 0x26 /* VK_UP */))
            player.position.y -= speed * dt;
        if (example_key_down(window, 'S') || example_key_down(window, 0x28 /* VK_DOWN */))
            player.position.y += speed * dt;
        if (example_key_down(window, 'A') || example_key_down(window, 0x25 /* VK_LEFT */))
            player.position.x -= speed * dt;
        if (example_key_down(window, 'D') || example_key_down(window, 0x27 /* VK_RIGHT */))
            player.position.x += speed * dt;

        if (player.position.x < -0.85f) player.position.x = -0.85f;
        if (player.position.x >  0.85f) player.position.x =  0.85f;
        if (player.position.y < -0.85f) player.position.y = -0.85f;
        if (player.position.y >  0.85f) player.position.y =  0.85f;

        const SwapchainFrame frame = acquire(device);
        if (!frame.render_view)
            continue;

        CommandBuffer* commands = begin_commands(device);
        begin_render_pass(commands, {
            .colors = { { .render_view = frame.render_view, .load = LoadOp::clear, .clear = { 0.06f, 0.06f, 0.10f, 1.0f } } },
        });

        bind_pso(commands, triangle_pso);

        // Draw candle (amber)
        draw(commands, candle, 3);

        // Draw player (cyan)
        draw(commands, player, 3);

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

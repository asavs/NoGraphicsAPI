#pragma once

#include <NoGraphicsAPI/types.h>

namespace game
{

enum class Action : uint32
{
    move_up,
    move_down,
    move_left,
    move_right,
    interact,
    count
};

struct KeyBinding
{
    int primary = 0;
    int secondary = 0;
};

// Abstracted control settings; can be configured or loaded from settings in subsequent commits
struct ControlSettings
{
    KeyBinding bindings[static_cast<size_t>(Action::count)] = {
        { 'W', 0x26 /* VK_UP */ },
        { 'S', 0x28 /* VK_DOWN */ },
        { 'A', 0x25 /* VK_LEFT */ },
        { 'D', 0x27 /* VK_RIGHT */ },
        { 0x0D /* VK_RETURN */, 0 }
    };
};

struct PlayerInput
{
    float move_x = 0.0f;
    float move_y = 0.0f;
    bool interact = false; // Edge-triggered single frame press
};

struct InputState
{
    ControlSettings settings{};
    bool was_interact_down = false;

    PlayerInput poll(void* window) noexcept;
};

} // namespace game

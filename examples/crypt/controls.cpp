#include "controls.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace game
{

PlayerInput InputState::poll(void* window) noexcept
{
    PlayerInput input{};

    if (GetForegroundWindow() != static_cast<HWND>(window))
    {
        was_interact_down = false;
        return input;
    }

    auto is_down = [this](Action action) noexcept -> bool
    {
        const KeyBinding& b = settings.bindings[static_cast<size_t>(action)];
        if (b.primary != 0 && (GetAsyncKeyState(b.primary) & 0x8000) != 0)
            return true;
        if (b.secondary != 0 && (GetAsyncKeyState(b.secondary) & 0x8000) != 0)
            return true;
        return false;
    };

    if (is_down(Action::move_up))    input.move_y -= 1.0f;
    if (is_down(Action::move_down))  input.move_y += 1.0f;
    if (is_down(Action::move_left))  input.move_x -= 1.0f;
    if (is_down(Action::move_right)) input.move_x += 1.0f;

    if (input.move_x != 0.0f && input.move_y != 0.0f)
    {
        constexpr float inv_sqrt2 = 0.70710678f;
        input.move_x *= inv_sqrt2;
        input.move_y *= inv_sqrt2;
    }

    const bool interact_is_down = is_down(Action::interact);
    input.interact = interact_is_down && !was_interact_down;
    was_interact_down = interact_is_down;

    return input;
}

} // namespace game

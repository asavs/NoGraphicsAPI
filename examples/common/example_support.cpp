#include "example_support.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace gpu;

Span<uint32> read_spirv(const char* path) noexcept
{
    assert(path);
    FILE* file = fopen(path, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open SPIR-V file: %s\n", path);
        return {};
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Failed to read SPIR-V file: %s\n", path);
        fclose(file);
        return {};
    }
    const long byte_count = ftell(file);
    if (byte_count < static_cast<long>(5 * sizeof(uint32)) || byte_count % static_cast<long>(sizeof(uint32)) != 0)
    {
        fprintf(stderr, "Invalid SPIR-V file size: %s\n", path);
        fclose(file);
        return {};
    }
    rewind(file);

    Span<uint32> code(static_cast<uint32*>(malloc(size_t(byte_count))), size_t(byte_count) / sizeof(uint32));
    const bool read_succeeded = fread(code.data, sizeof(uint32), code.size, file) == code.size;
    fclose(file);
    if (!read_succeeded || code.data[0] != 0x07230203u)
    {
        fprintf(stderr, "Invalid SPIR-V file: %s\n", path);
        free(code.data);
        return {};
    }
    return code;
}

bool read_binary_file(const char* path, Span<byte> data) noexcept
{
    assert(path && data.data && data.size);
    FILE* file = fopen(path, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open resource file: %s\n", path);
        return false;
    }
    const bool size_succeeded = fseek(file, 0, SEEK_END) == 0 && ftell(file) == static_cast<long>(data.size);
    rewind(file);
    const bool read_succeeded = size_succeeded && fread(data.data, 1, data.size, file) == data.size;
    fclose(file);
    if (!read_succeeded)
        fprintf(stderr, "Invalid resource file: %s\n", path);
    return read_succeeded;
}

double example_time_seconds() noexcept
{
    static double seconds_per_tick = 0.0;
    if (seconds_per_tick == 0.0)
    {
        LARGE_INTEGER frequency{};
        QueryPerformanceFrequency(&frequency);
        seconds_per_tick = 1.0 / double(frequency.QuadPart);
    }
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return double(counter.QuadPart) * seconds_per_tick;
}
bool example_key_down(void* window, int key) noexcept
{
    if (GetForegroundWindow() != static_cast<HWND>(window))
        return false;
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}
void example_toggle_fullscreen(void* window) noexcept
{
    HWND hwnd = static_cast<HWND>(window);
    static bool is_fullscreen = false;
    static RECT prev_rect{};
    static DWORD prev_style = 0;

    if (!is_fullscreen)
    {
        prev_style = static_cast<DWORD>(GetWindowLongPtrA(hwnd, GWL_STYLE));
        GetWindowRect(hwnd, &prev_rect);
        SetWindowLongPtrA(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
        is_fullscreen = true;
    }
    else
    {
        SetWindowLongPtrA(hwnd, GWL_STYLE, prev_style | WS_VISIBLE);
        SetWindowPos(hwnd, nullptr, prev_rect.left, prev_rect.top,
                     prev_rect.right - prev_rect.left, prev_rect.bottom - prev_rect.top, SWP_FRAMECHANGED);
        is_fullscreen = false;
    }
}



namespace
{

constexpr const char* window_class_name = "NoGraphicsAPI_example_window";
constexpr DWORD window_style = WS_OVERLAPPEDWINDOW;

LRESULT CALLBACK example_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            ShowWindow(hwnd, SW_HIDE);
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcA(hwnd, message, wparam, lparam);
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }
}

} // namespace

void* open_example_window(const char* title, uint32 width, uint32 height) noexcept
{
    assert(title && width && height);
    const HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSEXA window_class{
        .cbSize = sizeof(WNDCLASSEXA),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = example_window_proc,
        .hInstance = instance,
        .hCursor = LoadCursorA(nullptr, IDC_ARROW),
        .lpszClassName = window_class_name,
    };
    if (!RegisterClassExA(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return {};

    RECT rectangle{
        .right = static_cast<LONG>(width),
        .bottom = static_cast<LONG>(height),
    };
    if (!AdjustWindowRectEx(&rectangle, window_style, FALSE, 0))
        return {};

    const HWND hwnd = CreateWindowExA(
        0,
        window_class_name,
        title,
        window_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rectangle.right - rectangle.left,
        rectangle.bottom - rectangle.top,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (!hwnd)
        return {};
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    return hwnd;
}

bool pump_example_window(void* window) noexcept
{
    for (;;)
    {
        MSG message{};
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
                return false;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        if (!IsIconic(static_cast<HWND>(window)))
            return true;
        WaitMessage();
    }
}

void close_example_window(void*& window) noexcept
{
    if (window)
        DestroyWindow(static_cast<HWND>(window));
    window = nullptr;
}

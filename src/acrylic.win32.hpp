#pragma once
#include <Windows.h>

namespace saucer
{
    enum class composition_attribute
    {
        accent_policy = 19,
    };

    enum class accent_state
    {
        disabled = 0,
        blur_behind = 3,
        acrylic_blur_behind = 4,
        transparent_gradient = 2,
    };

    struct accent_policy
    {
        accent_state state;
        DWORD flags;
        DWORD gradient_color;
        DWORD animation_id;
    };

    struct composition_data
    {
        composition_attribute attribute;
        accent_policy *policy;
        SIZE_T data_size;
    };

    inline auto SetWindowCompositionAttribute = [] {
        auto *user32dll = LoadLibraryW(L"user32.dll");
        return reinterpret_cast<BOOL(WINAPI *)(HWND, composition_data *)>(GetProcAddress(user32dll, "SetWindowCompositionAttribute"));
    }();
} // namespace saucer

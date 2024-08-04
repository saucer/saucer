#pragma once

#include <type_traits>

namespace saucer
{
    struct qt67_warning
    {
        [[deprecated("This functionality is only available from QT6.7 onwards")]]
        static void raise()
        {
        }
    };

    struct gtk_warning
    {
        [[deprecated("This functionality is not available in GTK")]]
        static void raise()
        {
        }
    };

    struct gtk_buggy_warning
    {
        [[deprecated("This functionality does not work properly in GTK due to upstream issues")]]
        static void raise()
        {
        }
    };

    struct gtk_wayland_warning
    {
        [[deprecated("GTK does not implement this functionality correctly under Wayland")]]
        static void raise()
        {
        }
    };

    template <typename T>
    consteval void emit_warning()
    {
        if constexpr (!std::is_constant_evaluated())
        {
            T::raise();
        }
    }
} // namespace saucer

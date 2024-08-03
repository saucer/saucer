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

    template <typename T>
    void emit_warning()
    {
        if (!std::is_constant_evaluated())
        {
            T::raise();
        }
    }
} // namespace saucer

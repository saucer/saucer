#include "window.gtk.impl.hpp"

namespace saucer
{
    bool window::impl::is_thread_safe() const
    {
        return creation_thread == std::this_thread::get_id();
    }
} // namespace saucer

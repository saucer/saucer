#include "window.gtk.impl.hpp"

namespace saucer
{
    bool window::impl::is_thread_safe()
    {
        return impl::application != nullptr;
    }
} // namespace saucer

#include "window.hpp"

namespace saucer
{
    window::impl *window::native() const
    {
        return m_impl.get();
    }
} // namespace saucer

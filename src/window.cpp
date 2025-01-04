#include "window.hpp"

namespace saucer
{
    application &window::parent() const
    {
        return *m_parent;
    }
} // namespace saucer

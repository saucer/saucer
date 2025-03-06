#include "qt.app.impl.hpp"

namespace saucer
{
    screen application::impl::convert(QScreen *screen)
    {
        const auto geometry = screen->geometry();

        return {
            .name     = screen->name().toStdString(),
            .size     = {geometry.width(), geometry.height()},
            .position = {geometry.x(), geometry.y()},
        };
    }

    safe_event::safe_event(callback_t callback) : QEvent(QEvent::User), m_callback(std::move(callback)) {}

    safe_event::~safe_event()
    {
        std::invoke(m_callback);
    }
} // namespace saucer

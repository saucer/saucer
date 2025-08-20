#include "qt.app.impl.hpp"

namespace saucer
{
    using native = application::impl::native;

    void native::iteration()
    {
        QApplication::processEvents();
    }

    screen native::convert(QScreen *screen)
    {
        const auto geometry = screen->geometry();

        return {
            .name     = screen->name().toStdString(),
            .size     = {.w = geometry.width(), .h = geometry.height()},
            .position = {.x = geometry.x(), .y = geometry.y()},
        };
    }

    safe_event::safe_event(callback_t callback) : QEvent(QEvent::User), m_callback(std::move(callback)) {}

    safe_event::~safe_event()
    {
        std::invoke(m_callback);
    }
} // namespace saucer

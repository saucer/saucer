#include "qt.window.impl.hpp"

#include "qt.app.impl.hpp"
#include "qt.icon.impl.hpp"

#include "instantiate.hpp"

#include <QWindow>

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::window::edge> = true;

namespace saucer
{
    using impl = window::impl;

    impl::impl() = default;

    result<> impl::init_platform()
    {
        platform = std::make_unique<native>();

        platform->window   = new main_window{this};
        platform->max_size = platform->window->maximumSize();
        platform->min_size = platform->window->minimumSize();

        auto *const content = new QWidget{platform->window};
        content->setLayout(new overlay_layout);

        platform->window->setCentralWidget(content);

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        platform->window->close();
        platform->window->deleteLater();
    }

    template <window::event Event>
    void impl::setup()
    {
    }

    bool impl::visible() const
    {
        return platform->window->isVisible();
    }

    bool impl::focused() const
    {
        return platform->window->isActiveWindow();
    }

    bool impl::minimized() const
    {
        return platform->window->isMinimized();
    }

    bool impl::maximized() const
    {
        return platform->window->isMaximized();
    }

    bool impl::resizable() const
    {
        return platform->window->maximumSize() != platform->window->minimumSize();
    }

    bool impl::fullscreen() const
    {
        return platform->window->isFullScreen();
    }

    bool impl::always_on_top() const
    {
        return platform->window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
    }

    bool impl::click_through() const
    {
        return platform->window->windowFlags().testFlag(Qt::WindowTransparentForInput);
    }

    std::string impl::title() const
    {
        return platform->window->windowTitle().toStdString();
    }

    color impl::background() const
    {
        const auto palette = platform->window->palette();
        const auto color   = palette.color(QPalette::ColorRole::Window);

        return {
            .r = static_cast<std::uint8_t>(color.red()),
            .g = static_cast<std::uint8_t>(color.green()),
            .b = static_cast<std::uint8_t>(color.blue()),
            .a = static_cast<std::uint8_t>(color.alpha()),
        };
    }

    window::decoration impl::decorations() const
    {
        using enum decoration;

        if (platform->window->windowFlags().testFlag(Qt::FramelessWindowHint))
        {
            return none;
        }

        if (platform->window->windowFlags().testFlag(Qt::CustomizeWindowHint))
        {
            return partial;
        }

        return full;
    }

    size impl::size() const
    {
        return {.w = platform->window->width(), .h = platform->window->height()};
    }

    size impl::max_size() const
    {
        return {.w = platform->window->maximumWidth(), .h = platform->window->maximumHeight()};
    }

    size impl::min_size() const
    {
        return {.w = platform->window->minimumWidth(), .h = platform->window->minimumHeight()};
    }

    position impl::position() const
    {
        return {.x = platform->window->x(), .y = platform->window->y()};
    }

    std::optional<saucer::screen> impl::screen() const
    {
        auto *const screen = platform->window->screen();

        if (!screen)
        {
            return std::nullopt;
        }

        return application::impl::native::convert(screen);
    }

    void impl::hide() const
    {
        platform->window->hide();
    }

    void impl::show() const
    {
        platform->window->show();
    }

    void impl::close() const
    {
        platform->window->close();
    }

    void impl::focus() const
    {
        platform->window->activateWindow();
    }

    void impl::start_drag() const
    {
        platform->window->windowHandle()->startSystemMove();
    }

    void impl::start_resize(edge edge) // NOLINT(*-function-const)
    {
        using enum window::edge;
        using enum Qt::Edge;

        Qt::Edges translated;

        if (edge & top)
        {
            translated |= TopEdge;
        }
        if (edge & bottom)
        {
            translated |= BottomEdge;
        }
        if (edge & left)
        {
            translated |= LeftEdge;
        }
        if (edge & right)
        {
            translated |= RightEdge;
        }

        platform->window->windowHandle()->startSystemResize(translated);
    }

    void impl::set_minimized(bool enabled) // NOLINT(*-function-const)
    {
        using enum Qt::WindowState;

        auto state = platform->window->windowState();

        if (enabled)
        {
            state |= WindowMinimized;
        }
        else
        {
            state &= ~WindowMinimized;
            state |= WindowActive;
        }

        platform->window->setWindowState(state);
    }

    void impl::set_maximized(bool enabled) // NOLINT(*-function-const)
    {
        using enum Qt::WindowState;

        auto state = platform->window->windowState();

        if (enabled)
        {
            state |= WindowMaximized;
        }
        else
        {
            state &= ~WindowMaximized;
        }

        platform->window->setWindowState(state);
    }

    void impl::set_resizable(bool enabled) // NOLINT(*-function-const)
    {
        if (!enabled)
        {
            platform->window->setFixedSize(platform->window->size());
            return;
        }

        platform->window->setMaximumSize(platform->max_size);
        platform->window->setMinimumSize(platform->min_size);
    }

    void impl::set_fullscreen(bool enabled) // NOLINT(*-function-const)
    {
        using enum Qt::WindowState;

        auto state = platform->window->windowState();

        if (enabled)
        {
            state |= WindowFullScreen;
        }
        else
        {
            state &= ~WindowFullScreen;
        }

        platform->window->setWindowState(state);
    }

    void impl::set_always_on_top(bool enabled) // NOLINT(*-function-const)
    {
        platform->set_flags({{Qt::WindowStaysOnTopHint, enabled}});
    }

    void impl::set_click_through(bool enabled) // NOLINT(*-function-const)
    {
        platform->set_flags({{Qt::WindowTransparentForInput, enabled}});
    }

    void impl::set_icon(const icon &icon) // NOLINT(*-function-const)
    {
        if (icon.empty())
        {
            return;
        }

        platform->window->setWindowIcon(icon.native<false>()->icon);
    }

    void impl::set_title(const std::string &title) // NOLINT(*-function-const)
    {
        platform->window->setWindowTitle(QString::fromStdString(title));
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        auto palette            = platform->window->palette();
        const auto [r, g, b, a] = color;

        palette.setColor(QPalette::ColorRole::Window, {r, g, b, a});

        platform->window->setPalette(palette);
        platform->window->setAttribute(Qt::WA_TranslucentBackground, a < 255);
    }

    void impl::set_decorations(decoration decoration) // NOLINT(*-function-const)
    {
        using enum window::decoration;
        platform->set_flags({{Qt::CustomizeWindowHint, decoration == partial}, {Qt::FramelessWindowHint, decoration == none}});
    }

    void impl::set_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->window->resize(size.w, size.h);
    }

    void impl::set_max_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->window->setMaximumSize(size.w, size.h);
        platform->max_size = platform->window->maximumSize();
    }

    void impl::set_min_size(saucer::size size) // NOLINT(*-function-const)
    {
        platform->window->setMinimumSize(size.w, size.h);
        platform->min_size = platform->window->minimumSize();
    }

    void impl::set_position(saucer::position pos) // NOLINT(*-function-const)
    {
        platform->window->move(pos.x, pos.y);
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_IMPL_EVENT);
} // namespace saucer

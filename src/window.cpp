#include "window.impl.hpp"

#include "instantiate.hpp"

#include <cassert>

#include <rebind/enum.hpp>

namespace saucer
{
    window::window() : m_events(std::make_unique<events>()), m_impl(std::make_unique<impl>()) {}

    std::shared_ptr<window> window::create(application *parent)
    {
        if (!parent->thread_safe())
        {
            return {};
        }

        auto rtn = std::shared_ptr<window>(new window);

        rtn->m_impl->parent = parent;
        rtn->m_impl->events = rtn->m_events.get();

        if (!rtn->m_impl->init_platform())
        {
            return {};
        }

        return rtn;
    }

    window::~window()
    {
        for (const auto &event : rebind::enum_values<event>)
        {
            m_events->clear(event);
        }
    }

    template <window::event Event>
    void window::setup()
    {
        return invoke(m_impl.get(), &impl::setup<Event>);
    }

    application &window::parent() const
    {
        return *m_impl->parent;
    }

    bool window::visible() const
    {
        return invoke(m_impl.get(), &impl::visible);
    }

    bool window::focused() const
    {
        return invoke(m_impl.get(), &impl::focused);
    }

    bool window::minimized() const
    {
        return invoke(m_impl.get(), &impl::minimized);
    }

    bool window::maximized() const
    {
        return invoke(m_impl.get(), &impl::maximized);
    }

    bool window::resizable() const
    {
        return invoke(m_impl.get(), &impl::resizable);
    }

    bool window::always_on_top() const
    {
        return invoke(m_impl.get(), &impl::always_on_top);
    }

    bool window::click_through() const
    {
        return invoke(m_impl.get(), &impl::click_through);
    }

    std::string window::title() const
    {
        return invoke(m_impl.get(), &impl::title);
    }

    window::decoration window::decorations() const
    {
        return invoke(m_impl.get(), &impl::decorations);
    }

    size window::size() const
    {
        return invoke(m_impl.get(), &impl::size);
    }

    size window::max_size() const
    {
        return invoke(m_impl.get(), &impl::max_size);
    }

    size window::min_size() const
    {
        return invoke(m_impl.get(), &impl::min_size);
    }

    position window::position() const
    {
        return invoke(m_impl.get(), &impl::position);
    }

    std::optional<screen> window::screen() const
    {
        return invoke(m_impl.get(), &impl::screen);
    }

    void window::hide()
    {
        return invoke(m_impl.get(), &impl::hide);
    }

    void window::show()
    {
        return invoke(m_impl.get(), &impl::show);
    }

    void window::close()
    {
        return invoke(m_impl.get(), &impl::close);
    }

    void window::focus()
    {
        return invoke(m_impl.get(), &impl::focus);
    }

    void window::start_drag()
    {
        return invoke(m_impl.get(), &impl::start_drag);
    }

    void window::start_resize(edge edge)
    {
        return invoke(m_impl.get(), &impl::start_resize, edge);
    }

    void window::set_minimized(bool enabled)
    {
        return invoke(m_impl.get(), &impl::set_minimized, enabled);
    }

    void window::set_maximized(bool enabled)
    {
        return invoke(m_impl.get(), &impl::set_maximized, enabled);
    }

    void window::set_resizable(bool enabled)
    {
        return invoke(m_impl.get(), &impl::set_resizable, enabled);
    }

    void window::set_always_on_top(bool enabled)
    {
        return invoke(m_impl.get(), &impl::set_always_on_top, enabled);
    }

    void window::set_click_through(bool enabled)
    {
        return invoke(m_impl.get(), &impl::set_click_through, enabled);
    }

    void window::set_icon(const icon &icon)
    {
        return invoke(m_impl.get(), &impl::set_icon, icon);
    }

    void window::set_decorations(decoration decoration)
    {
        return invoke(m_impl.get(), &impl::set_decorations, decoration);
    }

    void window::set_title(const std::string &title)
    {
        return invoke(m_impl.get(), &impl::set_title, title);
    }

    void window::set_size(const saucer::size &size)
    {
        return invoke(m_impl.get(), &impl::set_size, size);
    }

    void window::set_max_size(const saucer::size &size)
    {
        return invoke(m_impl.get(), &impl::set_max_size, size);
    }

    void window::set_min_size(const saucer::size &size)
    {
        return invoke(m_impl.get(), &impl::set_min_size, size);
    }

    void window::set_position(const saucer::position &position)
    {
        return invoke(m_impl.get(), &impl::set_position, position);
    }

    void window::off(event event)
    {
        return invoke(m_impl.get(), [impl = m_impl.get(), event] { impl->events->clear(event); });
    }

    void window::off(event event, std::uint64_t id)
    {
        return invoke(m_impl.get(), [impl = m_impl.get(), event, id] { impl->events->remove(event, id); });
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_EVENT);
} // namespace saucer

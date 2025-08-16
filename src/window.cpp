#include "window.impl.hpp"

#include "invoke.hpp"
#include "instantiate.hpp"

#include <rebind/enum.hpp>

namespace saucer
{
    window::window() : m_events(std::make_unique<events>()), m_impl(std::make_unique<impl>()) {}

    result<std::shared_ptr<window>> window::create(application *parent)
    {
        if (!parent->thread_safe())
        {
            return err(contract_error::not_main_thread);
        }

        auto rtn = std::shared_ptr<window>(new window);

        rtn->m_impl->parent = parent;
        rtn->m_impl->events = rtn->m_events.get();

        if (auto status = rtn->m_impl->init_platform(); !status.has_value())
        {
            return err(status);
        }

        return rtn;
    }

    window::~window()
    {
        if (!m_impl)
        {
            return;
        }

        for (const auto &event : rebind::enum_values<event>)
        {
            m_events->clear(event);
        }
    }

    template <window::event Event>
    void window::setup()
    {
        return invoke<&impl::setup<Event>>(m_impl.get());
    }

    application &window::parent() const
    {
        return *m_impl->parent;
    }

    bool window::visible() const
    {
        return invoke<&impl::visible>(m_impl.get());
    }

    bool window::focused() const
    {
        return invoke<&impl::focused>(m_impl.get());
    }

    bool window::minimized() const
    {
        return invoke<&impl::minimized>(m_impl.get());
    }

    bool window::maximized() const
    {
        return invoke<&impl::maximized>(m_impl.get());
    }

    bool window::resizable() const
    {
        return invoke<&impl::resizable>(m_impl.get());
    }

    bool window::fullscreen() const
    {
        return invoke<&impl::fullscreen>(m_impl.get());
    }

    bool window::always_on_top() const
    {
        return invoke<&impl::always_on_top>(m_impl.get());
    }

    bool window::click_through() const
    {
        return invoke<&impl::click_through>(m_impl.get());
    }

    std::string window::title() const
    {
        return invoke<&impl::title>(m_impl.get());
    }

    color window::background() const
    {
        return invoke<&impl::background>(m_impl.get());
    }

    window::decoration window::decorations() const
    {
        return invoke<&impl::decorations>(m_impl.get());
    }

    size window::size() const
    {
        return invoke<&impl::size>(m_impl.get());
    }

    size window::max_size() const
    {
        return invoke<&impl::max_size>(m_impl.get());
    }

    size window::min_size() const
    {
        return invoke<&impl::min_size>(m_impl.get());
    }

    position window::position() const
    {
        return invoke<&impl::position>(m_impl.get());
    }

    std::optional<screen> window::screen() const
    {
        return invoke<&impl::screen>(m_impl.get());
    }

    void window::hide()
    {
        return invoke<&impl::hide>(m_impl.get());
    }

    void window::show()
    {
        return invoke<&impl::show>(m_impl.get());
    }

    void window::close()
    {
        return invoke<&impl::close>(m_impl.get());
    }

    void window::focus()
    {
        return invoke<&impl::focus>(m_impl.get());
    }

    void window::start_drag()
    {
        return invoke<&impl::start_drag>(m_impl.get());
    }

    void window::start_resize(edge edge)
    {
        return invoke<&impl::start_resize>(m_impl.get(), edge);
    }

    void window::set_minimized(bool enabled)
    {
        return invoke<&impl::set_minimized>(m_impl.get(), enabled);
    }

    void window::set_maximized(bool enabled)
    {
        return invoke<&impl::set_maximized>(m_impl.get(), enabled);
    }

    void window::set_resizable(bool enabled)
    {
        return invoke<&impl::set_resizable>(m_impl.get(), enabled);
    }

    void window::set_fullscreen(bool enabled)
    {
        return invoke<&impl::set_fullscreen>(m_impl.get(), enabled);
    }

    void window::set_always_on_top(bool enabled)
    {
        return invoke<&impl::set_always_on_top>(m_impl.get(), enabled);
    }

    void window::set_click_through(bool enabled)
    {
        return invoke<&impl::set_click_through>(m_impl.get(), enabled);
    }

    void window::set_icon(const icon &icon)
    {
        return invoke<&impl::set_icon>(m_impl.get(), icon);
    }

    void window::set_title(const std::string &title)
    {
        return invoke<&impl::set_title>(m_impl.get(), title);
    }

    void window::set_background(color background)
    {
        return invoke<&impl::set_background>(m_impl.get(), background);
    }

    void window::set_decorations(decoration decoration)
    {
        return invoke<&impl::set_decorations>(m_impl.get(), decoration);
    }

    void window::set_size(saucer::size size)
    {
        return invoke<&impl::set_size>(m_impl.get(), size);
    }

    void window::set_max_size(saucer::size size)
    {
        return invoke<&impl::set_max_size>(m_impl.get(), size);
    }

    void window::set_min_size(saucer::size size)
    {
        return invoke<&impl::set_min_size>(m_impl.get(), size);
    }

    void window::set_position(saucer::position position)
    {
        return invoke<&impl::set_position>(m_impl.get(), position);
    }

    void window::off(event event)
    {
        return invoke([impl = m_impl.get(), event] { impl->events->clear(event); }, m_impl.get());
    }

    void window::off(event event, std::uint64_t id)
    {
        return invoke([impl = m_impl.get(), event, id] { impl->events->remove(event, id); }, m_impl.get());
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_EVENT);
} // namespace saucer

#include "window.impl.hpp"

#include "error.impl.hpp"

#include "invoke.hpp"
#include "instantiate.hpp"

namespace saucer
{
    window::window(application *app) : m_impl(detail::make_safe<impl>(app))
    {
        m_events = &m_impl->events;
    }

    result<std::shared_ptr<window>> window::create(application *parent)
    {
        if (!parent->thread_safe())
        {
            return parent->invoke(&window::create, parent);
        }

        auto rtn            = std::shared_ptr<window>{new window{parent}};
        rtn->m_impl->parent = parent;

        if (auto status = rtn->m_impl->init_platform(); !status.has_value())
        {
            return err(status);
        }

        return rtn;
    }

    window::~window()
    {
        utils::invoke([impl = m_impl.get()] { impl->events.clear(true); }, m_impl.get());
    }

    template <window::event Event>
    void window::setup()
    {
        return utils::invoke<&impl::setup<Event>>(m_impl.get());
    }

    application &window::parent() const
    {
        return *m_impl->parent;
    }

    bool window::visible() const
    {
        return utils::invoke<&impl::visible>(m_impl.get());
    }

    bool window::focused() const
    {
        return utils::invoke<&impl::focused>(m_impl.get());
    }

    bool window::minimized() const
    {
        return utils::invoke<&impl::minimized>(m_impl.get());
    }

    bool window::maximized() const
    {
        return utils::invoke<&impl::maximized>(m_impl.get());
    }

    bool window::resizable() const
    {
        return utils::invoke<&impl::resizable>(m_impl.get());
    }

    bool window::fullscreen() const
    {
        return utils::invoke<&impl::fullscreen>(m_impl.get());
    }

    bool window::always_on_top() const
    {
        return utils::invoke<&impl::always_on_top>(m_impl.get());
    }

    bool window::click_through() const
    {
        return utils::invoke<&impl::click_through>(m_impl.get());
    }

    std::string window::title() const
    {
        return utils::invoke<&impl::title>(m_impl.get());
    }

    color window::background() const
    {
        return utils::invoke<&impl::background>(m_impl.get());
    }

    window::decoration window::decorations() const
    {
        return utils::invoke<&impl::decorations>(m_impl.get());
    }

    size window::size() const
    {
        return utils::invoke<&impl::size>(m_impl.get());
    }

    size window::max_size() const
    {
        return utils::invoke<&impl::max_size>(m_impl.get());
    }

    size window::min_size() const
    {
        return utils::invoke<&impl::min_size>(m_impl.get());
    }

    position window::position() const
    {
        return utils::invoke<&impl::position>(m_impl.get());
    }

    std::optional<screen> window::screen() const
    {
        return utils::invoke<&impl::screen>(m_impl.get());
    }

    void window::hide()
    {
        return utils::invoke<&impl::hide>(m_impl.get());
    }

    void window::show()
    {
        return utils::invoke<&impl::show>(m_impl.get());
    }

    void window::close()
    {
        return utils::invoke<&impl::close>(m_impl.get());
    }

    void window::focus()
    {
        return utils::invoke<&impl::focus>(m_impl.get());
    }

    void window::start_drag()
    {
        return utils::invoke<&impl::start_drag>(m_impl.get());
    }

    void window::start_resize(edge edge)
    {
        return utils::invoke<&impl::start_resize>(m_impl.get(), edge);
    }

    void window::set_minimized(bool enabled)
    {
        return utils::invoke<&impl::set_minimized>(m_impl.get(), enabled);
    }

    void window::set_maximized(bool enabled)
    {
        return utils::invoke<&impl::set_maximized>(m_impl.get(), enabled);
    }

    void window::set_resizable(bool enabled)
    {
        return utils::invoke<&impl::set_resizable>(m_impl.get(), enabled);
    }

    void window::set_fullscreen(bool enabled)
    {
        return utils::invoke<&impl::set_fullscreen>(m_impl.get(), enabled);
    }

    void window::set_always_on_top(bool enabled)
    {
        return utils::invoke<&impl::set_always_on_top>(m_impl.get(), enabled);
    }

    void window::set_click_through(bool enabled)
    {
        return utils::invoke<&impl::set_click_through>(m_impl.get(), enabled);
    }

    void window::set_icon(const icon &icon)
    {
        return utils::invoke<&impl::set_icon>(m_impl.get(), icon);
    }

    void window::set_title(const std::string &title)
    {
        return utils::invoke<&impl::set_title>(m_impl.get(), title);
    }

    void window::set_background(color background)
    {
        return utils::invoke<&impl::set_background>(m_impl.get(), background);
    }

    void window::set_decorations(decoration decoration)
    {
        return utils::invoke<&impl::set_decorations>(m_impl.get(), decoration);
    }

    void window::set_size(saucer::size size)
    {
        return utils::invoke<&impl::set_size>(m_impl.get(), size);
    }

    void window::set_max_size(saucer::size size)
    {
        return utils::invoke<&impl::set_max_size>(m_impl.get(), size);
    }

    void window::set_min_size(saucer::size size)
    {
        return utils::invoke<&impl::set_min_size>(m_impl.get(), size);
    }

    void window::set_position(saucer::position position)
    {
        return utils::invoke<&impl::set_position>(m_impl.get(), position);
    }

    void window::off(event event)
    {
        return utils::invoke([impl = m_impl.get(), event] { impl->events.clear(event); }, m_impl.get());
    }

    void window::off(event event, std::size_t id)
    {
        return utils::invoke([impl = m_impl.get(), event, id] { impl->events.remove(event, id); }, m_impl.get());
    }

    SAUCER_INSTANTIATE_WINDOW_EVENTS(SAUCER_INSTANTIATE_WINDOW_EVENT);
} // namespace saucer

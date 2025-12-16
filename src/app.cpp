#include "app.impl.hpp"

#include "error.impl.hpp"

namespace saucer
{
    application::application() : m_impl(std::make_unique<impl>())
    {
        m_events = &m_impl->events;
    }

    application::application(application &&) noexcept = default;

    result<application> application::create(const options &opts)
    {
        if (static bool once{false}; once)
        {
            return err(contract_error::instance_exists);
        }
        else
        {
            once = true;
        }

        auto rtn           = application{};
        rtn.m_impl->thread = std::this_thread::get_id();

        if (auto status = rtn.m_impl->init_platform(opts); !status.has_value())
        {
            return err(status);
        }

        return rtn;
    }

    application::~application()
    {
        if (!m_impl)
        {
            return;
        }

        m_events->clear(true);
    }

    bool application::thread_safe() const
    {
        if (!m_impl)
        {
            return {};
        }

        return m_impl->thread == std::this_thread::get_id();
    }

    std::vector<screen> application::screens() const
    {
        if (!m_impl)
        {
            return {};
        }

        return invoke(&impl::screens, m_impl.get());
    }

    int application::run(callback_t callback)
    {
        if (static bool once{false}; once)
        {
            return -1;
        }
        else
        {
            once = true;
        }

        if (!m_impl)
        {
            return -1;
        }

        if (!thread_safe())
        {
            return -1;
        }

        return m_impl->run(this, std::move(callback));
    }

    coco::future<void> application::finish()
    {
        return std::move(m_impl->finish);
    }

    void application::quit()
    {
        if (!m_impl)
        {
            return;
        }

        if (!thread_safe())
        {
            return invoke(&application::quit, this);
        }

        if (m_events->get<event::quit>().fire().find(policy::block))
        {
            return;
        }

        m_impl->quit();
    }

    void application::off(event event)
    {
        if (!m_impl)
        {
            return;
        }

        return invoke([event](auto *impl) { impl->events.clear(event); }, m_impl.get());
    }

    void application::off(event event, std::size_t id)
    {
        if (!m_impl)
        {
            return;
        }

        return invoke([event, id](auto *impl) { impl->events.remove(event, id); }, m_impl.get());
    }
} // namespace saucer

#include "app.impl.hpp"

#include "invoke.hpp"

#include <rebind/enum.hpp>

namespace saucer
{
    application::application() : m_events(std::make_unique<events>()), m_impl(std::make_unique<impl>()) {}

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

        auto rtn = application{};

        rtn.m_impl->thread = std::this_thread::get_id();
        rtn.m_impl->events = rtn.m_events.get();

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

        for (const auto &event : rebind::enum_values<event>)
        {
            m_events->clear(event);
        }
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
        return saucer::invoke<&impl::screens>(this, m_impl.get());
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

        auto quit = [events = m_events.get(), impl = m_impl.get()]()
        {
            if (events->get<event::quit>().fire().find(policy::block))
            {
                return;
            }

            impl->quit();
        };

        return invoke(quit);
    }

    void application::off(event event)
    {
        if (!m_impl)
        {
            return;
        }

        return invoke([impl = m_impl.get(), event] { impl->events->clear(event); });
    }

    void application::off(event event, std::uint64_t id)
    {
        if (!m_impl)
        {
            return;
        }

        return invoke([impl = m_impl.get(), event, id] { impl->events->remove(event, id); });
    }
} // namespace saucer

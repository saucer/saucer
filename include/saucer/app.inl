#pragma once

#include "app.hpp"

namespace saucer
{
    namespace detail
    {
        template <typename T>
        struct safe_delete;

        template <typename T>
        using safe_ptr = std::unique_ptr<T, safe_delete<T>>;

        template <typename T, typename... Ts>
        auto make_safe(application *app, Ts &&...);
    } // namespace detail

    template <typename T>
    struct detail::safe_delete
    {
        const application *app;

      public:
        void operator()(T *ptr) const;
    };

    template <typename T>
    void detail::safe_delete<T>::operator()(T *ptr) const
    {
        if (!app->thread_safe())
        {
            return app->invoke(&safe_delete::operator(), this, ptr);
        }

        delete ptr;
    }

    template <typename T, typename... Ts>
    auto detail::make_safe(application *app, Ts &&...args)
    {
        return safe_ptr<T>(new T{std::forward<Ts>(args)...}, safe_delete<T>(app));
    }

    template <typename Callback, typename... Ts>
    auto application::invoke(Callback &&callback, Ts &&...args) const
    {
        if (thread_safe())
        {
            return std::invoke(std::forward<Callback>(callback), std::forward<Ts>(args)...);
        }

        auto task_callback = [callback = std::forward<Callback>(callback), ... args = std::forward<Ts>(args)]() mutable
        {
            return std::invoke(std::forward<Callback>(callback), std::forward<Ts>(args)...);
        };

        auto task   = std::packaged_task{task_callback};
        auto future = task.get_future();

        post([task = std::move(task)]() mutable { std::invoke(task); });

        return future.get();
    }

    template <application::event Event>
    auto application::on(events::event<Event>::listener listener)
    {
        return m_events->get<Event>().add(std::move(listener));
    }

    template <application::event Event>
    void application::once(events::event<Event>::listener::callback cb)
    {
        return m_events->get<Event>().once(std::move(cb));
    }

    template <application::event Event, typename... Ts>
    auto application::await(Ts &&...result)
    {
        return m_events->get<Event>().await(std::forward<Ts>(result)...);
    }
} // namespace saucer

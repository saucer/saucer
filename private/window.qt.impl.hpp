#pragma once

#include "window.hpp"

#include <optional>

#include <future>
#include <functional>

#include <QMainWindow>
#include <QApplication>

namespace saucer
{
    //! The window should be the first member of the impl struct, as it should
    //! be easily accessible for modules.

    struct window::impl
    {
        class main_window;

      public:
        std::unique_ptr<QMainWindow> window;

      public:
        std::function<void()> on_closed;
        std::optional<QSize> max_size, min_size;

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        template <typename Func>
        auto post_safe(Func &&);

      public:
        static thread_local inline std::unique_ptr<QApplication> application;
    };

    class window::impl::main_window : public QMainWindow
    {
        saucer::window *m_parent;

      public:
        main_window(saucer::window *parent);

      public:
        void changeEvent(QEvent *event) override;

      public:
        void closeEvent(QCloseEvent *event) override;

      public:
        void resizeEvent(QResizeEvent *event) override;
    };

    template <typename T>
    class event_callback : public QEvent
    {
        using callback_t = std::function<T()>;

      private:
        callback_t m_func;
        std::promise<T> *m_result;

      public:
        event_callback(callback_t func, std::promise<T> *result)
            : QEvent(QEvent::User), m_func(std::move(func)), m_result(result)
        {
        }

      public:
        ~event_callback() override
        {
            if constexpr (!std::is_void_v<T>)
            {
                m_result->set_value(std::invoke(m_func));
            }
            else
            {
                std::invoke(m_func);
                m_result->set_value();
            }
        }
    };

    template <typename Func>
    auto window::impl::post_safe(Func &&func)
    {
        using result_t = std::invoke_result_t<Func>;

        std::promise<result_t> result;
        auto *event = new event_callback<result_t>{std::forward<Func>(func), &result};

        // ? postEvent will automatically delete the event after processing.
        QApplication::postEvent(window.get(), event);

        return result.get_future().get();
    }
} // namespace saucer

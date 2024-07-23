#pragma once

#include "window.hpp"

#include <future>
#include <functional>

#include <QMainWindow>
#include <QApplication>

namespace saucer
{
    struct window::impl
    {
        class main_window;

      public:
        std::unique_ptr<QMainWindow> window;

      public:
        QSize max_size, min_size;
        std::function<void()> on_closed;

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        template <typename Func, typename Result = std::invoke_result_t<Func>>
        Result post_safe(Func &&);

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
    class safe_event : public QEvent
    {
        using callback_t = std::function<T()>;

      private:
        callback_t m_func;
        std::promise<T> *m_result;

      public:
        safe_event(callback_t func, std::promise<T> *result)
            : QEvent(QEvent::User), m_func(std::move(func)), m_result(result)
        {
        }

      public:
        ~safe_event() override
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

    template <typename Func, typename Result>
    Result window::impl::post_safe(Func &&func)
    {
        std::promise<Result> result;
        auto *event = new safe_event<Result>{std::forward<Func>(func), &result};

        // ? Qt will automatically delete the event after processing.
        QApplication::postEvent(window.get(), event);

        return result.get_future().get();
    }
} // namespace saucer

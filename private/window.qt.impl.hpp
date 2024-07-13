#pragma once

#include "window.hpp"

#include <future>
#include <optional>
#include <functional>

#include <QMainWindow>
#include <QCloseEvent>
#include <QApplication>

namespace saucer
{
    //! The window should be the first member of the impl struct, as it should
    //! be easily accessible for modules.

    struct window::impl
    {
        class main_window;

      public:
        QMainWindow *window;

      public:
        std::function<void()> on_closed;
        std::optional<QSize> max_size, min_size;

      public:
        [[nodiscard]] bool is_thread_safe() const;

      public:
        template <typename Func>
        auto post_safe(Func &&);
    };

    class window::impl::main_window : public QMainWindow
    {
        class window *m_parent;

      public:
        main_window(class window *parent);

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
        event_callback(callback_t &&func, std::promise<T> *result)
            : QEvent(QEvent::None), m_func(std::move(func)), m_result(result)
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
        using return_t = typename decltype(std::function{func})::result_type;

        std::promise<return_t> result;
        auto *event = new event_callback<return_t>(std::forward<Func>(func), &result);

        // ? postEvent will automatically delete the event after processing.
        QApplication::postEvent(window, event);

        return result.get_future().get();
    }
} // namespace saucer

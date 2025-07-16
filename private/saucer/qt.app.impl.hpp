#pragma once

#include "app.impl.hpp"

#include <QEvent>
#include <QScreen>
#include <QApplication>

namespace saucer
{
    struct application::impl::native
    {
        std::unique_ptr<QApplication> application;

      public:
        static inline std::string id;
        static inline int argc;
        static inline std::vector<char *> argv;

      public:
        static void iteration();
        static screen convert(QScreen *);
    };

    class safe_event : public QEvent
    {
        using callback_t = std::move_only_function<void()>;

      private:
        callback_t m_callback;

      public:
        safe_event(callback_t callback);

      public:
        ~safe_event() override;
    };
} // namespace saucer

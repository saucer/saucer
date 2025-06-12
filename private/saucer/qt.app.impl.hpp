#pragma once

#include "app.hpp"

#include <QEvent>
#include <QScreen>
#include <QApplication>

namespace saucer
{
    struct application::impl
    {
        std::unique_ptr<QApplication> application;

      public:
        coco::future<void> future;

      public:
        static void iteration();
        static screen convert(QScreen *);

      public:
        static inline std::string id;

      public:
        static inline int argc;
        static inline std::vector<char *> argv;
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

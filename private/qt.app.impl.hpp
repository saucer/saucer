#pragma once

#include "app.hpp"

#include <QEvent>
#include <QApplication>

namespace saucer
{
    struct application::impl
    {
        std::unique_ptr<QApplication> application;

      public:
        std::string id;

      public:
        int argc;
        std::vector<char *> argv;
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

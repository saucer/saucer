#pragma once

#include "app.hpp"

#include <vector>
#include <unordered_map>

#include <QEvent>
#include <QScreen>
#include <QApplication>

namespace saucer
{
    struct application::impl
    {
        std::unique_ptr<QApplication> application;
        std::unique_ptr<QEventLoop> loop;

      public:
        std::unordered_map<void *, bool> instances;

      public:
        std::string id;

      public:
        int argc;
        std::vector<char *> argv;

      public:
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

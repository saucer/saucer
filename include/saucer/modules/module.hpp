#pragma once
#include <memory>
#include <string>

namespace saucer
{
    class smartview;
    class module
    {
        std::string m_name;
        std::string m_version;

      protected:
        smartview &m_smartview;

      public:
        virtual ~module();
        module(std::string &&name, std::string &&version, smartview &);

      public:
        std::string get_name() const;
        std::string get_version() const;
        virtual void on_message(const std::string &) = 0;
        virtual void on_url_changed(const std::string &) = 0;
    };
} // namespace saucer
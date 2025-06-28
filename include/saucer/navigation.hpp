#pragma once

#include <memory>
#include <string>

namespace saucer
{
    class navigation
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        navigation(impl);

      public:
        navigation(const navigation &);

      public:
        ~navigation();

      public:
        [[nodiscard]] std::string url() const;

      public:
        [[nodiscard]] bool new_window() const;
        [[nodiscard]] bool redirection() const;
        [[nodiscard]] bool user_initiated() const;
    };
} // namespace saucer

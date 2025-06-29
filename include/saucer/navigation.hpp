#pragma once

#include "uri.hpp"

#include <memory>

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
        [[nodiscard]] uri url() const;

      public:
        [[nodiscard]] bool new_window() const;
        [[nodiscard]] bool redirection() const;
        [[nodiscard]] bool user_initiated() const;
    };
} // namespace saucer

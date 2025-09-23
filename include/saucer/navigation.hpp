#pragma once

#include "url.hpp"

#include <memory>

namespace saucer
{
    struct navigation
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        navigation(impl);

      public:
        ~navigation();

      public:
        [[nodiscard]] saucer::url url() const;

      public:
        [[nodiscard]] bool new_window() const;
        [[nodiscard]] bool redirection() const;
        [[nodiscard]] bool user_initiated() const;
    };
} // namespace saucer

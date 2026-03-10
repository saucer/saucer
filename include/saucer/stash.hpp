#pragma once

#include "modules/module.hpp"

#include <memory>
#include <cstdint>

#include <span>
#include <string_view>

#include <vector>
#include <functional>

namespace saucer
{
    struct stash
    {
        struct impl;

      public:
        using vec  = std::vector<std::uint8_t>;
        using span = std::span<const std::uint8_t>;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        stash(std::unique_ptr<impl>);

      public:
        stash(const stash &);
        stash(stash &&) noexcept;

      public:
        ~stash();

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<stash, Stable> native() const;

      public:
        [[nodiscard]] span data() const;
        [[nodiscard]] std::string_view str() const;

      public:
        [[nodiscard]] static stash from(vec);
        [[nodiscard]] static stash view(span);
        [[nodiscard]] static stash lazy(std::function<stash()>);

      public:
        [[nodiscard]] static stash from_str(std::string_view);
        [[nodiscard]] static stash view_str(std::string_view);

      public:
        [[nodiscard]] static stash empty();
    };
} // namespace saucer

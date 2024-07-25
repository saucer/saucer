#pragma once

#include "stash/stash.hpp"

#include <memory>
#include <optional>
#include <filesystem>

namespace saucer
{
    namespace fs = std::filesystem;

    class icon
    {
        friend class window;
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        icon();
        icon(impl);

      public:
        icon(const icon &);
        icon(icon &&) noexcept;

      public:
        ~icon();

      public:
        icon &operator=(const icon &);
        icon &operator=(icon &&) noexcept;

      public:
        [[nodiscard]] bool empty() const;

      public:
        [[nodiscard]] static std::optional<icon> from(const fs::path &file);
        [[nodiscard]] static std::optional<icon> from(const stash<const std::uint8_t> &ico);
    };
} // namespace saucer

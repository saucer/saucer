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
        friend struct window;
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
        icon &operator=(icon) noexcept;
        friend void swap(icon &, icon &) noexcept;

      public:
        [[nodiscard]] bool empty() const;
        [[nodiscard]] stash<> data() const;

      public:
        void save(const fs::path &path) const;

      public:
        [[nodiscard]] static std::optional<icon> from(const stash<> &ico);
        [[nodiscard]] static std::optional<icon> from(const fs::path &file);
    };
} // namespace saucer

#pragma once

#include "modules/module.hpp"

#include "error/error.hpp"
#include "stash/stash.hpp"

#include <memory>
#include <filesystem>

namespace saucer
{
    namespace fs = std::filesystem;

    struct icon
    {
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
        template <bool Stable = true>
        [[nodiscard]] natives<icon, Stable> native() const;

      public:
        [[nodiscard]] bool empty() const;
        [[nodiscard]] stash<> data() const;

      public:
        void save(const fs::path &path) const;

      public:
        [[nodiscard]] static result<icon> from(const stash<> &ico);
        [[nodiscard]] static result<icon> from(const fs::path &file);
    };
} // namespace saucer

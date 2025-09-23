#pragma once

#include "error/error.hpp"
#include "modules/module.hpp"

#include <string>
#include <filesystem>

#include <memory>
#include <optional>

namespace saucer
{
    namespace fs = std::filesystem;

    struct url
    {
        struct impl;
        struct options;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        url();
        url(impl);

      public:
        url(const url &);
        url(url &&) noexcept;

      public:
        ~url();

      public:
        url &operator=(url) noexcept;
        friend void swap(url &, url &) noexcept;

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<url, Stable> native() const;

      public:
        [[nodiscard]] std::string string() const;

      public:
        [[nodiscard]] fs::path path() const;
        [[nodiscard]] std::string scheme() const;

      public:
        [[nodiscard]] std::optional<std::string> host() const;
        [[nodiscard]] std::optional<std::size_t> port() const;

      public:
        [[nodiscard]] std::optional<std::string> user() const;
        [[nodiscard]] std::optional<std::string> password() const;

      public:
        static result<url> from(const fs::path &file);
        static result<url> parse(const std::string &input);

      public:
        static url make(const options &);
    }; // namespace std::filesystem

    struct url::options
    {
        std::string scheme;

      public:
        std::optional<std::string> host;
        std::optional<std::size_t> port;

      public:
        fs::path path;
    };
} // namespace saucer

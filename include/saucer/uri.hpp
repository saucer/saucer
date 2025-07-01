#pragma once

#include "modules/module.hpp"

#include <string>
#include <filesystem>

#include <memory>
#include <optional>

namespace saucer
{
    namespace fs = std::filesystem;

    struct uri
    {
        struct impl;
        struct options;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        uri();
        uri(impl);

      public:
        uri(const uri &);
        uri(uri &&) noexcept;

      public:
        ~uri();

      public:
        uri &operator=(uri) noexcept;
        friend void swap(uri &, uri &) noexcept;

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<uri, Stable> native() const;

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
        static std::optional<uri> from(const fs::path &file);
        static std::optional<uri> parse(const std::string &input);

      public:
        static uri make(const options &);
    }; // namespace std::filesystem

    struct saucer::uri::options
    {
        std::string scheme;

      public:
        std::optional<std::string> host;
        std::optional<std::size_t> port;

      public:
        fs::path path;
    };
} // namespace saucer

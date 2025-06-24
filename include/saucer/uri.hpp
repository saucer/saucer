#pragma once

#include <string>
#include <filesystem>

#include <memory>
#include <optional>

namespace saucer
{
    namespace fs = std::filesystem;

    struct uri
    {
        friend struct webview;
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
        uri &operator=(const uri &);
        uri &operator=(uri &&) noexcept;

      public:
        [[nodiscard]] std::string string() const;

      public:
        [[nodiscard]] std::optional<std::string> host() const;
        [[nodiscard]] std::optional<std::string> query() const;

      public:
      public:
        [[nodiscard]] fs::path path() const;
        [[nodiscard]] std::string scheme() const;

      public:
        [[nodiscard]] std::optional<std::size_t> port() const;

      public:
        static std::optional<uri> from(const fs::path &file);
        static std::optional<uri> parse(const std::string &input);

      public:
        static uri make(const options &);
    }; // namespace std::filesystem

    struct saucer::uri::options
    {
        fs::path path;
        std::string scheme;

      public:
        std::optional<std::string> host;
        std::optional<std::string> query;

      public:
        std::optional<std::size_t> port;
    };
} // namespace saucer

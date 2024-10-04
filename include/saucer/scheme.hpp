#pragma once

#include "stash/stash.hpp"

#include <map>
#include <string>

#include <memory>
#include <expected>
#include <functional>

namespace saucer::scheme
{
    enum class error
    {
        not_found,
        invalid,
        aborted,
        denied,
        failed,
    };

    struct response
    {
        stash<> data;
        std::string mime;
        std::map<std::string, std::string> headers;

      public:
        int status{200};
    };

    class request
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        request(impl);

      public:
        ~request();

      public:
        [[nodiscard]] std::string url() const;
        [[nodiscard]] std::string method() const;

      public:
        [[nodiscard]] stash<> content() const;
        [[nodiscard]] std::map<std::string, std::string> headers() const;
    };

    using handler = std::move_only_function<std::expected<response, error>(const request &)>;
} // namespace saucer::scheme

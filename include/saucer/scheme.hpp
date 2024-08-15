#pragma once

#include "stash/stash.hpp"

#include <map>
#include <string>

#include <memory>
#include <functional>

#include <tl/expected.hpp>

namespace saucer
{
    enum class request_error
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

    using scheme_handler = std::move_only_function<tl::expected<response, request_error>(const request &)>;
} // namespace saucer

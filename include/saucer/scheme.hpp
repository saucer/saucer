#pragma once

#include "uri.hpp"
#include "executor.hpp"
#include "stash/stash.hpp"

#include <memory>
#include <cstdint>

#include <map>
#include <string>

namespace saucer::scheme
{
    enum class error : std::uint8_t
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
        request(const request &);
        request(request &&) noexcept;

      public:
        ~request();

      public:
        [[nodiscard]] uri url() const;
        [[nodiscard]] std::string method() const;

      public:
        [[nodiscard]] stash<> content() const;
        [[nodiscard]] std::map<std::string, std::string> headers() const;
    };

    using executor = saucer::executor<response, error>;
    using resolver = std::function<void(request, executor)>;
} // namespace saucer::scheme

#pragma once

#include "url.hpp"
#include "executor.hpp"
#include "stash/stash.hpp"

#include <memory>
#include <cstdint>

#include <map>
#include <string>

namespace saucer::scheme
{
    enum class error : std::int16_t
    {
        not_found = 404,
        invalid   = 400,
        denied    = 401,
        failed    = -1,
    };

    struct response
    {
        stash<> data;
        std::string mime;
        std::map<std::string, std::string> headers;

      public:
        int status{200};
    };

    struct request
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        request(impl);

      public:
        request(const request &);

      public:
        ~request();

      public:
        [[nodiscard]] saucer::url url() const;
        [[nodiscard]] std::string method() const;

      public:
        [[nodiscard]] stash<> content() const;
        [[nodiscard]] std::map<std::string, std::string> headers() const;
    };

    using executor = saucer::executor<response, error>;
    using resolver = std::function<void(const request &, const executor &)>;
} // namespace saucer::scheme

#pragma once

#include <string>
#include <memory>

#include <cstdint>

namespace saucer::permission
{
    enum class type : std::uint8_t
    {
        clipboard,
        device_info,
        location,
        media,
        notification,
        pointer,
        devices,
        third_party_cookies,
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
        [[nodiscard]] std::string url() const;
        [[nodiscard]] permission::type type() const;

      public:
        void accept(bool) const;
    };
} // namespace saucer::permission

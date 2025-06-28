#pragma once

#include <string>
#include <memory>

#include <cstdint>

namespace saucer::permission
{
    enum class type : std::uint8_t
    {
        audio_media,
        video_media,
        desktop_media,
        mouse_lock,
        device_info,
        location,
        clipboard,
        notification,
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

      public:
        ~request();

      public:
        [[nodiscard]] std::string url() const;
        [[nodiscard]] permission::type type() const;

      public:
        void accept(bool) const;
    };
} // namespace saucer::permission

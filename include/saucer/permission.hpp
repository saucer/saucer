#pragma once

#include "modules/module.hpp"
#include "url.hpp"

#include <memory>
#include <cstdint>

namespace saucer::permission
{
    enum class type : std::uint8_t
    {
        unknown       = 0,
        audio_media   = 1 << 0,
        video_media   = 1 << 1,
        desktop_media = 1 << 2,
        mouse_lock    = 1 << 3,
        device_info   = 1 << 4,
        location      = 1 << 5,
        clipboard     = 1 << 6,
        notification  = 1 << 7,
    };

    struct request
    {
        struct impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        request(impl);

      public:
        ~request();

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<request, Stable> native() const;

      public:
        [[nodiscard]] saucer::url url() const;
        [[nodiscard]] permission::type type() const;

      public:
        void accept(bool) const;
    };
} // namespace saucer::permission

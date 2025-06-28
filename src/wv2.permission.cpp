#include "wv2.permission.impl.hpp"

#include "win32.utils.hpp"

#include <cassert>

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    std::string request::url() const
    {
        utils::string_handle raw;
        m_impl->request->get_Uri(&raw.reset());

        return utils::narrow(raw.get());
    }

    permission::type request::type() const
    {
        using enum permission::type;

        COREWEBVIEW2_PERMISSION_KIND type{};
        m_impl->request->get_PermissionKind(&type);

        switch (type)
        {
        case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:
            return audio_media;
        case COREWEBVIEW2_PERMISSION_KIND_CAMERA:
            return video_media;
        case COREWEBVIEW2_PERMISSION_KIND_LOCAL_FONTS:
            return device_info;
        case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:
            return location;
        case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:
            return clipboard;
        case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:
            return notification;
        default:
            return static_cast<permission::type>(std::to_underlying(unknown) + type);
        }
    }

    void request::accept(bool value) const
    {
        m_impl->request->put_State(value ? COREWEBVIEW2_PERMISSION_STATE_ALLOW : COREWEBVIEW2_PERMISSION_STATE_DENY);
        m_impl->deferral->Complete();
    }
} // namespace saucer::permission

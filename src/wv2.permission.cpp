#include "wv2.permission.impl.hpp"

#include "win32.utils.hpp"

#include <cassert>

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::~request()
    {
        accept(false);
    }

    uri request::url() const
    {
        utils::string_handle raw;
        m_impl->request->get_Uri(&raw.reset());
        return uri::parse(utils::narrow(raw.get())).value_or({});
    }

    permission::type request::type() const
    {
        COREWEBVIEW2_PERMISSION_KIND type{};
        m_impl->request->get_PermissionKind(&type);

        switch (type)
        {
            using enum permission::type;

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
            return unknown;
        }
    }

    void request::accept(bool value) const
    {
        if (!m_impl->deferral)
        {
            return;
        }

        auto deferral = std::move(m_impl->deferral);
        {
            m_impl->request->put_State(value ? COREWEBVIEW2_PERMISSION_STATE_ALLOW : COREWEBVIEW2_PERMISSION_STATE_DENY);
        }
        deferral->Complete();
    }
} // namespace saucer::permission

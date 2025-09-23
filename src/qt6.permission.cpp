#include "qt.permission.impl.hpp"

#include "qt.url.impl.hpp"

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::permission::type> = true;

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::~request()
    {
        accept(false);
    }

    url request::url() const
    {
        return url::impl{m_impl->origin};
    }

    permission::type request::type() const
    {
        switch (m_impl->type)
        {
            using enum permission::type;
            using enum QWebEnginePermission::PermissionType;

        case ClipboardReadWrite:
            return clipboard;
        case LocalFontsAccess:
            return device_info;
        case Geolocation:
            return location;
        case Notifications:
            return notification;
        case MouseLock:
            return mouse_lock;
        case MediaAudioCapture:
            return audio_media;
        case MediaVideoCapture:
            return video_media;
        case MediaAudioVideoCapture:
            return audio_media | video_media;
        case DesktopVideoCapture:
        case DesktopAudioVideoCapture:
            return desktop_media;
        default:
            return unknown;
        }
    }

    void request::accept(bool value) const
    {
        if (!m_impl->request.isValid())
        {
            return;
        }

        auto request = std::move(m_impl->request);

        (request.*(value ? &QWebEnginePermission::grant : &QWebEnginePermission::deny))();
    }
} // namespace saucer::permission

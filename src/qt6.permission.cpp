#include "qt.permission.impl.hpp"

#include "qt.uri.impl.hpp"

#include <flagpp/flags.hpp>

template <>
constexpr bool flagpp::enabled<saucer::permission::type> = true;

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    uri request::url() const
    {
        return uri::impl{m_impl->request.origin()};
    }

    permission::type request::type() const
    {
        const auto type = m_impl->request.permissionType();

        switch (type)
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
        case Unsupported:
            return unknown;
        }
    }

    void request::accept(bool value) const
    {
        (m_impl->request.*(value ? &QWebEnginePermission::grant : &QWebEnginePermission::deny))();
    }
} // namespace saucer::permission

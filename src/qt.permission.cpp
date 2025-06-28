#include "qt.permission.impl.hpp"

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    std::string request::url() const
    {
        return m_impl->request.origin().toString().toStdString();
    }

    permission::type request::type() const
    {
        using enum permission::type;
        using enum QWebEnginePermission::PermissionType;

        const auto type = m_impl->request.permissionType();

        switch (type)
        {
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
        case DesktopVideoCapture:
        case DesktopAudioVideoCapture:
            return desktop_media;
        default:
            return static_cast<permission::type>(std::to_underlying(unknown) + std::to_underlying(m_impl->request.permissionType()));
        }
    }

    void request::accept(bool value) const
    {
        (m_impl->request.*(value ? &QWebEnginePermission::grant : &QWebEnginePermission::deny))();
    }
} // namespace saucer::permission

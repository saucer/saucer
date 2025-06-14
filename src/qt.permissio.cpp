#include "qt.permission.impl.hpp"

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    request::request(request &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    request::~request() = default;

    std::string request::url() const
    {
        return m_impl->request.origin().toString().toStdString();
    }

    permission::type request::type() const
    {
        using enum permission::type;
        using enum QWebEnginePermission::PermissionType;

        switch (m_impl->request.permissionType())
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
            return pointer;
        case MediaAudioCapture:
        case MediaVideoCapture:
        case MediaAudioVideoCapture:
        case DesktopVideoCapture:
        case DesktopAudioVideoCapture:
            return devices;
        case Unsupported:
            assert(false && "Unsupported permission type");
        }
    }

    void request::accept(bool value) const
    {
        (m_impl->request.*(value ? &QWebEnginePermission::grant : &QWebEnginePermission::deny))();
    }
} // namespace saucer::permission

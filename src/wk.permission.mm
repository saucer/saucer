#include "wk.permission.impl.hpp"

#include "wk.url.impl.hpp"

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
        const utils::autorelease_guard guard{};
        return url::impl{[m_impl->frame.get().webView.URL copy]};
    }

    permission::type request::type() const
    {
        using enum permission::type;

        switch (m_impl->type)
        {
        case WKMediaCaptureTypeCamera:
            return video_media;
        case WKMediaCaptureTypeMicrophone:
            return audio_media;
        case WKMediaCaptureTypeCameraAndMicrophone:
            return audio_media | video_media;
        default:
            return unknown;
        }
    }

    void request::accept(bool value) const
    {
        if (!m_impl->handler)
        {
            return;
        }

        auto handler = std::move(m_impl->handler);

        (handler.get())(value ? WKPermissionDecisionGrant : WKPermissionDecisionDeny);
    }
} // namespace saucer::permission

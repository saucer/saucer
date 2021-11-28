#include <bridge/bridge.hpp>

namespace saucer
{
    bridge::~bridge() = default;
    bridge::bridge(webview &webview) : m_webview(webview) {}
} // namespace saucer
#include <module/module.hpp>

namespace saucer
{
    void module::on_url_changed(const std::string &) {}

    module::~module() = default;
    module::module(webview &webview) : m_webview(webview) {}
} // namespace saucer
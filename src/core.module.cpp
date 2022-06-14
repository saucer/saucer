#include "modules/core.module.hpp"
#include "serializers/json.hpp"
#include "smartview.hpp"

namespace saucer
{
    core_module::~core_module() = default;
    core_module::core_module(smartview &smartview) : module("core_module", "1.0.0", smartview)
    {
        using serializers::json;

        smartview.expose<json>("core.hide", [this] { m_smartview.hide(); });
        smartview.expose<json>("core.show", [this] { m_smartview.show(); });
        smartview.expose<json>("core.close", [this] { m_smartview.close(); });

        smartview.expose<json>("core.get_url", [this] { return m_smartview.get_url(); });
        smartview.expose<json>("core.set_url", [this](const std::string &url) { m_smartview.set_url(url); });

        smartview.expose<json>("core.get_dev_tools", [this] { return m_smartview.get_dev_tools(); });
        smartview.expose<json>("core.set_dev_tools", [this](bool enabled) { m_smartview.set_dev_tools(enabled); });

        smartview.expose<json>("core.get_title", [this] { return m_smartview.get_title(); });
        smartview.expose<json>("core.set_title", [this](const std::string &title) { m_smartview.set_title(title); });

        smartview.expose<json>("core.get_resizable", [this] { return m_smartview.get_resizable(); });
        smartview.expose<json>("core.set_resizable", [this](bool enabled) { m_smartview.set_resizable(enabled); });

        smartview.expose<json>("core.get_decorations", [this] { return m_smartview.get_decorations(); });
        smartview.expose<json>("core.set_decorations", [this](bool enabled) { m_smartview.set_decorations(enabled); });

        smartview.expose<json>("core.get_context_menu", [this] { return m_smartview.get_context_menu(); });
        smartview.expose<json>("core.set_context_menu", [this](bool enabled) { m_smartview.set_context_menu(enabled); });

        smartview.expose<json>("core.get_always_on_top", [this] { return m_smartview.get_always_on_top(); });
        smartview.expose<json>("core.set_always_on_top", [this](bool enabled) { m_smartview.set_always_on_top(enabled); });

        smartview.expose<json>("core.get_size", [this] { return m_smartview.get_size(); });
        smartview.expose<json>("core.set_size", [this](std::size_t width, std::size_t height) { m_smartview.set_size(width, height); });

        smartview.expose<json>("core.get_min_size", [this] { return m_smartview.get_min_size(); });
        smartview.expose<json>("core.set_min_size", [this](std::size_t width, std::size_t height) { m_smartview.set_min_size(width, height); });

        smartview.expose<json>("core.get_max_size", [this] { return m_smartview.get_max_size(); });
        smartview.expose<json>("core.set_max_size", [this](std::size_t width, std::size_t height) { m_smartview.set_max_size(width, height); });

        smartview.expose<json>("core.is_module_loaded", [this](const std::string &module) { return static_cast<bool>(m_smartview.get_module(module)); });
        smartview.expose<json>("core.inject", [this](const std::string &java_script, size_t load_time) { m_smartview.inject(java_script, static_cast<enum load_time>(load_time)); });
    }

    void core_module::on_message(const std::string &) {}
    void core_module::on_url_changed(const std::string &) {}
} // namespace saucer
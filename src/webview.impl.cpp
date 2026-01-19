#include "webview.impl.hpp"

#include "scripts.hpp"
#include "request.hpp"

namespace saucer
{
    using impl = webview::impl;

    status impl::on_message(std::string_view message)
    {
        if (!attributes)
        {
            return status::unhandled;
        }

        auto request = request::parse(message);

        if (!request.has_value())
        {
            return status::unhandled;
        }

        overload visitor = {
            [this](const request::start_resize &data) { window->start_resize(static_cast<window::edge>(data.edge)); },
            [this](const request::start_drag &) { window->start_drag(); },
            [this](const request::maximize &data) { window->set_maximized(data.value); },
            [this](const request::minimize &data) { window->set_minimized(data.value); },
            [this](const request::close &) { window->close(); },
            [this](const request::maximized &data) { resolve(data.id, std::format("{}", window->maximized())); },
            [this](const request::minimized &data) { resolve(data.id, std::format("{}", window->minimized())); },
        };

        std::visit(visitor, *request);

        return status::handled;
    }

    std::string impl::attribute_script()
    {
        static const auto rtn = std::format(scripts::attribute_script, request::stubs());
        return rtn;
    }
} // namespace saucer

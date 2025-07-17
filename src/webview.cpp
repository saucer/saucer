#include "webview.impl.hpp"

#include "window.impl.hpp"
#include "instantiate.hpp"

#include <format>
#include <algorithm>
#include <functional>

#include <rebind/enum.hpp>

namespace saucer
{
    using impl = webview::impl;

    webview::webview() : m_events(std::make_unique<events>()), m_impl(std::make_unique<impl>()) {}

    webview::webview(webview &&other) noexcept = default;

    std::optional<webview> webview::create(const options &opts)
    {
        auto window = opts.window.value();

        if (!window)
        {
            return {};
        }

        auto *const parent = window->native<false>()->parent;

        if (!parent->thread_safe())
        {
            return {};
        }

        auto rtn         = webview{};
        auto *const impl = rtn.m_impl.get();

        impl->window     = opts.window.value();
        impl->parent     = parent;
        impl->events     = rtn.m_events.get();
        impl->attributes = opts.attributes;

        if (!impl->init_platform(opts))
        {
            return {};
        }

        rtn.on<event::message>({{.func = std::bind_front(&impl::on_message, impl), .clearable = false}});

        rtn.inject({.code = impl::creation_script(), .time = load_time::creation, .clearable = false});
        rtn.inject({.code = impl::ready_script(), .time = load_time::ready, .clearable = false});

        if (opts.attributes)
        {
            rtn.inject({.code = impl::attribute_script(), .time = load_time::creation, .clearable = false});
        }

        return rtn;
    }

    webview::~webview()
    {
        if (!m_impl)
        {
            return;
        }

        for (const auto &event : rebind::enum_values<event>)
        {
            m_events->clear(event);
        }
    }

    template <webview::event Event>
    void webview::setup()
    {
        return invoke(m_impl.get(), &impl::setup<Event>);
    }

    void webview::handle_scheme(const std::string &name, scheme::resolver &&handler)
    {
        return invoke(m_impl.get(), &impl::handle_scheme, name, std::move(handler));
    }

    void impl::reject(std::uint64_t id, std::string_view reason)
    {
        execute(std::format(
            R"(
                window.saucer.internal.rpc[{0}].reject({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, reason));
    }

    void impl::resolve(std::uint64_t id, std::string_view result)
    {
        execute(std::format(
            R"(
                window.saucer.internal.rpc[{0}].resolve({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, result));
    }

    window &webview::parent() const
    {
        return *m_impl->window;
    }

    icon webview::favicon() const
    {
        return invoke(m_impl.get(), &impl::favicon);
    }

    std::string webview::page_title() const
    {
        return invoke(m_impl.get(), &impl::page_title);
    }

    bool webview::dev_tools() const
    {
        return invoke(m_impl.get(), &impl::dev_tools);
    }

    bool webview::context_menu() const
    {
        return invoke(m_impl.get(), &impl::context_menu);
    }

    std::optional<uri> webview::url() const
    {
        return invoke(m_impl.get(), &impl::url);
    }

    color webview::background() const
    {
        return invoke(m_impl.get(), &impl::background);
    }

    bool webview::force_dark_mode() const
    {
        return invoke(m_impl.get(), &impl::force_dark_mode);
    }

    void webview::set_dev_tools(bool value)
    {
        return invoke(m_impl.get(), &impl::set_dev_tools, value);
    }

    void webview::set_context_menu(bool value)
    {
        return invoke(m_impl.get(), &impl::set_context_menu, value);
    }

    void webview::set_force_dark_mode(bool value)
    {
        return invoke(m_impl.get(), &impl::set_force_dark_mode, value);
    }

    void webview::set_background(const color &color)
    {
        return invoke(m_impl.get(), &impl::set_background, color);
    }

    void webview::set_url(const uri &url)
    {
        return invoke(m_impl.get(), &impl::set_url, url);
    }

    void webview::set_url(const std::string &url)
    {
        auto uri = saucer::uri::parse(url);

        if (!uri.has_value())
        {
            return;
        }

        set_url(uri.value());
    }

    void webview::back()
    {
        return invoke(m_impl.get(), &impl::back);
    }

    void webview::forward()
    {
        return invoke(m_impl.get(), &impl::forward);
    }

    void webview::reload()
    {
        return invoke(m_impl.get(), &impl::reload);
    }

    void webview::serve(fs::path file)
    {
        return set_url(uri::make({.scheme = "saucer", .host = "embedded", .path = std::move(file)}));
    }

    void webview::embed(embedded_files files)
    {
        auto handler = [impl = m_impl.get()](const scheme::request &request, const scheme::executor &exec)
        {
            const auto &[resolve, reject] = exec;
            auto url                      = request.url();

            if (url.scheme() != "saucer" || url.host() != "embedded")
            {
                return reject(scheme::error::invalid);
            }

            auto file = url.path();

            if (!impl->embedded.contains(file))
            {
                return reject(scheme::error::not_found);
            }

            const auto &data = impl->embedded.at(file);

            return resolve({
                .data    = data.content,
                .mime    = data.mime,
                .headers = {{"Access-Control-Allow-Origin", "*"}},
            });
        };

        auto embed = [impl = m_impl.get()](auto files, auto handler)
        {
            impl->embedded.merge(std::move(files));
            impl->handle_scheme("saucer", std::move(handler));
        };

        return invoke(m_impl.get(), embed, std::move(files), std::move(handler));
    }

    void webview::unembed()
    {
        auto unembed = [impl = m_impl.get()]
        {
            impl->embedded.clear();
            impl->remove_scheme("saucer");
        };

        return invoke(m_impl.get(), std::move(unembed));
    }

    void webview::unembed(const fs::path &file)
    {
        return invoke(m_impl.get(), [impl = m_impl.get(), file] { impl->embedded.erase(file); });
    }

    void webview::execute(const std::string &code)
    {
        return invoke(m_impl.get(), &impl::execute, code);
    }

    std::uint64_t webview::inject(const script &script)
    {
        return invoke(m_impl.get(), &impl::inject, script);
    }

    void webview::uninject()
    {
        auto uninject = static_cast<void (impl::*)()>(&impl::uninject);
        return invoke(m_impl.get(), uninject);
    }

    void webview::uninject(std::uint64_t id)
    {
        auto uninject = static_cast<void (impl::*)(std::uint64_t)>(&impl::uninject);
        return invoke(m_impl.get(), uninject, id);
    }

    void webview::remove_scheme(const std::string &name)
    {
        return invoke(m_impl.get(), &impl::remove_scheme, name);
    }

    void webview::off(event event)
    {
        return invoke(m_impl.get(), [impl = m_impl.get(), event] { impl->events->clear(event); });
    }

    void webview::off(event event, std::uint64_t id)
    {
        return invoke(m_impl.get(), [impl = m_impl.get(), event, id] { impl->events->remove(event, id); });
    }

    void webview::register_scheme(const std::string &name)
    {
        return impl::register_scheme(name);
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_EVENT);
} // namespace saucer

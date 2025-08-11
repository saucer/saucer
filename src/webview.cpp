#include "webview.impl.hpp"

#include "invoke.hpp"
#include "instantiate.hpp"

#include "window.impl.hpp"

#include <format>
#include <algorithm>
#include <functional>

#include <rebind/enum.hpp>

namespace saucer
{
    using impl = webview::impl;

    webview::webview() : m_events(std::make_unique<events>()), m_impl(std::make_unique<impl>()) {}

    webview::webview(webview &&other) noexcept = default;

    result<webview> webview::create(const options &opts)
    {
        auto window = opts.window.value();

        if (!window)
        {
            return err(contract_error::required_invalid);
        }

        auto *const parent = window->native<false>()->parent;

        if (!parent->thread_safe())
        {
            return err(contract_error::not_main_thread);
        }

        if (static auto once{true}; once)
        {
            register_scheme("saucer");
            once = false;
        }

        auto rtn         = webview{};
        auto *const impl = rtn.m_impl.get();

        impl->window     = opts.window.value();
        impl->parent     = parent;
        impl->events     = rtn.m_events.get();
        impl->attributes = opts.attributes;

        if (auto status = impl->init_platform(opts); !status.has_value())
        {
            return err(status);
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
        return invoke<&impl::setup<Event>>(m_impl.get());
    }

    void webview::handle_scheme(const std::string &name, scheme::resolver &&handler)
    {
        return invoke<&impl::handle_scheme>(m_impl.get(), name, std::move(handler));
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
        return invoke<&impl::favicon>(m_impl.get());
    }

    std::string webview::page_title() const
    {
        return invoke<&impl::page_title>(m_impl.get());
    }

    bool webview::dev_tools() const
    {
        return invoke<&impl::dev_tools>(m_impl.get());
    }

    bool webview::context_menu() const
    {
        return invoke<&impl::context_menu>(m_impl.get());
    }

    std::optional<uri> webview::url() const
    {
        return invoke<&impl::url>(m_impl.get());
    }

    color webview::background() const
    {
        return invoke<&impl::background>(m_impl.get());
    }

    bool webview::force_dark_mode() const
    {
        return invoke<&impl::force_dark_mode>(m_impl.get());
    }

    bounds webview::bounds() const
    {
        return invoke<&impl::bounds>(m_impl.get());
    }

    void webview::set_dev_tools(bool value)
    {
        return invoke<&impl::set_dev_tools>(m_impl.get(), value);
    }

    void webview::set_context_menu(bool value)
    {
        return invoke<&impl::set_context_menu>(m_impl.get(), value);
    }

    void webview::set_background(color background)
    {
        return invoke<&impl::set_background>(m_impl.get(), background);
    }

    void webview::set_force_dark_mode(bool value)
    {
        return invoke<&impl::set_force_dark_mode>(m_impl.get(), value);
    }

    void webview::reset_bounds()
    {
        return invoke<&impl::reset_bounds>(m_impl.get());
    }

    void webview::set_bounds(saucer::bounds bounds)
    {
        return invoke<&impl::set_bounds>(m_impl.get(), bounds);
    }

    void webview::set_url(const uri &url)
    {
        return invoke<&impl::set_url>(m_impl.get(), url);
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
        return invoke<&impl::back>(m_impl.get());
    }

    void webview::forward()
    {
        return invoke<&impl::forward>(m_impl.get());
    }

    void webview::reload()
    {
        return invoke<&impl::reload>(m_impl.get());
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

        return invoke(embed, m_impl.get(), std::move(files), std::move(handler));
    }

    void webview::unembed()
    {
        auto unembed = [impl = m_impl.get()]
        {
            impl->embedded.clear();
            impl->remove_scheme("saucer");
        };

        return invoke(unembed, m_impl.get());
    }

    void webview::unembed(const fs::path &file)
    {
        return invoke([impl = m_impl.get(), file] { impl->embedded.erase(file); }, m_impl.get());
    }

    void webview::execute(const std::string &code)
    {
        return invoke<&impl::execute>(m_impl.get(), code);
    }

    std::uint64_t webview::inject(const script &script)
    {
        return invoke<&impl::inject>(m_impl.get(), script);
    }

    void webview::uninject()
    {
        static constexpr auto uninject = static_cast<void (impl::*)()>(&impl::uninject);
        return invoke<uninject>(m_impl.get());
    }

    void webview::uninject(std::uint64_t id)
    {
        static constexpr auto uninject = static_cast<void (impl::*)(std::uint64_t)>(&impl::uninject);
        return invoke<uninject>(m_impl.get(), id);
    }

    void webview::remove_scheme(const std::string &name)
    {
        return invoke<&impl::remove_scheme>(m_impl.get(), name);
    }

    void webview::off(event event)
    {
        return invoke([impl = m_impl.get(), event] { impl->events->clear(event); }, m_impl.get());
    }

    void webview::off(event event, std::uint64_t id)
    {
        return invoke([impl = m_impl.get(), event, id] { impl->events->remove(event, id); }, m_impl.get());
    }

    void webview::register_scheme(const std::string &name)
    {
        return impl::register_scheme(name);
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_EVENT);
} // namespace saucer

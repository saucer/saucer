#include "webview.impl.hpp"

#include "invoke.hpp"
#include "instantiate.hpp"

#include "error.impl.hpp"
#include "window.impl.hpp"

#include <format>
#include <algorithm>
#include <functional>

namespace saucer
{
    using impl = webview::impl;

    webview::webview(application *app) : m_impl(detail::make_safe<impl>(app))
    {
        m_events = &m_impl->events;
    }

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
            return parent->invoke(&webview::create, opts);
        }

        if (static auto once{true}; once)
        {
            register_scheme("saucer");
            once = false;
        }

        auto rtn         = webview{parent};
        auto *const impl = rtn.m_impl.get();

        impl->window     = opts.window.value();
        impl->parent     = parent;
        impl->attributes = opts.attributes;

        if (auto status = impl->init_platform(opts); !status.has_value())
        {
            return err(status);
        }

        rtn.on<event::message>({{.func = std::bind_front(&impl::on_message, impl), .clearable = false}});

        rtn.inject({.code = impl::creation_script(), .run_at = script::time::creation, .clearable = false});
        rtn.inject({.code = impl::ready_script(), .run_at = script::time::ready, .clearable = false});

        if (opts.attributes)
        {
            rtn.inject({.code = impl::attribute_script(), .run_at = script::time::creation, .clearable = false});
        }

        return rtn;
    }

    webview::~webview()
    {
        utils::invoke([impl = m_impl.get()] { impl->events.clear(true); }, m_impl.get());
    }

    template <webview::event Event>
    void webview::setup()
    {
        return utils::invoke<&impl::setup<Event>>(m_impl.get());
    }

    void webview::handle(const std::string &name, scheme::resolver &&handler)
    {
        return utils::invoke<&impl::handle>(m_impl.get(), name, std::move(handler));
    }

    void impl::reject(std::size_t id, std::string_view reason)
    {
        static constexpr auto code = R"(
            window.saucer.internal.rpc[{0}].reject({1});
            delete window.saucer.internal.rpc[{0}];
        )";

        return utils::invoke([this, id, reason] { execute(std::format(code, id, reason)); }, this);
    }

    void impl::resolve(std::size_t id, std::string_view result)
    {
        static constexpr auto code = R"(
            window.saucer.internal.rpc[{0}].resolve({1});
            delete window.saucer.internal.rpc[{0}];
        )";

        return utils::invoke([this, id, result] { execute(std::format(code, id, result)); }, this);
    }

    window &webview::parent() const
    {
        return *m_impl->window;
    }

    result<saucer::url> webview::url() const
    {
        return utils::invoke<&impl::url>(m_impl.get());
    }

    icon webview::favicon() const
    {
        return utils::invoke<&impl::favicon>(m_impl.get());
    }

    std::string webview::page_title() const
    {
        return utils::invoke<&impl::page_title>(m_impl.get());
    }

    bool webview::dev_tools() const
    {
        return utils::invoke<&impl::dev_tools>(m_impl.get());
    }

    bool webview::context_menu() const
    {
        return utils::invoke<&impl::context_menu>(m_impl.get());
    }

    bool webview::force_dark() const
    {
        return utils::invoke<&impl::force_dark>(m_impl.get());
    }

    color webview::background() const
    {
        return utils::invoke<&impl::background>(m_impl.get());
    }

    bounds webview::bounds() const
    {
        return utils::invoke<&impl::bounds>(m_impl.get());
    }

    void webview::set_url(const saucer::url &url)
    {
        return utils::invoke<&impl::set_url>(m_impl.get(), url);
    }

    void webview::set_url(const std::string &url)
    {
        auto parsed = saucer::url::parse(url);

        if (!parsed.has_value())
        {
            return;
        }

        set_url(parsed.value());
    }

    void webview::set_dev_tools(bool value)
    {
        return utils::invoke<&impl::set_dev_tools>(m_impl.get(), value);
    }

    void webview::set_context_menu(bool value)
    {
        return utils::invoke<&impl::set_context_menu>(m_impl.get(), value);
    }

    void webview::set_force_dark(bool value)
    {
        return utils::invoke<&impl::set_force_dark>(m_impl.get(), value);
    }

    void webview::set_background(color background)
    {
        return utils::invoke<&impl::set_background>(m_impl.get(), background);
    }

    void webview::reset_bounds()
    {
        return utils::invoke<&impl::reset_bounds>(m_impl.get());
    }

    void webview::set_bounds(saucer::bounds bounds)
    {
        return utils::invoke<&impl::set_bounds>(m_impl.get(), bounds);
    }

    void webview::back()
    {
        return utils::invoke<&impl::back>(m_impl.get());
    }

    void webview::forward()
    {
        return utils::invoke<&impl::forward>(m_impl.get());
    }

    void webview::reload()
    {
        return utils::invoke<&impl::reload>(m_impl.get());
    }

    void webview::serve(fs::path file)
    {
        return set_url(saucer::url::make({.scheme = "saucer", .host = "embedded", .path = std::move(file)}));
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
            impl->handle("saucer", std::move(handler));
        };

        return utils::invoke(embed, m_impl.get(), std::move(files), std::move(handler));
    }

    void webview::unembed()
    {
        auto unembed = [impl = m_impl.get()]
        {
            impl->embedded.clear();
            impl->remove_scheme("saucer");
        };

        return utils::invoke(unembed, m_impl.get());
    }

    void webview::unembed(const fs::path &file)
    {
        return utils::invoke([impl = m_impl.get(), file] { impl->embedded.erase(file); }, m_impl.get());
    }

    void webview::execute(const std::string &code)
    {
        return utils::invoke<&impl::execute>(m_impl.get(), code);
    }

    std::size_t webview::inject(const script &script)
    {
        return utils::invoke<&impl::inject>(m_impl.get(), script);
    }

    void webview::uninject()
    {
        static constexpr auto uninject = static_cast<void (impl::*)()>(&impl::uninject);
        return utils::invoke<uninject>(m_impl.get());
    }

    void webview::uninject(std::size_t id)
    {
        static constexpr auto uninject = static_cast<void (impl::*)(std::size_t)>(&impl::uninject);
        return utils::invoke<uninject>(m_impl.get(), id);
    }

    void webview::remove_scheme(const std::string &name)
    {
        return utils::invoke<&impl::remove_scheme>(m_impl.get(), name);
    }

    void webview::off(event event)
    {
        return utils::invoke([impl = m_impl.get(), event] { impl->events.clear(event); }, m_impl.get());
    }

    void webview::off(event event, std::size_t id)
    {
        return utils::invoke([impl = m_impl.get(), event, id] { impl->events.remove(event, id); }, m_impl.get());
    }

    void webview::register_scheme(const std::string &name)
    {
        return impl::register_scheme(name);
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_EVENT);
} // namespace saucer

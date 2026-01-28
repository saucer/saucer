#include "qt.webview.impl.hpp"

#include "error.impl.hpp"

#include "scripts.hpp"
#include "instantiate.hpp"

#include "qt.url.impl.hpp"
#include "qt.icon.impl.hpp"
#include "qt.window.impl.hpp"

#include <ranges>

#include <QFile>
#include <QWebEngineScriptCollection>

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlScheme>
#include <QWebEngineFullScreenRequest>

namespace saucer
{
    using impl = webview::impl;

    impl::impl() = default;

    result<> impl::init_platform(const options &opts)
    {
        using enum QWebEngineProfile::PersistentCookiesPolicy;

        if (!native::init_web_channel())
        {
            return err(std::errc::no_such_file_or_directory);
        }

        auto flags = opts.browser_flags;

        if (opts.hardware_acceleration)
        {
            flags.emplace("--gpu");
            flags.emplace("--ignore-gpu-blocklist");
        }

        const auto arguments = flags                        //
                               | std::views::join_with(' ') //
                               | std::ranges::to<std::string>();

        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", arguments.c_str());

        auto profile = std::make_unique<QWebEngineProfile>("saucer");

        if (opts.user_agent.has_value())
        {
            profile->setHttpUserAgent(QString::fromStdString(*opts.user_agent));
        }

        if (opts.storage_path.has_value())
        {
            const auto path = QString::fromStdString(opts.storage_path->string());

            profile->setCachePath(path);
            profile->setPersistentStoragePath(path);
        }

        using enum QWebEngineProfile::PersistentPermissionsPolicy;
        profile->setPersistentPermissionsPolicy(AskEveryTime);

        profile->setPersistentCookiesPolicy(opts.persistent_cookies ? ForcePersistentCookies : NoPersistentCookies);
        profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
        profile->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

        platform = std::make_unique<native>();

        platform->profile     = std::move(profile);
        platform->web_view    = utils::make_deferred<QWebEngineView>();
        platform->web_page    = std::make_unique<QWebEnginePage>();
        platform->channel     = std::make_unique<QWebChannel>();
        platform->channel_obj = std::make_unique<web_class>(this);

        platform->web_view->setPage(platform->web_page.get());
        platform->web_page->setWebChannel(platform->channel.get());
        platform->channel->registerObject("saucer", platform->channel_obj.get());

        QWebEngineScript creation_script;
        {
            creation_script.setRunsOnSubFrames(true);
            creation_script.setWorldId(QWebEngineScript::MainWorld);

            creation_script.setName(native::creation_script);
            creation_script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        }
        platform->web_page->scripts().insert(creation_script);

        QWebEngineScript ready_script;
        {
            ready_script.setRunsOnSubFrames(true);
            ready_script.setWorldId(QWebEngineScript::MainWorld);

            ready_script.setName(native::ready_script);
            ready_script.setInjectionPoint(QWebEngineScript::DocumentReady);
        }
        platform->web_page->scripts().insert(ready_script);

        platform->on_load = platform->web_view->connect(platform->web_view.get(), &QWebEngineView::loadStarted,
                                                        [this]
                                                        {
                                                            platform->dom_loaded = false;
                                                            events.get<event::load>().fire(state::started);
                                                        });

        platform->on_fullscreen =
            platform->web_page->connect(platform->web_page.get(), &QWebEnginePage::fullScreenRequested,
                                        [this](QWebEngineFullScreenRequest request)
                                        {
                                            if (events.get<event::fullscreen>().fire(request.toggleOn()).find(policy::block))
                                            {
                                                return request.reject();
                                            }

                                            window->set_fullscreen(request.toggleOn());
                                            return request.accept();
                                        });

        platform->on_closed = window->on<window::event::closed>({{.func = [this] { set_dev_tools(false); }, .clearable = false}});

        window->native<false>()->platform->add_widget(platform->web_view.get());
        reset_bounds();

        platform->web_view->show();

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        set_dev_tools(false);
        window->off(window::event::closed, platform->on_closed);

        platform->web_view->disconnect(platform->on_load);
        platform->web_page->disconnect(platform->on_fullscreen);

        window->native<false>()->platform->remove_widget(platform->web_view.get());
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    url impl::url() const
    {
        auto rtn = platform->web_view->url();

        if (rtn.isEmpty() || !rtn.isValid())
        {
            return {};
        }

        return url::impl{rtn};
    }

    icon impl::favicon() const
    {
        return icon::impl{platform->web_view->icon()};
    }

    std::string impl::page_title() const
    {
        return platform->web_view->title().toStdString();
    }

    bool impl::dev_tools() const
    {
        return platform->dev_page != nullptr;
    }

    bool impl::context_menu() const
    {
        return platform->web_view->contextMenuPolicy() == Qt::ContextMenuPolicy::DefaultContextMenu;
    }

    bool impl::force_dark() const
    {
        const auto *settings = platform->profile->settings();
        return settings->testAttribute(QWebEngineSettings::ForceDarkMode);
    }

    color impl::background() const
    {
        const auto color = platform->web_page->backgroundColor();

        return {
            .r = static_cast<std::uint8_t>(color.red()),
            .g = static_cast<std::uint8_t>(color.green()),
            .b = static_cast<std::uint8_t>(color.blue()),
            .a = static_cast<std::uint8_t>(color.alpha()),
        };
    }

    bounds impl::bounds() const
    {
        const auto geometry = platform->web_view->geometry();
        return {.x = geometry.x(), .y = geometry.y(), .w = geometry.width(), .h = geometry.height()};
    }

    void impl::set_url(const saucer::url &url) // NOLINT(*-function-const)
    {
        platform->web_view->setUrl(url.native<false>()->url);
    }

    void impl::set_html(cstring_view html) // NOLINT(*-function-const)
    {
        platform->web_view->setHtml(QString::fromUtf8(html));
    }

    void impl::set_dev_tools(bool enabled) // NOLINT(*-function-const)
    {
        if (!platform->dev_page && !enabled)
        {
            return;
        }

        if (!enabled)
        {
            platform->web_page->setDevToolsPage(nullptr);
            platform->dev_page.reset();

            return;
        }

        if (!platform->dev_page)
        {
            platform->dev_page = std::make_unique<QWebEngineView>();
            platform->web_page->setDevToolsPage(platform->dev_page->page());
        }

        platform->dev_page->show();
    }

    void impl::set_context_menu(bool enabled) // NOLINT(*-function-const)
    {
        platform->web_view->setContextMenuPolicy(enabled ? Qt::ContextMenuPolicy::DefaultContextMenu
                                                         : Qt::ContextMenuPolicy::NoContextMenu);
    }

    void impl::set_force_dark([[maybe_unused]] bool enabled) // NOLINT(*-function-const)
    {
        auto *settings = platform->profile->settings();
        settings->setAttribute(QWebEngineSettings::ForceDarkMode, enabled);
    }

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        const auto [r, g, b, a] = color;

        platform->web_page->setBackgroundColor({r, g, b, a});
        platform->web_view->setAttribute(Qt::WA_TranslucentBackground, a < 255);
    }

    void impl::reset_bounds() // NOLINT(*-function-const)
    {
        window->native<false>()->platform->unmanaged.erase(platform->web_view.get());
        platform->web_view->layout()->invalidate();
    }

    void impl::set_bounds(saucer::bounds bounds) // NOLINT(*-function-const)
    {
        window->native<false>()->platform->unmanaged.emplace(platform->web_view.get());
        platform->web_view->setGeometry({bounds.x, bounds.y, bounds.w, bounds.h});
    }

    void impl::back() // NOLINT(*-function-const)
    {
        platform->web_view->back();
    }

    void impl::forward() // NOLINT(*-function-const)
    {
        platform->web_view->forward();
    }

    void impl::reload() // NOLINT(*-function-const)
    {
        platform->web_view->reload();
    }

    void impl::execute(cstring_view code) // NOLINT(*-function-const)
    {
        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        platform->web_view->page()->runJavaScript(QString::fromUtf8(code));
    }

    std::size_t impl::inject(const script &script) // NOLINT(*-function-const)
    {
        using enum script::time;

        auto original = platform->find(script.run_at == creation ? native::creation_script : native::ready_script);
        auto source   = original.sourceCode().toStdString();

        auto uuid       = QUuid::createUuid();
        auto identifier = std::format(native::script_identifier, uuid.toString().toStdString());

        auto code = script.code;

        if (script.no_frames)
        {
            code = std::format(R"js(
                if (self === top)
                {{
                    {}
                }}
            )js",
                               code);
        }

        auto replacement = original;
        auto new_source  = std::format("{0}\n{1}\n{2}\n{1}", source, identifier, code, identifier);

        replacement.setSourceCode(QString::fromStdString(new_source));

        platform->web_page->scripts().remove(original);
        platform->web_page->scripts().insert(replacement);

        const auto id = platform->id_counter++;
        platform->scripts.emplace(id, qt_script{.id = identifier, .run_at = script.run_at, .clearable = script.clearable});

        return id;
    }

    void impl::uninject()
    {
        static constexpr auto uninject = static_cast<void (impl::*)(std::size_t)>(&impl::uninject);

        auto clearable = platform->scripts                                                  //
                         | std::views::filter([](auto &it) { return it.second.clearable; }) //
                         | std::views::keys                                                 //
                         | std::ranges::to<std::vector>();

        std::ranges::for_each(clearable, std::bind_front(uninject, this));
    }

    void impl::uninject(std::size_t id) // NOLINT(*-function-const)
    {
        using enum script::time;

        if (!platform->scripts.contains(id))
        {
            return;
        }

        const auto &script = platform->scripts[id];

        auto original = platform->find(script.run_at == creation ? native::creation_script : native::ready_script);
        auto source   = original.sourceCode().toStdString();

        auto replacement       = original;
        const auto &identifier = script.id;

        const auto begin = source.find(identifier);
        const auto end   = source.find(identifier, begin + identifier.length()) + identifier.length();

        source.erase(begin, end - begin);
        replacement.setSourceCode(QString::fromStdString(source));

        platform->web_page->scripts().remove(original);
        platform->web_page->scripts().insert(replacement);

        platform->scripts.erase(id);
    }

    void impl::handle_scheme(const std::string &name, scheme::resolver &&resolver) // NOLINT(*-function-const)
    {
        if (platform->schemes.contains(name))
        {
            return;
        }

        auto &scheme = platform->schemes.emplace(name, std::move(resolver)).first->second;
        platform->web_view->page()->profile()->installUrlSchemeHandler(QByteArray::fromStdString(name), &scheme);
    }

    void impl::handle_stream_scheme(const std::string &name, scheme::stream_resolver &&resolver) // NOLINT(*-function-const)
    {
        if (platform->stream_schemes.contains(name))
        {
            return;
        }

        auto &scheme = platform->stream_schemes.emplace(name, std::move(resolver)).first->second;
        platform->web_view->page()->profile()->installUrlSchemeHandler(QByteArray::fromStdString(name), &scheme);
    }

    void impl::remove_scheme(const std::string &name) // NOLINT(*-function-const)
    {
        const auto it = platform->schemes.find(name);

        if (it == platform->schemes.end())
        {
            return;
        }

        platform->web_view->page()->profile()->removeUrlSchemeHandler(&it->second);
        platform->schemes.erase(it);
    }

    void impl::remove_stream_scheme(const std::string &name) // NOLINT(*-function-const)
    {
        const auto it = platform->stream_schemes.find(name);

        if (it == platform->stream_schemes.end())
        {
            return;
        }

        platform->web_view->page()->profile()->removeUrlSchemeHandler(&it->second);
        platform->stream_schemes.erase(it);
    }

    void impl::register_scheme(const std::string &name)
    {
        using enum QWebEngineUrlScheme::Flag;

        auto scheme = QWebEngineUrlScheme{QByteArray::fromStdString(name)};

        scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
        scheme.setFlags(SecureScheme | FetchApiAllowed | CorsEnabled);

        QWebEngineUrlScheme::registerScheme(scheme);
    }

    std::string impl::ready_script()
    {
        return "window.saucer.internal.message('dom_loaded')";
    }

    std::string impl::creation_script()
    {
        static const auto script = std::format(scripts::ipc_script, R"js(
            channel: new Promise((resolve) =>
            {
                new QWebChannel(qt.webChannelTransport, function(channel) {
                    resolve(channel.objects.saucer);
                });
            }),
            message: async (message) =>
            {
                (await window.saucer.internal.channel).on_message(message);
            }
        )js");

        return std::format("{}\n{}", native::channel_script, script);
    }

    SAUCER_INSTANTIATE_WEBVIEW_EVENTS(SAUCER_INSTANTIATE_WEBVIEW_IMPL_EVENT);
} // namespace saucer

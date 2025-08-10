#include "qt.webview.impl.hpp"

#include "scripts.hpp"
#include "instantiate.hpp"

#include "qt.uri.impl.hpp"
#include "qt.icon.impl.hpp"
#include "qt.window.impl.hpp"

#include <ranges>

#include <QFile>
#include <QWebEngineScriptCollection>

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlScheme>

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

        platform = std::make_unique<native>();

        static std::once_flag flag;
        std::call_once(flag, [] { register_scheme("saucer"); });

        auto flags = opts.browser_flags;

        if (opts.hardware_acceleration)
        {
            flags.emplace("--gpu");
            flags.emplace("--ignore-gpu-blocklist");
        }

        const auto args = flags | std::views::join_with(' ') | std::ranges::to<std::string>();
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", args.c_str());

        platform->profile = std::make_unique<QWebEngineProfile>("saucer");

        if (opts.user_agent.has_value())
        {
            platform->profile->setHttpUserAgent(QString::fromStdString(opts.user_agent.value()));
        }

        if (opts.storage_path.has_value())
        {
            const auto path = QString::fromStdString(opts.storage_path->string());

            platform->profile->setCachePath(path);
            platform->profile->setPersistentStoragePath(path);
        }

#ifdef SAUCER_QT6
        using enum QWebEngineProfile::PersistentPermissionsPolicy;
        platform->profile->setPersistentPermissionsPolicy(AskEveryTime);
#endif

        platform->profile->setPersistentCookiesPolicy(opts.persistent_cookies ? ForcePersistentCookies : NoPersistentCookies);
        platform->profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

        platform->web_view    = std::make_unique<QWebEngineView>(window->native<false>()->platform->window->centralWidget());
        platform->web_page    = std::make_unique<QWebEnginePage>(platform->profile.get());
        platform->channel     = std::make_unique<QWebChannel>();
        platform->channel_obj = std::make_unique<web_class>(this);

        platform->web_view->setPage(platform->web_page.get());
        platform->web_page->setWebChannel(platform->channel.get());
        platform->channel->registerObject("saucer", platform->channel_obj.get());

        QWebEngineScript script;
        {
            script.setRunsOnSubFrames(true);
            script.setWorldId(QWebEngineScript::MainWorld);
        }

        script.setName(native::creation_script);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);

        platform->web_page->scripts().insert(script);

        script.setName(native::ready_script);
        script.setInjectionPoint(QWebEngineScript::DocumentReady);

        platform->web_page->scripts().insert(script);

        platform->web_view->connect(platform->web_view.get(), &QWebEngineView::loadStarted,
                                    [this]
                                    {
                                        platform->dom_loaded = false;
                                        events->get<event::load>().fire(state::started);
                                    });

        platform->on_closed = window->on<window::event::closed>({{.func = [this] { set_dev_tools(false); }, .clearable = false}});
        window->native<false>()->platform->window->centralWidget()->layout()->addWidget(platform->web_view.get());

        platform->web_view->show();

        return {};
    }

    impl::~impl()
    {
        if (!platform)
        {
            return;
        }

        window->off(window::event::closed, platform->on_closed);

        set_dev_tools(false);
        platform->web_view->disconnect();

        window->native<false>()->platform->window->centralWidget()->layout()->removeWidget(platform->web_view.get());
    }

    template <webview::event Event>
    void impl::setup()
    {
        platform->setup<Event>(this);
    }

    icon impl::favicon() const
    {
        return {icon::impl{platform->web_view->icon()}};
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

    std::optional<uri> impl::url() const
    {
        auto url = platform->web_view->url();

        if (!url.isValid())
        {
            return std::nullopt;
        }

        return uri::impl{url};
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

    bool impl::force_dark_mode() const
    {
#ifdef SAUCER_QT6
        const auto *settings = platform->profile->settings();
        return settings->testAttribute(QWebEngineSettings::ForceDarkMode);
#else
        return false;
#endif
    }

    bounds impl::bounds() const
    {
        const auto geometry = platform->web_view->geometry();
        return {.x = geometry.x(), .y = geometry.y(), .w = geometry.width(), .h = geometry.height()};
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

    void impl::set_background(color color) // NOLINT(*-function-const)
    {
        const auto [r, g, b, a] = color;

        platform->web_page->setBackgroundColor({r, g, b, a});
        platform->web_view->setAttribute(Qt::WA_TranslucentBackground, a < 255);
    }

    void impl::set_force_dark_mode([[maybe_unused]] bool enabled) // NOLINT(*-function-const)
    {
#ifdef SAUCER_QT6
        auto *settings = platform->profile->settings();
        settings->setAttribute(QWebEngineSettings::ForceDarkMode, enabled);
#endif
    }

    void impl::unset_bounds() // NOLINT(*-function-const)
    {
        platform->web_view->setGeometry(QRect{});
        platform->web_view->layout()->invalidate();
    }

    void impl::set_bounds(saucer::bounds bounds) // NOLINT(*-function-const)
    {
        platform->web_view->setGeometry({bounds.x, bounds.y, bounds.w, bounds.h});
    }

    void impl::set_url(const uri &url) // NOLINT(*-function-const)
    {
        platform->web_view->setUrl(url.native<false>()->uri);
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

    void impl::execute(const std::string &code) // NOLINT(*-function-const)
    {
        if (!platform->dom_loaded)
        {
            platform->pending.emplace_back(code);
            return;
        }

        platform->web_view->page()->runJavaScript(QString::fromStdString(code));
    }

    std::uint64_t impl::inject(const script &script) // NOLINT(*-function-const)
    {
        using enum load_time;
        using enum web_frame;

        auto original = platform->find(script.time == creation ? native::creation_script : native::ready_script);
        auto source   = original.sourceCode().toStdString();

        auto uuid       = QUuid::createUuid();
        auto identifier = std::format(native::script_identifier, uuid.toString().toStdString());

        auto code = script.code;

        if (script.frame == top)
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
        platform->scripts.emplace(id, qt_script{.id = identifier, .time = script.time, .clearable = script.clearable});

        return id;
    }

    void impl::uninject()
    {
        auto scripts = platform->scripts;

        for (const auto &[id, script] : scripts)
        {
            if (!script.clearable)
            {
                continue;
            }

            uninject(id);
        }
    }

    void impl::uninject(std::uint64_t id) // NOLINT(*-function-const)
    {
        using enum load_time;

        if (!platform->scripts.contains(id))
        {
            return;
        }

        const auto &script = platform->scripts[id];

        auto original = platform->find(script.time == creation ? native::creation_script : native::ready_script);
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

    void impl::register_scheme(const std::string &name)
    {
        using enum QWebEngineUrlScheme::Flag;

        auto scheme = QWebEngineUrlScheme{QByteArray::fromStdString(name)};
        scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);

#ifdef SAUCER_QT6
        scheme.setFlags(SecureScheme | FetchApiAllowed | CorsEnabled);
#else
        scheme.setFlags(SecureScheme | CorsEnabled);
#endif

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

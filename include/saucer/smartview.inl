#pragma once
#include "smartview.hpp"
#include "constants.hpp"

namespace saucer
{
    template <typename Plugin> void smartview::add_plugin()
    {
        auto plugin = std::make_unique<Plugin>();

        plugin->load(*this);
        m_plugins.emplace_back(std::move(plugin));
    }

    template <typename Return, typename Serializer, typename... Params> std::shared_ptr<promise<Return>> smartview::eval(const std::string &code, Params &&...params)
    {
        if (!m_serializers.read()->count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.write()->emplace(typeid(Serializer), std::make_unique<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        auto rtn = std::make_shared<promise<Return>>(m_creation_thread);
        add_eval(typeid(Serializer), rtn, Serializer::resolve_promise(rtn), fmt::vformat(code, Serializer::serialize_arguments(params...)));

        return rtn;
    }

    template <typename Serializer, typename Function> void smartview::expose(const std::string &name, const Function &func, bool async)
    {
        if (!m_serializers.read()->count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.write()->emplace(typeid(Serializer), std::make_unique<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        add_callback(typeid(Serializer), name, Serializer::serialize_function(func), async);
    }

    template <typename DefaultSerializer>
    template <typename Serializer, typename Function>
    void simple_smartview<DefaultSerializer>::expose(const std::string &name, const Function &func, bool async)
    {
        smartview::expose<Serializer>(name, func, async);
    }

    template <typename DefaultSerializer>
    template <typename Return, typename Serializer, typename... Params>
    std::shared_ptr<promise<Return>> simple_smartview<DefaultSerializer>::eval(const std::string &code, Params &&...params)
    {
        return smartview::eval<Return, Serializer>(code, std::forward<Params>(params)...);
    }

    template <template <backend_type> class... Modules>
    modularized_smartview<Modules...>::modularized_smartview()
        : Modules<backend>(*this, reinterpret_cast<typename Modules<backend>::webview_impl *>(m_impl.get()),
                           reinterpret_cast<typename Modules<backend>::window_impl *>(window::m_impl.get()))...
    {
    }

    template <typename DefaultSerializer, template <backend_type> class... Modules>
    modularized_simple_smartview<DefaultSerializer, Modules...>::modularized_simple_smartview()
        : Modules<backend>(*this, reinterpret_cast<typename Modules<backend>::webview_impl *>(m_impl.get()),
                           reinterpret_cast<typename Modules<backend>::window_impl *>(window::m_impl.get()))...
    {
    }

    template <typename DefaultSerializer, template <backend_type> class... Modules>
    template <typename Serializer, typename Function>
    void modularized_simple_smartview<DefaultSerializer, Modules...>::expose(const std::string &name, const Function &func, bool async)
    {
        smartview::expose<Serializer>(name, func, async);
    }

    template <typename DefaultSerializer, template <backend_type> class... Modules>
    template <typename Return, typename Serializer, typename... Params>
    std::shared_ptr<promise<Return>> modularized_simple_smartview<DefaultSerializer, Modules...>::eval(const std::string &code, Params &&...params)
    {
        return smartview::eval<Return, Serializer>(code, std::forward<Params>(params)...);
    }
} // namespace saucer
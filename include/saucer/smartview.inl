#pragma once
#include "smartview.hpp"
#include "constants.hpp"

#include <fmt/format.h>

namespace saucer
{
    struct smartview::callback_t
    {
        const bool async;
        const resolve_callback resolve;
        const std::type_index serializer_type;
    };

    struct smartview::eval_t
    {
        const eval_callback resolve;
        const std::type_index serializer_type;
    };

    template <typename Plugin> Plugin &smartview::add_plugin()
    {
        auto plugin = std::make_unique<Plugin>();
        plugin->load(*this);

        auto plugins = m_plugins.write();

        plugins->emplace_back(std::move(plugin));
        return *plugins->back();
    }

    template <typename Return, typename Serializer, typename... Params>
    std::future<Return> smartview::eval(const std::string &code, Params &&...params)
    {
        const auto &type = typeid(Serializer);

        if (!m_serializers.read()->count(typeid(Serializer)))
        {
            auto serializers = m_serializers.write();
            serializers->emplace(type, std::make_unique<Serializer>());
            const auto &serializer = serializers->at(type);

            inject(serializer->init_script(), load_time::creation);
        }

        auto promise = std::make_shared<std::promise<Return>>();
        auto fut = promise->get_future();

        auto resolve = Serializer::resolve(std::move(promise));
        add_eval(type, std::move(resolve), fmt::vformat(code, Serializer::serialize_args(params...)));

        return fut;
    }

    template <typename Serializer, typename Function>
    void smartview::expose(const std::string &name, const Function &func, bool async)
    {
        const auto &type = typeid(Serializer);

        if (!m_serializers.read()->count(type))
        {
            auto serializers = m_serializers.write();
            serializers->emplace(type, std::make_unique<Serializer>());
            const auto &serializer = serializers->at(type);

            inject(serializer->init_script(), load_time::creation);
        }

        auto resolve = Serializer::serialize(func);
        add_callback(type, name, std::move(resolve), async);
    }

    template <typename DefaultSerializer> class simple_smartview : public smartview
    {
      public:
        template <typename Serializer = DefaultSerializer, typename Function>
        void expose(const std::string &name, const Function &func, bool async = false)
        {
            smartview::expose<Serializer>(name, func, async);
        }

      public:
        template <typename Return, typename Serializer = DefaultSerializer, typename... Params>
        [[nodiscard]] std::future<Return> eval(const std::string &code, Params &&...params)
        {
            return smartview::eval<Return, Serializer>(code, std::forward<Params>(params)...);
        }
    };

    template <template <backend_type> class... Modules>
    class modularized_smartview : public smartview, public Modules<backend>...
    {
      public:
        modularized_smartview()
            : Modules<backend>(*this, reinterpret_cast<typename Modules<backend>::webview_impl *>(m_impl.get()),
                               reinterpret_cast<typename Modules<backend>::window_impl *>(window::m_impl.get()))...
        {
        }
    };

    template <typename DefaultSerializer, template <backend_type> class... Modules>
    class modularized_simple_smartview : public smartview, public Modules<backend>...
    {
      public:
        modularized_simple_smartview()
            : Modules<backend>(*this, reinterpret_cast<typename Modules<backend>::webview_impl *>(m_impl.get()),
                               reinterpret_cast<typename Modules<backend>::window_impl *>(window::m_impl.get()))...
        {
        }

      public:
        template <typename Serializer = DefaultSerializer, typename Function>
        void expose(const std::string &name, const Function &func, bool async = false)
        {
            smartview::expose<Serializer>(name, func, async);
        }

      public:
        template <typename Return, typename Serializer = DefaultSerializer, typename... Params>
        [[nodiscard]] std::future<Return> eval(const std::string &code, Params &&...params)
        {
            return smartview::eval<Return, Serializer>(code, std::forward<Params>(params)...);
        }
    };
} // namespace saucer
#pragma once
#include "webview.hpp"
#include "plugin/plugin.hpp"
#include "module/module.hpp"
#include "serializers/serializer.hpp"

#include <future>
#include <typeindex>
#include <fmt/args.h>
#include <lockpp/lock.hpp>
#include <tl/expected.hpp>

namespace saucer
{
#include "annotations.hpp"
    class smartview : public webview
    {
        using callback_resolver =
            std::function<tl::expected<std::string, serializer::error>(const std::shared_ptr<function_data> &)>;

        using eval_resolver = std::function<void(const std::shared_ptr<result_data> &)>;

        struct callback_t
        {
            const bool async;
            const callback_resolver resolver;
            const std::type_index serializer_type;
        };

        struct eval_t
        {
            const eval_resolver resolver;
            const std::type_index serializer_type;
        };

      protected:
        lockpp::lock<std::map<const std::type_index, const std::unique_ptr<serializer>>> m_serializers;
        lockpp::lock<std::map<const std::string, const callback_t>> m_callbacks;
        lockpp::lock<std::map<std::size_t, eval_t>> m_evals;
        std::vector<std::unique_ptr<plugin>> m_plugins;
        std::atomic<std::size_t> m_id_counter{0};

      public:
        smartview();
        ~smartview() override;

      protected:
        void on_message(const std::string &) override;

      protected:
        [[thread_safe]] void resolve(const std::shared_ptr<function_data> &, const callback_resolver &);
        [[thread_safe]] void add_eval(const std::type_index &, const eval_resolver &, const std::string &);
        [[thread_safe]] void add_callback(const std::type_index &, const std::string &, const callback_resolver &,
                                          bool);

      protected:
        [[thread_safe]] void reject(const std::size_t &, const std::string &);
        [[thread_safe]] void resolve(const std::size_t &, const std::string &);

      public:
        template <typename Plugin> void add_plugin();

      public:
        template <typename Serializer, typename Function>
        [[thread_safe]] void expose(const std::string &name, const Function &func, bool async = false);

      public:
        template <typename Return, typename Serializer, typename... Params>
        [[thread_safe]] [[nodiscard]] std::future<Return> eval(const std::string &code, Params &&...params);
    };

    template <typename DefaultSerializer> class simple_smartview : public smartview
    {
      public:
        template <typename Serializer = DefaultSerializer, typename Function>
        [[thread_safe]] void expose(const std::string &name, const Function &func, bool async = false);

      public:
        template <typename Return, typename Serializer = DefaultSerializer, typename... Params>
        [[thread_safe]] [[nodiscard]] std::future<Return> eval(const std::string &code, Params &&...params);
    };

    template <template <backend_type> class... Modules>
    class modularized_smartview : public smartview, public Modules<backend>...
    {
      public:
        modularized_smartview();
    };

    template <typename DefaultSerializer, template <backend_type> class... Modules>
    class modularized_simple_smartview : public smartview, public Modules<backend>...
    {
      public:
        modularized_simple_smartview();

      public:
        template <typename Serializer = DefaultSerializer, typename Function>
        [[thread_safe]] void expose(const std::string &name, const Function &func, bool async = false);

      public:
        template <typename Return, typename Serializer = DefaultSerializer, typename... Params>
        [[thread_safe]] [[nodiscard]] std::future<Return> eval(const std::string &code, Params &&...params);
    };
#include "annotations.hpp" //NOLINT
} // namespace saucer

#include "smartview.inl"
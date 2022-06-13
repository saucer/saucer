#pragma once
#include <typeindex>
#include <fmt/args.h>
#include <tl/expected.hpp>

#include "webview.hpp"
#include "modules/module.hpp"
#include "promise/promise.hpp"
#include "serializers/serializer.hpp"

namespace saucer
{
#include "annotations.hpp"
    class smartview : public webview
    {
        template <typename Result, typename Data> using resolver_t = std::function<tl::expected<Result, serializer::error>(const std::shared_ptr<Data> &)>;
        using callback_resolver = resolver_t<std::string, function_data>;
        using eval_resolver = resolver_t<void, result_data>;

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
            const std::shared_ptr<promise_base> promise;
        };

      protected:
        lockpp::lock<std::map<const std::type_index, const std::unique_ptr<serializer>>> m_serializers;
        lockpp::lock<std::map<const std::string, const callback_t>> m_callbacks;
        lockpp::lock<std::map<std::size_t, eval_t>> m_evals;
        std::vector<std::unique_ptr<module>> m_modules;
        std::atomic<std::size_t> m_id_counter{0};
        std::thread::id m_creation_thread;

      public:
        smartview();
        ~smartview() override;

      protected:
        void on_message(const std::string &) override;
        void on_url_changed(const std::string &) override;

      protected:
        [[thread_safe]] void resolve(const std::shared_ptr<function_data> &, const callback_resolver &);
        [[thread_safe]] void add_callback(const std::type_index &, const std::string &, const callback_resolver &, bool);
        [[thread_safe]] void add_eval(const std::type_index &, const std::shared_ptr<promise_base> &, const eval_resolver &, const std::string &);

      protected:
        [[thread_safe]] void reject(const std::size_t &, const std::string &);
        [[thread_safe]] void resolve(const std::size_t &, const std::string &);

      public:
        std::vector<module *> get_modules() const;
        module *get_module(const std::string &name) const;

      public:
        template <typename Module, typename... Args> void add_module(Args &&...);
        template <typename Serializer, typename Function> [[thread_safe]] void expose(const std::string &name, const Function &func, bool async = false);
        template <typename Return, typename Serializer, typename... Params> [[thread_safe]] std::shared_ptr<promise<Return>> eval(const std::string &code, Params &&...params);
    };

    template <typename DefaultSerializer> class simple_smartview : public smartview
    {
      public:
        template <typename Serializer = DefaultSerializer, typename Function> void expose(const std::string &name, const Function &func, bool async = false);
        template <typename Return, typename Serializer = DefaultSerializer, typename... Params> std::shared_ptr<promise<Return>> eval(const std::string &code, Params &&...params);
    };
#include "annotations.hpp" //NOLINT
} // namespace saucer

#include "smartview.inl"
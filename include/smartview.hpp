#pragma once
#include <fmt/args.h>
#include <modules/module.hpp>
#include <promise/promise.hpp>
#include <serializers/serializer.hpp>
#include <tl/expected.hpp>
#include <typeindex>
#include <webview.hpp>

namespace saucer
{
    class smartview : public webview
    {
        using callback_t = std::function<tl::expected<std::string, serializer::error>(const std::shared_ptr<function_data> &)>;
        using resolve_callback_t = std::function<tl::expected<void, serializer::error>(const std::shared_ptr<result_data> &)>;

      protected:
        lockpp::lock<std::map<const std::size_t, std::tuple<const resolve_callback_t, const std::shared_ptr<base_promise>, const std::type_index>>> m_evals;
        std::map<const std::string, std::tuple<const callback_t, const bool, const std::type_index>> m_callbacks;
        std::map<const std::type_index, const std::shared_ptr<serializer>> m_serializers;
        std::map<const std::type_index, const std::shared_ptr<module>> m_modules;
        std::atomic<std::size_t> m_id_counter{};
        std::thread::id m_creation_thread;

      public:
        smartview();
        ~smartview() override;

      protected:
        void on_message(const std::string &) override;
        void url_changed(const std::string &) override;
        void resolve_callback(const std::shared_ptr<function_data> &, const callback_t &);
        void add_callback(const std::type_index &, const std::string &, const callback_t &, bool);
        void add_eval(const std::type_index &, const std::shared_ptr<base_promise> &, const resolve_callback_t &, const std::string &);

      protected:
        void reject(const std::size_t &, const std::string &);
        void resolve(const std::size_t &, const std::string &);

      public:
        std::vector<std::shared_ptr<module>> get_modules();
        std::shared_ptr<module> get_module(const std::string &name);

      public:
        template <typename module_t> void add_module();
        template <typename module_t> std::shared_ptr<module_t> get_module();
        template <typename serializer_t, typename func_t> void expose(const std::string &name, const func_t &func, bool async = false);
        template <typename rtn_t, typename serializer_t, typename... params_t> auto eval(const std::string &code, params_t &&...params);
    };

    template <typename default_serializer> class simple_smartview : public smartview
    {
      public:
        template <typename serializer_t = default_serializer, typename func_t> void expose(const std::string &name, const func_t &func, bool async = false);
        template <typename rtn_t, typename serializer_t = default_serializer, typename... params_t> auto eval(const std::string &code, params_t &&...params);
    };
} // namespace saucer

#include "smartview.inl"
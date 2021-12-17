#pragma once
#include <atomic>
#include <fmt/args.h>
#include <serializers/serializer.hpp>
#include <tl/expected.hpp>
#include <typeindex>
#include <webview.hpp>

namespace saucer
{
    class smartview : public webview
    {
        using callback_t = std::function<tl::expected<std::string, serializer::error>(const std::shared_ptr<message_data> &)>;
        using resolve_callback_t = std::function<tl::expected<void, serializer::error>(const std::shared_ptr<result_data> &)>;
        using arg_store_t = fmt::dynamic_format_arg_store<fmt::format_context>;

      protected:
        std::map<const std::type_index, const std::shared_ptr<serializer>> m_serializers;
        std::map<const std::size_t, const resolve_callback_t> m_evals;
        std::map<const std::string, const callback_t> m_callbacks;
        std::atomic<std::uint64_t> m_id_counter{};

      public:
        smartview();
        ~smartview() override;

      protected:
        void on_message(const std::string &) override;
        void add_callback(const std::shared_ptr<serializer> &, const std::string &, const callback_t &);
        void add_eval(const std::shared_ptr<serializer> &, const resolve_callback_t &, const std::string &, const arg_store_t &);

      protected:
        void reject(const std::size_t &, const std::string &);
        void resolve(const std::size_t &, const std::string &);

      public:
        template <typename serializer_t, typename func_t> void expose(const std::string &name, const func_t &func);
        template <typename rtn_t, typename serializer_t, typename... params_t> auto eval(const std::string &code, params_t &&...params);
    };

    template <typename default_serializer> class simple_smartview : public smartview
    {
      public:
        template <typename serializer_t = default_serializer, typename func_t> void expose(const std::string &name, const func_t &func);
        template <typename rtn_t, typename serializer_t = default_serializer, typename... params_t> auto eval(const std::string &code, params_t &&...params);
    };
} // namespace saucer

#include "smartview.inl"
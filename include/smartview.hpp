#pragma once
#include <serializer.hpp>
#include <tl/expected.hpp>
#include <typeindex>
#include <webview.hpp>

namespace saucer
{
    class smartview : public webview
    {
      public:
        using callback_t = std::function<tl::expected<std::string, serializer::error>(const std::shared_ptr<message_data> &)>;

      protected:
        std::map<const std::type_index, std::shared_ptr<serializer>> m_serializers;
        std::map<const std::string, const callback_t> m_callbacks;

      public:
        smartview();
        ~smartview() override;

      protected:
        void on_message(const std::string &) override;
        void add_callback(const std::shared_ptr<serializer> &, const std::string &, const callback_t &);

      protected:
        void reject(const std::size_t &, const std::string &);
        void resolve(const std::size_t &, const std::string &);

      public:
        template <typename serializer_t, typename func_t> void expose(const std::string &name, const func_t &func);
    };
} // namespace saucer

#include "smartview.inl"
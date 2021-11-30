#pragma once
#include <functional>
#include <string>

namespace saucer
{
    namespace internal
    {
        template <class bridge_t> class is_bridge
        {
            static auto test(...) -> std::uint8_t;
            template <typename O>
            static auto test(O *) -> decltype(std::declval<O>().expose(std::declval<std::string>(), std::declval<std::function<int(int)>>()),
                                              std::declval<O>().expose_async(std::declval<std::string>(), std::declval<std::function<int(int)>>()),
                                              std::declval<O>().template call<int>(std::declval<std::string>(), std::declval<int>()), std::uint16_t{});

          public:
            static const bool value = sizeof(test(reinterpret_cast<bridge_t *>(0))) == sizeof(std::uint16_t);
        };
        template <typename bridge_t> inline constexpr bool is_bridge_t = is_bridge<bridge_t>::value;
    } // namespace internal

    class webview;
    class bridge
    {
      protected:
        webview &m_webview;

      protected:
        bridge(webview &);

      public:
        virtual ~bridge();

      public:
        virtual void on_message(const std::string &) = 0;
    };
} // namespace saucer
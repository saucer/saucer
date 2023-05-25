#pragma once
#include <string>
#include <concepts>

namespace saucer
{
    class smartview;
    class plugin
    {
      public:
        virtual ~plugin() = default;

      public:
        virtual void load(smartview &) = 0;

      public:
        [[nodiscard]] virtual std::string get_name() const = 0;
        [[nodiscard]] virtual std::string get_version() const = 0;
    };

    template <typename T>
    concept Plugin = requires() { requires std::derived_from<T, plugin>; };
} // namespace saucer
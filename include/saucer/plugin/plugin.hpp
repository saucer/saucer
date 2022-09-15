#pragma once
#include <string>

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
        virtual std::string get_name() const = 0;
        virtual std::string get_version() const = 0;
    };
} // namespace saucer
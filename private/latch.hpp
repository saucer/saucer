#pragma once

#include <memory>

namespace saucer
{
    class shared_latch
    {
        struct state;

      public:
        std::shared_ptr<state> m_state;

      public:
        shared_latch();

      public:
        shared_latch(const shared_latch &);

      public:
        ~shared_latch();

      public:
        [[nodiscard]] bool empty() const;
    };
} // namespace saucer

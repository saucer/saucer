#include "latch.hpp"

#include <atomic>

namespace saucer
{
    struct shared_latch::state
    {
        std::atomic_size_t count;
    };

    shared_latch::shared_latch() : m_state(std::make_shared<state>()) {}

    shared_latch::shared_latch(const shared_latch &other) : m_state(other.m_state)
    {
        m_state->count.fetch_add(1);
    }

    shared_latch::~shared_latch()
    {
        m_state->count.fetch_sub(1);
    }

    bool shared_latch::empty() const
    {
        return m_state->count.load() == 0;
    }
} // namespace saucer

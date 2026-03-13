#include "stash.impl.hpp"

#include <atomic>
#include <utility>

namespace saucer
{
    std::size_t stash::impl::count()
    {
        static auto counter = std::atomic_size_t{};
        return counter.fetch_add(1);
    }

    stash::impl::lazy::lazy(std::function<stash()> callable) : m_data(std::move(callable)) {}

    stash &stash::impl::lazy::unwrap() const
    {
        if (m_data.index() == 0)
        {
            m_data.emplace<1>(std::get<0>(m_data)());
        }

        return std::get<1>(m_data);
    }

    std::size_t stash::impl::lazy::type() const
    {
        return id_of<lazy>();
    }

    std::unique_ptr<stash::impl> stash::impl::lazy::clone() const
    {
        return std::make_unique<lazy>(*this);
    }

    stash::span stash::impl::lazy::data() const
    {
        return unwrap().data();
    }

    stash::impl::view::view(span data) : m_data(data) {}

    std::size_t stash::impl::view::type() const
    {
        return id_of<view>();
    }

    std::unique_ptr<stash::impl> stash::impl::view::clone() const
    {
        return std::make_unique<view>(*this);
    }

    stash::span stash::impl::view::data() const
    {
        return m_data;
    }

    stash::impl::owng::owng(vec data) : m_data(std::move(data)) {}

    std::size_t stash::impl::owng::type() const
    {
        return id_of<owng>();
    }

    std::unique_ptr<stash::impl> stash::impl::owng::clone() const
    {
        return std::make_unique<owng>(*this);
    }

    stash::span stash::impl::owng::data() const
    {
        return m_data;
    }
} // namespace saucer

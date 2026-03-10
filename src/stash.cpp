#include "stash.impl.hpp"

namespace saucer
{
    stash::stash(std::unique_ptr<impl> data) : m_impl(std::move(data)) {}

    stash::stash(const stash &other) : m_impl(other.m_impl->clone()) {}

    stash::stash(stash &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    stash::~stash() = default;

    std::string_view stash::str() const
    {
        const auto content = data();
        return {reinterpret_cast<const char *>(content.data()), content.size()};
    }

    std::span<const std::uint8_t> stash::data() const
    {
        return m_impl->data();
    }

    stash stash::from(std::vector<std::uint8_t> data)
    {
        return {std::make_unique<impl::owng>(std::move(data))};
    }

    stash stash::view(std::span<const std::uint8_t> data)
    {
        return {std::make_unique<impl::view>(data)};
    }

    stash stash::lazy(std::function<stash()> callable)
    {
        return {std::make_unique<impl::lazy>(std::move(callable))};
    }

    stash stash::from_str(std::string_view str)
    {
        const auto *const begin = reinterpret_cast<const std::uint8_t *>(str.data());
        const auto *const end   = reinterpret_cast<const std::uint8_t *>(str.data() + str.size());

        return from({begin, end});
    }

    stash stash::view_str(std::string_view str)
    {
        const auto *const begin = reinterpret_cast<const std::uint8_t *>(str.data());
        const auto *const end   = reinterpret_cast<const std::uint8_t *>(str.data() + str.size());

        return view({begin, end});
    }

    stash stash::empty()
    {
        return view({});
    }
} // namespace saucer

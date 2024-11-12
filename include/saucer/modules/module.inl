#pragma once

#include "module.hpp"

#include <atomic>
#include <algorithm>

namespace saucer
{
    namespace impl
    {
        inline auto count()
        {
            static std::atomic_size_t counter{};
            return counter++;
        }

        template <typename T>
        struct id
        {
            static const inline auto value = count();
        };
    } // namespace impl

    struct erased_module::base
    {
        template <typename T>
        class model;

      public:
        virtual ~base() = default;

      public:
        virtual bool on_message(const std::string &) = 0;
    };

    template <typename T>
    class erased_module::base::model : public base
    {
        T m_value;

      public:
        model(T &&value) : m_value(std::move(value)) {}

      public:
        ~model() override = default;

      public:
        T *value()
        {
            return &m_value;
        }

      public:
        bool on_message([[maybe_unused]] const std::string &message) override
        {
            if constexpr (requires {
                              { m_value.on_message(message) } -> std::same_as<bool>;
                          })
            {
                return m_value.on_message(message);
            }

            return false;
        }
    };

    inline erased_module::base *erased_module::get() const
    {
        return m_value.get();
    }

    template <typename T>
    std::optional<T *> erased_module::get() const
    {
        if (id_of<T>() != m_id)
        {
            return std::nullopt;
        }

        return static_cast<base::model<T> *>(m_value.get())->value();
    }

    template <typename T>
    std::size_t erased_module::id_of()
    {
        return impl::id<T>::value;
    }

    template <typename T, typename... Ts>
    erased_module erased_module::from(Ts &&...args)
    {
        erased_module rtn;

        rtn.m_id    = id_of<T>();
        rtn.m_value = std::make_unique<base::model<T>>(T{std::forward<Ts>(args)...});

        return rtn;
    }

    template <typename T>
    extensible<T>::extensible(T *parent) : m_parent(parent)
    {
    }

    template <typename T>
    bool extensible<T>::on_message(const std::string &message)
    {
        return std::ranges::any_of(m_modules, [&](const auto &item) { return item.second.get()->on_message(message); });
    }

    template <typename T>
    template <typename M, typename... Ts>
        requires Module<M, T, Ts...>
    M &extensible<T>::add_module(Ts &&...args)
    {
        const auto id      = erased_module::id_of<M>();
        const auto [it, _] = m_modules.emplace(id, erased_module::from<M>(m_parent, std::forward<Ts>(args)...));

        return *it->second.template get<M>().value();
    }

    template <typename T>
    template <typename M>
    std::optional<M *> extensible<T>::module()
    {
        const auto key = erased_module::id_of<M>();

        if (!m_modules.contains(key))
        {
            return std::nullopt;
        }

        return m_modules.at(key).template get<M>();
    }
} // namespace saucer

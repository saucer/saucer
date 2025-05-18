#pragma once

#include "serializer.hpp"

#include "../utils/tuple.hpp"
#include "../utils/traits.hpp"

#include <ranges>

namespace saucer
{
    namespace impl
    {
        template <typename Interface, typename T>
        std::expected<T, std::string> read(const auto &value)
        {
            return Interface::template read<T>(value);
        }

        template <typename Interface, tuple::Tuple T>
            requires(std::tuple_size_v<T> == 0)
        std::expected<T, std::string> read(const auto &)
        {
            return {};
        }

        template <typename Interface>
        std::string write()
        {
            return {};
        }

        template <typename Interface, typename T>
        std::string write(T &&value)
        {
            return Interface::template write<T>(std::forward<T>(value));
        }

        template <typename Interface, typename... Ts>
        std::string write(arguments<Ts...> value)
        {
            std::vector<std::string> rtn;
            rtn.reserve(sizeof...(Ts));

            auto unpack = [&]<typename... Us>(Us &&...args)
            {
                (rtn.emplace_back(write(std::forward<Us>(args))), ...);
            };
            std::apply(unpack, std::move(value.tuple()));

            return rtn | std::views::join_with(',') | std::ranges::to<std::string>();
        }
    } // namespace impl

    template <typename Interface>
    serializer<Interface>::serializer()
    {
        static_assert(saucer::Interface<Interface>, "Serializer does not fullfill requirements!");
    }

    template <typename Interface>
    template <typename T>
    auto serializer<Interface>::convert(T &&callable)
    {
        using resolver = traits::resolver<T>;

        using converter = resolver::converter;
        using executor  = resolver::executor;
        using args      = resolver::args;

        return [converted = converter::convert(std::forward<T>(callable))](std::unique_ptr<function_data> data,
                                                                           serializer_core::executor exec) mutable
        {
            const auto &message = *static_cast<Interface::function_data *>(data.get());
            auto parsed         = impl::read<Interface, args>(message);

            if (!parsed)
            {
                return std::invoke(exec.reject, impl::write<Interface>(parsed.error()));
            }

            auto resolve = [resolve = std::move(exec.resolve)]<typename... Ts>(Ts &&...value)
            {
                std::invoke(resolve, impl::write<Interface>(std::forward<Ts>(value)...));
            };

            auto reject = [reject = std::move(exec.reject)]<typename... Ts>(Ts &&...value)
            {
                std::invoke(reject, impl::write<Interface>(std::forward<Ts>(value)...));
            };

            auto converted_exec = executor{std::move(resolve), std::move(reject)};
            auto params         = std::tuple_cat(std::move(parsed.value()), std::make_tuple(std::move(converted_exec)));

            std::apply(converted, std::move(params));
        };
    }

    template <typename Interface>
    template <Readable<Interface> T>
    auto serializer<Interface>::resolve(coco::promise<T> promise)
    {
        return [promise = std::move(promise)](std::unique_ptr<result_data> data) mutable
        {
            const auto &res = *static_cast<Interface::result_data *>(data.get());

            if constexpr (!std::is_void_v<T>)
            {
                auto parsed = impl::read<Interface, T>(res);

                if (!parsed)
                {
                    auto exception = std::runtime_error{parsed.error()};
                    auto ptr       = std::make_exception_ptr(exception);

                    promise.set_exception(ptr);
                    return;
                }

                promise.set_value(parsed.value());
            }
            else
            {
                promise.set_value();
            }
        };
    }

    template <typename Interface>
    template <Writable<Interface> T>
    auto serializer<Interface>::serialize(T &&value)
    {
        return impl::write<Interface>(std::forward<T>(value));
    }

    template <typename T>
    struct is_serializer : std::is_base_of<serializer<T>, T>
    {
    };
} // namespace saucer

#pragma once

#include "serializer.hpp"

#include "format/args.hpp"
#include "format/unquoted.hpp"

#include "../utils/tuple.hpp"
#include "../traits/traits.hpp"

#include <ranges>

namespace saucer
{
    namespace detail
    {
        template <typename T>
        using result = serializer_core::result<T>;

        template <typename T, typename Interface>
        struct is_writable : std::false_type
        {
        };

        template <typename T, typename Interface>
        concept Writable = requires(T value) {
            { Interface::write(value) } -> std::same_as<std::string>;
        };

        template <typename T, typename Interface>
            requires Writable<T, Interface>
        struct is_writable<T, Interface> : std::true_type
        {
        };

        template <typename... Ts, typename Interface>
            requires(Writable<Ts, Interface> && ...)
        struct is_writable<arguments<Ts...>, Interface> : std::true_type
        {
        };

        template <typename Interface>
        struct is_writable<unquoted_t, Interface> : std::true_type
        {
        };

        template <typename T, typename Interface, typename Buffer>
        struct is_readable : std::false_type
        {
        };

        template <typename T, typename Interface, typename Buffer>
        concept Readable = requires(Buffer value) {
            { Interface::template read<T>(value) } -> std::same_as<serializer_core::result<T>>;
        };

        template <typename T, typename Interface, typename Buffer>
            requires Readable<T, Interface, Buffer>
        struct is_readable<T, Interface, Buffer> : std::true_type
        {
        };

        template <typename Interface, typename Buffer>
        struct is_readable<void, Interface, Buffer> : std::true_type
        {
        };

        template <typename T>
        struct is_serializer : std::is_base_of<serializer<T>, T>
        {
        };

        template <>
        struct is_serializer<void> : std::true_type
        {
        };

        template <typename Interface, typename T>
        struct reader
        {
            static result<T> read(const auto &value)
            {
                return Interface::template read<T>(value);
            }
        };

        template <typename Interface, tuple::Tuple T>
            requires(std::tuple_size_v<T> == 0)
        struct reader<Interface, T>
        {
            static result<T> read(auto &&)
            {
                return {};
            }
        };

        template <typename Interface>
        struct reader<Interface, void>
        {
            static result<void> read(auto &&)
            {
                return {};
            }
        };

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

        template <typename Interface>
        std::string write(unquoted_t &&value) // NOLINT(*-not-moved)
        {
            return std::move(value.str);
        }

        template <typename Interface>
        std::string write(unquoted_t &)
        {
            static_assert(false, "Do not pass `saucer::unquoted` as l-value reference!");
            return {};
        }

        template <typename Interface, typename... Ts>
        std::string write(arguments<Ts...> value)
        {
            std::vector<std::string> rtn;
            rtn.reserve(sizeof...(Ts));

            auto unpack = [&]<typename... Us>(Us &&...args)
            {
                (rtn.emplace_back(write<Interface>(std::forward<Us>(args))), ...);
            };
            std::apply(unpack, std::move(value.tuple));

            return rtn | std::views::join_with(',') | std::ranges::to<std::string>();
        }
    } // namespace detail

    template <typename Interface>
    serializer<Interface>::serializer()
    {
        static_assert(saucer::Interface<Interface>, "Serializer does not fullfill requirements!");
    }

    template <typename Interface>
    template <typename T>
    auto serializer<Interface>::convert(T &&callable) // NOLINT(*-std-forward)
    {
        using resolver = traits::resolver<T>;

        using args        = resolver::args;
        using executor    = resolver::executor;
        using transformer = resolver::transformer;
        using reader      = detail::reader<Interface, args>;

        static_assert(transformer::valid, "Could not transform callable. Please refer to the documentation on how to expose functions!");

        return [converted = transformer{std::forward<T>(callable)}](std::unique_ptr<function_data> data,
                                                                    serializer_core::executor exec) mutable
        {
            const auto &message = *static_cast<Interface::function_data *>(data.get());
            auto parsed         = reader::read(message);

            if (!parsed.has_value())
            {
                return exec.reject(detail::write<Interface>(parsed.error()));
            }

            auto resolve = [resolve = std::move(exec.resolve)]<typename... Ts>(Ts &&...value)
            {
                resolve(detail::write<Interface>(std::forward<Ts>(value)...));
            };

#if defined(__cpp_exceptions) && !defined(SAUCER_NO_EXCEPTIONS)
            auto except = [reject = exec.reject](const std::exception_ptr &ptr)
            {
                try
                {
                    std::rethrow_exception(ptr);
                }
                catch (std::exception &ex)
                {
                    reject(detail::write<Interface>(ex.what()));
                }
                catch (...)
                {
                    reject(detail::write<Interface>("Unknown Exception"));
                }
            };
#endif

            auto reject = [reject = std::move(exec.reject)]<typename... Ts>(Ts &&...value)
            {
                reject(detail::write<Interface>(std::forward<Ts>(value)...));
            };

            auto transformed_exec = executor{std::move(resolve), std::move(reject)};
            auto params           = std::tuple_cat(
#if defined(__cpp_exceptions) && !defined(SAUCER_NO_EXCEPTIONS)
                std::make_tuple(std::move(except)),
#endif
                std::move(*parsed), std::make_tuple(std::move(transformed_exec)));

            std::apply(converted, std::move(params));
        };
    }

    template <typename Interface>
    template <Readable<Interface> T>
    auto serializer<Interface>::resolve(coco::promise<result<T>> promise)
    {
        using reader           = detail::reader<Interface, T>;
        using exception_reader = detail::reader<Interface, std::string>;

        return [promise = std::move(promise)](result<std::unique_ptr<result_data>> data) mutable
        {
            if (!data.has_value())
            {
                return promise.set_value(std::unexpected{data.error()});
            }

            const auto &res = *static_cast<Interface::result_data *>(data->get());

            if (!res.exception) [[likely]]
            {
                return promise.set_value(reader::read(res));
            }

            auto exception = exception_reader::read(res);
            auto error     = exception.has_value() ? *exception : exception.error();

            promise.set_value(std::unexpected{error});
        };
    }

    template <typename Interface>
    template <Writable<Interface> T>
    auto serializer<Interface>::serialize(T &&value)
    {
        return detail::write<Interface>(std::forward<T>(value));
    }
} // namespace saucer

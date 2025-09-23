#include "serializers/rflpp/rflpp.hpp"

#include <optional>

namespace rfl
{
    using namespace saucer::serializers::rflpp;

    template <>
    struct Reflector<function_data>
    {
        struct ReflType
        {
            rfl::Rename<"saucer:call", bool> tag;
            std::size_t id;
            std::string name;
            rfl::Generic params;
        };

        static function_data to(const ReflType &v) noexcept
        {
            function_data rtn;

            rtn.id     = v.id;
            rtn.name   = v.name;
            rtn.params = v.params;

            return rtn;
        }
    };

    template <>
    struct Reflector<result_data>
    {
        struct ReflType
        {
            rfl::Rename<"saucer:resolve", bool> tag;
            std::size_t id;
            rfl::Generic result;
        };

        static result_data to(const ReflType &v) noexcept
        {
            result_data rtn;

            rtn.id     = v.id;
            rtn.result = v.result;

            return rtn;
        }
    };
} // namespace rfl

namespace saucer::serializers::rflpp
{
    serializer::~serializer() = default;

    std::string serializer::script() const
    {
        return {};
    }

    std::string serializer::js_serializer() const
    {
        return "JSON.stringify";
    }

    template <typename T>
    std::optional<T> parse_as(std::string_view buffer)
    {
        auto result = rfl::json::read<T>(buffer);

        if (!result)
        {
            return std::nullopt;
        }

        return result.value();
    }

    serializer::parse_result serializer::parse(std::string_view data) const
    {
        if (auto res = parse_as<function_data>(data); res.has_value())
        {
            return std::make_unique<function_data>(res.value());
        }

        if (auto res = parse_as<result_data>(data); res.has_value())
        {
            return std::make_unique<result_data>(res.value());
        }

        return std::monostate{};
    }
} // namespace saucer::serializers::rflpp

#include "serializers/json.hpp"

namespace saucer::serializers
{
    json_function_data::~json_function_data() = default;

    json_result_data::~json_result_data() = default;

    json::~json() = default;

    std::string json::script() const
    {
        return "";
    }

    std::string json::js_serializer() const
    {
        return "JSON.stringify";
    }

    std::unique_ptr<message_data> json::parse(const std::string &data) const
    {
        auto parsed = nlohmann::json::parse(data, nullptr, false);

        if (parsed.is_discarded())
        {
            return nullptr;
        }

        if (!parsed["id"].is_number_integer())
        {
            return nullptr;
        }

        if (parsed["params"].is_array() && parsed["name"].is_string())
        {
            auto rtn = std::make_unique<json_function_data>();

            rtn->id = parsed["id"];
            rtn->data = parsed["params"];
            rtn->function = parsed["name"];

            return rtn;
        }

        if (!parsed["result"].is_discarded())
        {
            auto rtn = std::make_unique<json_result_data>();

            rtn->id = parsed["id"];
            rtn->data = parsed["result"];

            return rtn;
        }

        return nullptr;
    }
} // namespace saucer::serializers
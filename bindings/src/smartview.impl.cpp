#include "smartview.impl.hpp"

#include "serializer.impl.hpp"

#include <saucer/serializers/errors/serialize.hpp>

saucer_handle::saucer_handle(saucer_serializer *serializer, const saucer::options &options)
    : saucer::smartview_core(std::unique_ptr<saucer::serializer>(serializer), options)
{
    using function = saucer::serializer::function;
    using resolver = saucer::serializer::resolver;

    m_function = [serializer](saucer::function_data &data) -> function::result_type
    {
        auto &message = static_cast<saucer_function_data &>(data);
        auto *result  = serializer->m_function(&message);

        if (auto *expected = dynamic_cast<saucer_parse_result_expected *>(result); expected)
        {
            return expected->code;
        }
        else if (auto *unexpected = dynamic_cast<saucer_parse_result_unexpected *>(result); unexpected)
        {
            return tl::make_unexpected(std::move(unexpected->error));
        }

        return tl::make_unexpected(std::make_unique<saucer::errors::serialize>());
    };

    m_resolver = [serializer](saucer::result_data &data) -> resolver::result_type
    {
        auto &message = static_cast<saucer_result_data &>(data);
        serializer->m_resolver(&message);
    };
}

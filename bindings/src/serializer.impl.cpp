#include "serializer.impl.hpp"

#include "utils.hpp"

std::string saucer_serializer::script() const
{
    return m_script;
}

std::string saucer_serializer::js_serializer() const
{
    return m_js_serializer;
}

saucer_serializer::parse_result saucer_serializer::parse(const std::string &data) const
{
    auto *rtn = cast(m_parser(data.c_str()));
    return parse_result{rtn};
}

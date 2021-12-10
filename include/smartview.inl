#pragma once
#include "smartview.hpp"

namespace saucer
{
    template <typename serializer_t, typename func_t> void smartview::expose(const std::string &name, const func_t &func)
    {
        if (!m_serializers.count(typeid(serializer_t)))
        {
            m_serializers.emplace(typeid(serializer_t), std::make_shared<serializer_t>());
        }

        add_callback(m_serializers.at(typeid(serializer_t)), name, serializer_t::encode_function(func));
    }
} // namespace saucer
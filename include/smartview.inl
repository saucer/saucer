#pragma once
#include "smartview.hpp"

namespace saucer
{
    template <typename serializer_t, typename func_t> void smartview::expose(const std::string &name, const func_t &func)
    {
        if (!m_serializers.count(typeid(serializer_t)))
        {
            const auto &serializer = m_serializers.emplace(typeid(serializer_t), std::make_shared<serializer_t>()).first->second;
            inject(serializer->initialization_script(), load_time_t::creation);
        }

        add_callback(m_serializers.at(typeid(serializer_t)), name, serializer_t::encode_function(func));
    }

    template <typename default_serializer>
    template <typename serializer_t, typename func_t>
    void simple_smartview<default_serializer>::expose(const std::string &name, const func_t &func)
    {
        smartview::expose<serializer_t>(name, func);
    };
} // namespace saucer
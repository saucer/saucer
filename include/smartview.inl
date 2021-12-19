#pragma once
#include "smartview.hpp"

namespace saucer
{
    template <typename serializer_t, typename func_t> void smartview::expose(const std::string &name, const func_t &func, bool async)
    {
        if (!m_serializers.count(typeid(serializer_t)))
        {
            const auto &serializer = m_serializers.emplace(typeid(serializer_t), std::make_shared<serializer_t>()).first->second;
            inject(serializer->initialization_script(), load_time_t::creation);
        }

        add_callback(m_serializers.at(typeid(serializer_t)), name, serializer_t::serialize_function(func), async);
    }

    template <typename rtn_t, typename serializer_t, typename... params_t> auto smartview::eval(const std::string &code, params_t &&...params)
    {
        if (!m_serializers.count(typeid(serializer_t)))
        {
            const auto &serializer = m_serializers.emplace(typeid(serializer_t), std::make_shared<serializer_t>()).first->second;
            inject(serializer->initialization_script(), load_time_t::creation);
        }

        auto rtn = std::make_shared<promise<rtn_t>>(m_creation_thread);
        add_eval(m_serializers.at(typeid(serializer_t)), rtn, serializer_t::resolve_function(rtn), fmt::vformat(code, serializer_t::serialize_arguments(params...)));

        return rtn;
    }

    template <typename default_serializer>
    template <typename serializer_t, typename func_t>
    void simple_smartview<default_serializer>::expose(const std::string &name, const func_t &func, bool async)
    {
        smartview::expose<serializer_t>(name, func, async);
    };

    template <typename default_serializer>
    template <typename rtn_t, typename serializer_t, typename... params_t>
    auto simple_smartview<default_serializer>::eval(const std::string &code, params_t &&...params)
    {
        return smartview::eval<rtn_t, serializer_t>(code, std::forward<params_t>(params)...);
    };
} // namespace saucer
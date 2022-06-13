#pragma once
#include "smartview.hpp"

namespace saucer
{
    template <typename Module, typename... Args> void smartview::add_module(Args &&...args)
    {
        m_modules.emplace_back(std::make_unique<Module>(*this, std::forward<Args>(args)...));
    }

    template <typename Return, typename Serializer, typename... Params> std::shared_ptr<promise<Return>> smartview::eval(const std::string &code, Params &&...params)
    {
        if (!m_serializers.read()->count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.write()->emplace(typeid(Serializer), std::make_unique<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        auto rtn = std::make_shared<promise<Return>>(m_creation_thread);
        add_eval(typeid(Serializer), rtn, Serializer::resolve_promise(rtn), fmt::vformat(code, Serializer::serialize_arguments(params...)));

        return rtn;
    }

    template <typename Serializer, typename Function> void smartview::expose(const std::string &name, const Function &func, bool async)
    {
        if (!m_serializers.read()->count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.write()->emplace(typeid(Serializer), std::make_unique<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        add_callback(typeid(Serializer), name, Serializer::serialize_function(func), async);
    }

    template <typename DefaultSerializer>
    template <typename Serializer, typename Function>
    void simple_smartview<DefaultSerializer>::expose(const std::string &name, const Function &func, bool async)
    {
        smartview::expose<Serializer>(name, func, async);
    }

    template <typename DefaultSerializer>
    template <typename Return, typename Serializer, typename... Params>
    std::shared_ptr<promise<Return>> simple_smartview<DefaultSerializer>::eval(const std::string &code, Params &&...params)
    {
        return smartview::eval<Return, Serializer>(code, std::forward<Params>(params)...);
    }
} // namespace saucer
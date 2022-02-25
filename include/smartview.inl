#pragma once
#include "smartview.hpp"

namespace saucer
{
    template <typename Module> void smartview::add_module()
    {
        if (!m_modules.count(typeid(Module)))
        {
            m_modules.emplace(typeid(Module), std::make_shared<Module>(*this));
        }
    }

    template <typename Module> std::shared_ptr<Module> smartview::get_module()
    {
        if (m_modules.count(typeid(Module)))
        {
            return std::dynamic_pointer_cast<Module>(m_modules.at(typeid(Module)));
        }
        return nullptr;
    }

    template <typename Return, typename Serializer, typename... Params> auto smartview::eval(const std::string &code, Params &&...params)
    {
        if (!m_serializers.count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.emplace(typeid(Serializer), std::make_shared<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        auto rtn = std::make_shared<promise<Return>>(m_creation_thread);
        add_eval(typeid(Serializer), rtn, Serializer::resolve_promise(rtn), fmt::vformat(code, Serializer::serialize_arguments(params...)));

        return rtn;
    }

    template <typename Serializer, typename T> void smartview::expose(const std::string &name, variable<T> &variable, bool async)
    {
        if (!m_serializers.count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.emplace(typeid(Serializer), std::make_shared<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        auto [getter, setter] = Serializer::serialize_variable(variable);

        add_variable(typeid(Serializer), name, getter, setter, async);
        variable.template on<variable_event::updated>([name, this]() { on_variable_updated(name); });
    }

    template <typename Serializer, typename Function> void smartview::expose(const std::string &name, const Function &func, bool async)
    {
        if (!m_serializers.count(typeid(Serializer)))
        {
            const auto &serializer = m_serializers.emplace(typeid(Serializer), std::make_shared<Serializer>()).first->second;
            inject(serializer->initialization_script(), load_time::creation);
        }

        add_callback(typeid(Serializer), name, Serializer::serialize_function(func), async);
    }

    template <typename DefaultSerializer>
    template <typename Serializer, typename T>
    void simple_smartview<DefaultSerializer>::expose(const std::string &name, variable<T> &variable, bool async)
    {
        smartview::expose<Serializer>(name, variable, async);
    };

    template <typename DefaultSerializer>
    template <typename Serializer, typename Function>
    void simple_smartview<DefaultSerializer>::expose(const std::string &name, const Function &func, bool async)
    {
        smartview::expose<Serializer>(name, func, async);
    };

    template <typename DefaultSerializer>
    template <typename Return, typename Serializer, typename... Params>
    auto simple_smartview<DefaultSerializer>::eval(const std::string &code, Params &&...params)
    {
        return smartview::eval<Return, Serializer>(code, std::forward<Params>(params)...);
    };
} // namespace saucer
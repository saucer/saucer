#include <modules/module.hpp>
#include <smartview.hpp>

namespace saucer
{
    module::~module() = default;
    module::module(std::string &&name, std::string &&version, smartview &smartview) :m_name(name), m_version(version), m_smartview(smartview) {}

    std::string module::get_name() const
    {
        return m_name;
    }

    std::string module::get_version() const
    {
        return m_version;
    }
} // namespace saucer
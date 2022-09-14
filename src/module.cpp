#include "module/module.hpp"
#include "smartview.hpp"

namespace saucer
{
    module::~module() = default;
    module::module(smartview &smartview) :m_smartview(smartview) {}
} // namespace saucer
#include "modules/module.hpp"

namespace saucer
{
    module::~module() = default;

    module::module(smartview_core *core) :core(core) {}
} // namespace saucer

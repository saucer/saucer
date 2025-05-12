#include "app.hpp"

#include <cassert>
#include <lockpp/lock.hpp>

namespace saucer
{
    auto &instance()
    {
        static lockpp::lock<std::weak_ptr<application>> instance;
        return instance;
    }

    std::shared_ptr<application> application::init(const options &options)
    {
        auto locked = instance().write();
        auto rtn    = locked->lock();

        if (!rtn)
        {
            assert(!options.id->empty() && "Expected non empty ID");
            rtn.reset(new application{options});
            *locked = rtn;
        }

        return rtn;
    }

    std::shared_ptr<application> application::active()
    {
        auto locked = instance().read();
        return locked->lock();
    }
} // namespace saucer

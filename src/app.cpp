#include "app.hpp"

#include <cassert>
#include <lockpp/lock.hpp>

namespace saucer
{
    poolparty::pool<> &application::pool()
    {
        return m_pool;
    }

    std::shared_ptr<application> application::acquire(const options &options)
    {
        static lockpp::lock<std::weak_ptr<application>> instance;

        auto locked = instance.write();
        auto rtn    = locked->lock();

        if (!rtn)
        {
            assert(!options.id->empty() && "Expected non empty ID");
            rtn.reset(new application{options});
            *locked = rtn;
        }

        return rtn;
    }
} // namespace saucer

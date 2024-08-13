#include "cocoa.window.impl.hpp"

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}
@end

namespace saucer
{
    bool window::impl::is_thread_safe()
    {
        return impl::application != nullptr;
    }
} // namespace saucer

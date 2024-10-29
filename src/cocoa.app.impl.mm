#include "cocoa.app.impl.hpp"

#include "cocoa.utils.hpp"

namespace saucer
{
    void application::impl::init_menu()
    {
        const utils::autorelease_guard guard{};

        auto *const mainmenu = [[NSMenu new] autorelease];
        {
            auto *const item = [[NSMenuItem new] autorelease];
            auto *const menu = [[NSMenu new] autorelease];

            auto *const name = [[NSProcessInfo processInfo] processName];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:name]
                                                      action:@selector(terminate:)
                                               keyEquivalent:@"q"] autorelease]];

            [mainmenu addItem:item];
            [mainmenu setSubmenu:menu forItem:item];
        }
        {
            auto *const item = [[NSMenuItem new] autorelease];
            auto *const menu = [[[NSMenu alloc] initWithTitle:@"Edit"] autorelease];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(undo:)
                                               keyEquivalent:@"z"] autorelease]];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:@"Redo" action:@selector(redo:)
                                               keyEquivalent:@"y"] autorelease]];

            [menu addItem:[NSMenuItem separatorItem]];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"] autorelease]];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(copy:)
                                               keyEquivalent:@"c"] autorelease]];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:@"Paste" action:@selector(paste:)
                                               keyEquivalent:@"v"] autorelease]];

            [menu addItem:[NSMenuItem separatorItem]];

            [menu addItem:[[[NSMenuItem alloc] initWithTitle:@"Select All" action:@selector(selectAll:)
                                               keyEquivalent:@"a"] autorelease]];

            [mainmenu addItem:item];
            [mainmenu setSubmenu:menu forItem:item];
        }

        [NSApp setMainMenu:mainmenu];
    }
} // namespace saucer

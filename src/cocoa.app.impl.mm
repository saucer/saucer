#include "cocoa.app.impl.hpp"

namespace saucer
{
    void application::impl::init_menu()
    {
        auto *const mainmenu = [NSMenu new];

        {
            auto *const item = [NSMenuItem new];
            auto *const menu = [NSMenu new];

            auto *const name = [[NSProcessInfo processInfo] processName];

            [menu addItem:[[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:name]
                                                     action:@selector(terminate:)
                                              keyEquivalent:@"q"]];

            [mainmenu addItem:item];
            [mainmenu setSubmenu:menu forItem:item];
        }
        {
            auto *const item = [NSMenuItem new];
            auto *const menu = [[NSMenu alloc] initWithTitle:@"Edit"];

            [menu addItem:[[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(undo:) keyEquivalent:@"z"]];
            [menu addItem:[[NSMenuItem alloc] initWithTitle:@"Redo" action:@selector(redo:) keyEquivalent:@"y"]];

            [menu addItem:[NSMenuItem separatorItem]];

            [menu addItem:[[NSMenuItem alloc] initWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"]];
            [menu addItem:[[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"]];
            [menu addItem:[[NSMenuItem alloc] initWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"]];

            [menu addItem:[NSMenuItem separatorItem]];

            [menu addItem:[[NSMenuItem alloc] initWithTitle:@"Select All"
                                                     action:@selector(selectAll:)
                                              keyEquivalent:@"a"]];

            [mainmenu addItem:item];
            [mainmenu setSubmenu:menu forItem:item];
        }

        [NSApp setMainMenu:mainmenu];
    }
} // namespace saucer

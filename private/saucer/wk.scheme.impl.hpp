#pragma once

#include <saucer/scheme.hpp>

#include "stash.impl.hpp"
#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>
#include <lockpp/lock.hpp>

namespace saucer::scheme
{
    using task_ref = utils::objc_obj<id<WKURLSchemeTask>>;

    struct request::impl
    {
        task_ref task;
    };

    struct deferred_task
    {
        deferred_task(task_ref);

      public:
        deferred_task(const deferred_task &)     = delete;
        deferred_task(deferred_task &&) noexcept = default;

      public:
        ~deferred_task();

      public:
        task_ref task;
    };

    struct stash_stream : stash::impl
    {
        struct native;

      public:
        std::shared_ptr<native> platform;

      public:
        stash_stream();

      public:
        [[nodiscard]] stash::span data() const override;

      public:
        [[nodiscard]] std::size_t type() const override;
        [[nodiscard]] std::unique_ptr<impl> clone() const override;
    };

    struct stash_stream::native
    {
        using variant = std::variant<stash::vec, deferred_task>;

      public:
        lockpp::lock<variant> data;

      public:
        void attach(deferred_task &&);
    };
} // namespace saucer::scheme

@interface SchemeHandler : NSObject <WKURLSchemeHandler>
{
  @public
    std::unordered_map<WKWebView *, saucer::scheme::resolver> m_callbacks;
    lockpp::lock<std::unordered_map<NSUInteger, saucer::scheme::task_ref>> m_tasks;
}
- (void)add_callback:(saucer::scheme::resolver)callback webview:(WKWebView *)instance;
- (void)del_callback:(WKWebView *)instance;
@end

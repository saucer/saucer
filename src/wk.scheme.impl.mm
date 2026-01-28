#include "wk.scheme.impl.hpp"

#include <dispatch/dispatch.h>

using namespace saucer;
using namespace saucer::scheme;

stream_writer::stream_writer(std::shared_ptr<impl> impl) : m_impl(std::move(impl)) {}

stream_writer::stream_writer(const stream_writer &) = default;

stream_writer::stream_writer(stream_writer &&) noexcept = default;

stream_writer::~stream_writer() = default;

void stream_writer::start(const stream_response &response)
{
    if (!m_impl || m_impl->started.exchange(true) || m_impl->finished)
    {
        return;
    }

    task_ref task_copy;
    {
        auto tasks = m_impl->tasks->write();
        if (!tasks->contains(m_impl->handle))
        {
            m_impl->started = false;
            return;
        }
        task_copy = tasks->at(m_impl->handle);
    }

    auto mime_copy    = response.mime;
    auto headers_copy = response.headers;
    auto status_copy  = response.status;

    dispatch_async(dispatch_get_main_queue(), ^{
        const utils::autorelease_guard guard{};

        auto *const headers = [[[NSMutableDictionary<NSString *, NSString *> alloc] init] autorelease];

        for (const auto &[key, value] : headers_copy)
        {
            [headers setObject:[NSString stringWithUTF8String:value.c_str()]
                        forKey:[NSString stringWithUTF8String:key.c_str()]];
        }

        auto *const mime = [NSString stringWithUTF8String:mime_copy.c_str()];
        [headers setObject:mime forKey:@"Content-Type"];

        auto *const res = [[[NSHTTPURLResponse alloc] initWithURL:task_copy.get().request.URL
                                                       statusCode:status_copy
                                                      HTTPVersion:nil
                                                     headerFields:headers] autorelease];

        @try
        {
            [task_copy.get() didReceiveResponse:res];
        }
        @catch (NSException *)
        {
        }
    });
}

void stream_writer::write(stash data)
{
    if (!m_impl || !m_impl->started || m_impl->finished)
    {
        return;
    }

    task_ref task_copy;
    {
        auto tasks = m_impl->tasks->read();
        if (!tasks->contains(m_impl->handle))
        {
            return;
        }
        task_copy = tasks->at(m_impl->handle);
    }

    auto data_copy = std::vector<std::uint8_t>(data.data(), data.data() + data.size());

    dispatch_async(dispatch_get_main_queue(), ^{
        const utils::autorelease_guard guard{};

        auto *const ns_data = [NSData dataWithBytes:data_copy.data()
                                             length:static_cast<NSInteger>(data_copy.size())];

        @try
        {
            [task_copy.get() didReceiveData:ns_data];
        }
        @catch (NSException *)
        {
        }
    });
}

void stream_writer::finish()
{
    if (!m_impl || !m_impl->started || m_impl->finished.exchange(true))
    {
        return;
    }

    task_ref task_copy;
    {
        auto tasks = m_impl->tasks->write();
        if (!tasks->contains(m_impl->handle))
        {
            return;
        }
        task_copy = tasks->at(m_impl->handle);
        tasks->erase(m_impl->handle);
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        const utils::autorelease_guard guard{};

        @try
        {
            [task_copy.get() didFinish];
        }
        @catch (NSException *)
        {
        }
    });
}

void stream_writer::reject(error err)
{
    if (!m_impl || m_impl->finished.exchange(true))
    {
        return;
    }

    task_ref task_copy;
    {
        auto tasks = m_impl->tasks->write();
        if (!tasks->contains(m_impl->handle))
        {
            return;
        }
        task_copy = tasks->at(m_impl->handle);
        tasks->erase(m_impl->handle);
    }

    auto err_code = std::to_underlying(err);

    dispatch_async(dispatch_get_main_queue(), ^{
        const utils::autorelease_guard guard{};

        @try
        {
            [task_copy.get() didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                                                  code:err_code
                                                              userInfo:nil]];
        }
        @catch (NSException *)
        {
        }
    });
}

bool stream_writer::valid() const
{
    if (!m_impl)
    {
        return false;
    }

    auto tasks = m_impl->tasks->read();
    return tasks->contains(m_impl->handle) && !m_impl->finished;
}

@implementation SchemeHandler
- (void)add_callback:(saucer::scheme::resolver)callback webview:(WKWebView *)instance
{
    m_callbacks.emplace(instance, std::move(callback));
}

- (void)add_stream_callback:(saucer::scheme::stream_resolver)callback webview:(WKWebView *)instance
{
    m_stream_callbacks.emplace(instance, std::move(callback));
}

- (void)del_callback:(WKWebView *)instance
{
    m_callbacks.erase(instance);
    m_stream_callbacks.erase(instance);
}

- (void)webView:(nonnull WKWebView *)instance startURLSchemeTask:(nonnull id<WKURLSchemeTask>)task
{
    const utils::autorelease_guard guard{};

    if (self->m_stream_callbacks.contains(instance))
    {
        auto ref = task_ref::ref(task);

        auto handle = [&]
        {
            auto locked = self->m_tasks.write();
            return locked->emplace(task.hash, ref).first->first;
        }();

        auto writer_impl    = std::make_shared<stream_writer::impl>();
        writer_impl->task   = ref;
        writer_impl->tasks  = &self->m_tasks;
        writer_impl->handle = handle;

        auto writer = stream_writer{writer_impl};
        auto req    = scheme::request{{ref}};

        return self->m_stream_callbacks[instance](std::move(req), std::move(writer));
    }

    // Regular callback handling
    if (!self->m_callbacks.contains(instance))
    {
        return;
    }

    auto ref = task_ref::ref(task);

    auto handle = [&]
    {
        auto locked = self->m_tasks.write();
        return locked->emplace(task.hash, ref).first->first;
    }();

    auto resolve = [self, handle](const scheme::response &response)
    {
        const utils::autorelease_guard guard{};

        auto tasks = self->m_tasks.write();

        if (!tasks->contains(handle))
        {
            return;
        }

        auto task          = tasks->at(handle);
        const auto content = response.data;

        auto *const data    = [NSData dataWithBytes:content.data() length:static_cast<NSInteger>(content.size())];
        auto *const headers = [[[NSMutableDictionary<NSString *, NSString *> alloc] init] autorelease];

        for (const auto &[key, value] : response.headers)
        {
            [headers setObject:[NSString stringWithUTF8String:value.c_str()] forKey:[NSString stringWithUTF8String:key.c_str()]];
        }

        auto *const mime   = [NSString stringWithUTF8String:response.mime.c_str()];
        auto *const length = [NSString stringWithFormat:@"%zu", content.size()];

        [headers setObject:mime forKey:@"Content-Type"];
        [headers setObject:length forKey:@"Content-Length"];

        auto *const res = [[[NSHTTPURLResponse alloc] initWithURL:task.get().request.URL
                                                       statusCode:response.status
                                                      HTTPVersion:nil
                                                     headerFields:headers] autorelease];

        [task.get() didReceiveResponse:res];
        [task.get() didReceiveData:data];
        [task.get() didFinish];

        tasks->erase(handle);
    };

    auto reject = [self, handle](const scheme::error &error)
    {
        const utils::autorelease_guard guard{};

        auto tasks = self->m_tasks.write();

        if (!tasks->contains(handle))
        {
            return;
        }

        auto task = tasks->at(handle);

        [task.get() didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:std::to_underlying(error) userInfo:nil]];

        tasks->erase(handle);
    };

    auto req      = scheme::request{{ref}};
    auto executor = scheme::executor{std::move(resolve), std::move(reject)};

    return self->m_callbacks[instance](std::move(req), std::move(executor));
}

- (void)webView:(nonnull WKWebView *)webview stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)task
{
    const saucer::utils::autorelease_guard guard{};

    auto tasks = m_tasks.write();
    tasks->erase(task.hash);
}
@end

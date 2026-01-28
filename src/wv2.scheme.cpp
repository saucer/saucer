#include "wv2.scheme.impl.hpp"

#include "win32.utils.hpp"

#include <shlwapi.h>

namespace saucer::scheme
{
    void stream_buffer::push(const std::uint8_t *data, std::size_t size)
    {
        {
            std::lock_guard lock{m_mutex};
            m_buffer.insert(m_buffer.end(), data, data + size);
        }
        m_cv.notify_one();
    }

    void stream_buffer::close_write()
    {
        {
            std::lock_guard lock{m_mutex};
            m_finished = true;
        }
        m_cv.notify_one();
    }

    HRESULT stream_buffer::QueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IUnknown || riid == IID_IStream || riid == IID_ISequentialStream)
        {
            *ppv = static_cast<IStream *>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG stream_buffer::AddRef() { return InterlockedIncrement(&m_ref); }

    ULONG stream_buffer::Release()
    {
        auto ref = InterlockedDecrement(&m_ref);
        if (ref == 0)
        {
            delete this;
        }
        return ref;
    }

    HRESULT stream_buffer::Read(void *pv, ULONG cb, ULONG *pcbRead)
    {
        std::unique_lock lock{m_mutex};
        m_cv.wait(lock, [this] { return !m_buffer.empty() || m_finished; });

        if (m_buffer.empty())
        {
            if (pcbRead)
            {
                *pcbRead = 0;
            }
            return m_finished ? S_FALSE : S_OK;
        }

        auto to_read = std::min(static_cast<std::size_t>(cb), m_buffer.size());
        std::copy_n(m_buffer.begin(), to_read, static_cast<std::uint8_t *>(pv));
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<std::ptrdiff_t>(to_read));

        if (pcbRead)
        {
            *pcbRead = static_cast<ULONG>(to_read);
        }
        return S_OK;
    }

    HRESULT stream_buffer::Write(const void *, ULONG, ULONG *) { return E_NOTIMPL; }
    HRESULT stream_buffer::Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *) { return E_NOTIMPL; }
    HRESULT stream_buffer::SetSize(ULARGE_INTEGER) { return E_NOTIMPL; }
    HRESULT stream_buffer::CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) { return E_NOTIMPL; }
    HRESULT stream_buffer::Commit(DWORD) { return E_NOTIMPL; }
    HRESULT stream_buffer::Revert() { return E_NOTIMPL; }
    HRESULT stream_buffer::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    HRESULT stream_buffer::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }

    HRESULT stream_buffer::Stat(STATSTG *pstatstg, DWORD)
    {
        if (!pstatstg)
        {
            return E_POINTER;
        }
        std::memset(pstatstg, 0, sizeof(*pstatstg));
        pstatstg->type = STGTY_STREAM;
        return S_OK;
    }

    HRESULT stream_buffer::Clone(IStream **) { return E_NOTIMPL; }

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

        std::vector<std::wstring> headers = {std::format(L"Content-Type: {}", utils::widen(response.mime))};

        for (const auto &[name, value] : response.headers)
        {
            headers.emplace_back(std::format(L"{}: {}", utils::widen(name), utils::widen(value)));
        }

        std::wstring combined;
        for (std::size_t i = 0; i < headers.size(); ++i)
        {
            if (i > 0)
            {
                combined += L"\r\n";
            }
            combined += headers[i];
        }

        ComPtr<ICoreWebView2WebResourceResponse> result;
        m_impl->environment->CreateWebResourceResponse(m_impl->buffer, response.status, L"OK", combined.c_str(), &result);

        m_impl->args->put_Response(result.Get());
        m_impl->deferral->Complete();
    }

    void stream_writer::write(stash data)
    {
        if (!m_impl || !m_impl->started || m_impl->finished)
        {
            return;
        }

        m_impl->buffer->push(data.data(), data.size());
    }

    void stream_writer::finish()
    {
        if (!m_impl || !m_impl->started || m_impl->finished.exchange(true))
        {
            return;
        }

        m_impl->buffer->close_write();
    }

    void stream_writer::reject(error err)
    {
        if (!m_impl || m_impl->finished.exchange(true))
        {
            return;
        }

        if (m_impl->started)
        {
            m_impl->buffer->close_write();
            return;
        }

        auto status = std::to_underlying(err);
        std::wstring phrase;

        switch (err)
        {
            using enum scheme::error;

        case not_found:
            phrase = L"Not Found";
            break;
        case invalid:
            phrase = L"Bad Request";
            break;
        case denied:
            phrase = L"Unauthorized";
            break;
        default:
            break;
        }

        ComPtr<ICoreWebView2WebResourceResponse> result;
        m_impl->environment->CreateWebResourceResponse(nullptr, status, L"", phrase.c_str(), &result);

        m_impl->args->put_Response(result.Get());
        m_impl->deferral->Complete();
    }

    bool stream_writer::valid() const
    {
        return m_impl && !m_impl->finished;
    }
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::request(request &&) noexcept = default;

    request::~request() = default;

    url request::url() const
    {
        utils::string_handle raw;
        m_impl->request->get_Uri(&raw.reset());
        return unwrap_safe(url::parse(utils::narrow(raw.get())));
    }

    std::string request::method() const
    {
        utils::string_handle raw;
        m_impl->request->get_Method(&raw.reset());

        return utils::narrow(raw.get());
    }

    stash request::content() const
    {
        if (!m_impl->body)
        {
            return stash::empty();
        }

        return stash::from(utils::read(m_impl->body.Get()));
    }

    std::map<std::string, std::string> request::headers() const
    {
        ComPtr<ICoreWebView2HttpRequestHeaders> headers;
        m_impl->request->get_Headers(&headers);

        ComPtr<ICoreWebView2HttpHeadersCollectionIterator> it;
        headers->GetIterator(&it);

        std::map<std::string, std::string> rtn;
        BOOL has_header{};

        while ((it->get_HasCurrentHeader(&has_header), has_header))
        {
            utils::string_handle header;
            utils::string_handle value;

            it->GetCurrentHeader(&header.reset(), &value.reset());
            rtn.emplace(utils::narrow(header.get()), utils::narrow(value.get()));

            BOOL has_next{};
            it->MoveNext(&has_next);
        }

        return rtn;
    }
} // namespace saucer::scheme

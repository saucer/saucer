#include "wv2.scheme.impl.hpp"

namespace saucer::scheme
{
    using Microsoft::WRL::Make;

    stash_stream::stash_stream() : platform(std::make_shared<native>(Make<custom_stream>())) {}

    std::size_t stash_stream::type() const
    {
        return id_of<stash_stream>();
    }

    std::unique_ptr<stash::impl> stash_stream::clone() const
    {
        return std::make_unique<stash_stream>(*this);
    }

    stash::span stash_stream::data() const
    {
        return {};
    }

    // See: https://github.com/MicrosoftEdge/WebView2Feedback/issues/3519
    // This is essentially a dummy implementation as it does not work currently.
    // WebView2 reads the entire IStream passed to it before passing the data along to the Network-Stack.
    // This means that the client receives only one final / accumulated chunk instead of multiple smaller ones as the data is written.

    void custom_stream::close()
    {
        std::lock_guard lock{m_mutex};

        m_data.reset();
        m_cv.notify_one();
    }

    bool custom_stream::append(stash::span data)
    {
        std::lock_guard lock{m_mutex};

        if (!m_data.has_value())
        {
            return false;
        }

        m_data->insert_range(m_data->end(), data);
        m_cv.notify_one();

        return true;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Read(void *dest, ULONG requested, ULONG *read)
    {
        auto lock = std::unique_lock{m_mutex};
        m_cv.wait(lock, [this] { return !m_data.has_value() || !m_data->empty(); });

        if (!m_data.has_value())
        {
            *read = 0;
            return S_FALSE;
        }

        const auto size = std::min(m_data->size(), static_cast<std::size_t>(requested));
        *read           = size;

        if (size > 0)
        {
            memcpy(dest, m_data->data(), size);
            m_data->erase(m_data->begin(), m_data->begin() + static_cast<std::ptrdiff_t>(size));
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Write(const void *, ULONG, ULONG *)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::SetSize(ULARGE_INTEGER)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Commit(DWORD)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Revert()
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Stat(STATSTG *, DWORD)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE custom_stream::Clone(IStream **)
    {
        return E_NOTIMPL;
    }

    stash_stream::native::~native()
    {
        stream->close();
    }
} // namespace saucer::scheme

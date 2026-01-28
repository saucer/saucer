#pragma once

#include <saucer/scheme.hpp>

#include <wrl.h>
#include <WebView2.h>

#include <lockpp/lock.hpp>

#include <deque>
#include <mutex>
#include <condition_variable>

namespace saucer::scheme
{
    using Microsoft::WRL::ComPtr;

    struct request::impl
    {
        ComPtr<ICoreWebView2WebResourceRequest> request;
        ComPtr<IStream> body;
    };

    class stream_buffer : public IStream
    {
        LONG m_ref{1};
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::deque<std::uint8_t> m_buffer;
        bool m_finished{false};

      public:
        void push(const std::uint8_t *data, std::size_t size);
        void close_write();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;
        HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead) override;
        HRESULT STDMETHODCALLTYPE Write(const void *, ULONG, ULONG *) override;
        HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *) override;
        HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) override;
        HRESULT STDMETHODCALLTYPE CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) override;
        HRESULT STDMETHODCALLTYPE Commit(DWORD) override;
        HRESULT STDMETHODCALLTYPE Revert() override;
        HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override;
        HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override;
        HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD) override;
        HRESULT STDMETHODCALLTYPE Clone(IStream **) override;
    };

    struct stream_writer::impl
    {
        ComPtr<ICoreWebView2WebResourceRequestedEventArgs> args;
        ComPtr<ICoreWebView2Deferral> deferral;
        ComPtr<ICoreWebView2Environment> environment;
        stream_buffer *buffer{nullptr};
        std::atomic<bool> started{false};
        std::atomic<bool> finished{false};
    };
} // namespace saucer::scheme

#pragma once

#include <saucer/scheme.hpp>

#include "stash.impl.hpp"

#include <wrl.h>
#include <WebView2.h>

#include <mutex>
#include <condition_variable>

namespace saucer::scheme
{
    using Microsoft::WRL::ClassicCom;
    using Microsoft::WRL::ComPtr;
    using Microsoft::WRL::RuntimeClass;
    using Microsoft::WRL::RuntimeClassFlags;

    struct request::impl
    {
        ComPtr<ICoreWebView2WebResourceRequest> request;
        ComPtr<IStream> body;
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

    class custom_stream : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IStream>
    {
        std::optional<stash::vec> m_data{stash::vec{}};

      private:
        std::mutex m_mutex;
        std::condition_variable m_cv;

      public:
        void close();
        bool append(stash::span);

      protected:
        HRESULT STDMETHODCALLTYPE Read(void *, ULONG, ULONG *) override;

      protected:
        HRESULT STDMETHODCALLTYPE Write(const void *, ULONG, ULONG *) override;
        HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *) override;
        HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) override;
        HRESULT STDMETHODCALLTYPE CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) override;
        HRESULT STDMETHODCALLTYPE Commit(DWORD) override;
        HRESULT STDMETHODCALLTYPE Revert() override;
        HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override;
        HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override;
        HRESULT STDMETHODCALLTYPE Stat(STATSTG *, DWORD) override;
        HRESULT STDMETHODCALLTYPE Clone(IStream **) override;
    };

    struct stash_stream::native
    {
        ~native();

      public:
        ComPtr<custom_stream> stream;
    };
} // namespace saucer::scheme

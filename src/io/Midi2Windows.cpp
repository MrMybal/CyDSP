#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "detail/Midi2Backend.h"
#include "detail/WindowsMidiAbi.h"
#include <atomic>
#include <cstring>
// Define SDK property GUIDs in this translation unit only. Keep before devpkey.h.
#include <initguid.h>

#include <devpkey.h>
#include <setupapi.h>
#include <wrl/client.h>
namespace cydsp::detail {
namespace {
using Microsoft::WRL::ComPtr;
struct Apartment {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ~Apartment() {
        if (SUCCEEDED(result))
            CoUninitialize();
    }
};
std::wstring wide(const std::string& text) {
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0);
    if (!count)
        return {};
    std::wstring s(static_cast<std::size_t>(count), 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                        s.data(), count);
    return s;
}
std::string narrow(const std::wstring& s) {
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(),
                                          static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(count), 0);
    if (count)
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()),
                            out.data(), count, nullptr, nullptr);
    return out;
}
Status activate(ComPtr<wms::Transport>& transport, ComPtr<wms::SessionTracker>& tracker) noexcept {
    auto hr = CoCreateInstance(wms::transportClass, nullptr, CLSCTX_INPROC_SERVER,
                               __uuidof(wms::Transport),
                               reinterpret_cast<void**>(transport.GetAddressOf()));
    if (FAILED(hr))
        return Status::device_unavailable;
    hr = transport->Activate(__uuidof(wms::SessionTracker),
                             reinterpret_cast<void**>(tracker.GetAddressOf()));
    if (FAILED(hr) || FAILED(tracker->Initialize()) || !tracker->VerifyConnectivity())
        return Status::device_unavailable;
    return Status::ok;
}
struct WindowsMidi final : NativeMidi2, wms::Callback {
    Apartment apartment;
    ComPtr<wms::Transport> transport;
    ComPtr<wms::SessionTracker> tracker;
    ComPtr<wms::Bidirectional> connection;
    GUID session{};
    bool registered = false;
    std::atomic<ULONG> references{1};
    LARGE_INTEGER frequency{};
    ReceiveMidi2 receive;
    void* context;
    WindowsMidi(ReceiveMidi2 f, void* c) : receive(f), context(c) {
        QueryPerformanceFrequency(&frequency);
    }
    ~WindowsMidi() override {
        if (connection)
            (void)connection->Shutdown();
        connection.Reset();
        if (registered && tracker)
            (void)tracker->RemoveClientSession(session);
        if (tracker)
            (void)tracker->Shutdown();
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override {
        if (!out)
            return E_POINTER;
        *out = nullptr;
        if (id == __uuidof(IUnknown) || id == __uuidof(wms::Callback)) {
            *out = static_cast<wms::Callback*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {
        return references.fetch_add(1) + 1;
    }
    ULONG STDMETHODCALLTYPE Release() override {
        return references.fetch_sub(1) - 1;
    } // Owner retains until Shutdown joins callbacks.
    HRESULT STDMETHODCALLTYPE Receive(unsigned, void* data, UINT size, LONGLONG position,
                                      LONGLONG) override {
        if (!data || size % 4)
            return E_INVALIDARG;
        std::uint64_t ns = 0;
        if (position > 0) {
            const auto ticks = static_cast<std::uint64_t>(position),
                       rate = static_cast<std::uint64_t>(frequency.QuadPart);
            ns = ticks / rate * 1000000000ull + ticks % rate * 1000000000ull / rate;
        }
        std::array<std::uint32_t, 1024> words{};
        if (size > sizeof(words))
            return E_INVALIDARG;
        std::memcpy(words.data(), data, size);
        receive(context, {words.data(), size / 4}, ns);
        return S_OK;
    }
    Status send(const Midi2TimedPacket& p) noexcept override {
        const auto rate = static_cast<std::uint64_t>(frequency.QuadPart);
        const auto whole = p.timestampNs / 1000000000ull, part = p.timestampNs % 1000000000ull;
        if (whole > static_cast<std::uint64_t>(std::numeric_limits<LONGLONG>::max()) / rate)
            return Status::invalid_argument;
        const auto ticks = whole * rate + part * rate / 1000000000ull;
        const auto hr =
            connection->SendMidiMessage(0, const_cast<std::uint32_t*>(p.packet.words.data()),
                                        p.packet.count * 4, static_cast<LONGLONG>(ticks));
        return SUCCEEDED(hr)                                         ? Status::ok
               : hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) ? Status::would_block
                                                                     : Status::io_error;
    }
    Status poll() noexcept override {
        return Status::ok;
    }
    bool schedules() const noexcept override {
        return true;
    }
};
} // namespace
Status enumerateNativeMidi2(std::vector<Midi2EndpointInfo>& result) noexcept {
    try {
        Apartment apartment;
        ComPtr<wms::Transport> transport;
        ComPtr<wms::SessionTracker> tracker;
        auto status = activate(transport, tracker);
        if (status != Status::ok)
            return status;
        struct TrackerGuard {
            wms::SessionTracker* p;
            ~TrackerGuard() {
                (void)p->Shutdown();
            }
        } trackerGuard{tracker.Get()};
        const auto handle = SetupDiGetClassDevsW(&wms::endpointInterface, nullptr, nullptr,
                                                 DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (handle == INVALID_HANDLE_VALUE)
            return Status::io_error;
        struct Guard {
            HDEVINFO h;
            ~Guard() {
                SetupDiDestroyDeviceInfoList(h);
            }
        } guard{handle};
        std::vector<Midi2EndpointInfo> next;
        for (DWORD index = 0;; ++index) {
            SP_DEVICE_INTERFACE_DATA data{};
            data.cbSize = sizeof(data);
            if (!SetupDiEnumDeviceInterfaces(handle, nullptr, &wms::endpointInterface, index,
                                             &data)) {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
                return Status::io_error;
            }
            DWORD size = 0;
            SetupDiGetDeviceInterfaceDetailW(handle, &data, nullptr, 0, &size, nullptr);
            if (!size)
                continue;
            std::vector<std::byte> bytes(size);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(bytes.data());
            detail->cbSize = sizeof(*detail);
            if (!SetupDiGetDeviceInterfaceDetailW(handle, &data, detail, size, nullptr, nullptr))
                continue;
            std::wstring name = detail->DevicePath;
            std::array<wchar_t, 1024> friendly{};
            DEVPROPTYPE type = 0;
            if (SetupDiGetDeviceInterfacePropertyW(
                    handle, &data, &DEVPKEY_DeviceInterface_FriendlyName, &type,
                    reinterpret_cast<PBYTE>(friendly.data()),
                    static_cast<DWORD>(friendly.size() * sizeof(wchar_t)), nullptr, 0) &&
                type == DEVPROP_TYPE_STRING)
                name = friendly.data();
            next.push_back({narrow(detail->DevicePath), narrow(name), Midi2Backend::windowsServices,
                            true, true});
        }
        result = std::move(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
Status createNativeMidi2(const Midi2NativeRequest& r, ReceiveMidi2 receive, void* context,
                         std::unique_ptr<NativeMidi2>& result) noexcept {
    if (r.endpoint.backend != Midi2Backend::windowsServices)
        return Status::unsupported_backend;
    try {
        auto next = std::make_unique<WindowsMidi>(receive, context);
        auto status = activate(next->transport, next->tracker);
        if (status != Status::ok)
            return status;
        if (FAILED(CoCreateGuid(&next->session)))
            return Status::device_unavailable;
        if (FAILED(next->tracker->AddClientSession(next->session, L"CyDSP")))
            return Status::device_unavailable;
        next->registered = true;
        if (FAILED(next->transport->Activate(
                __uuidof(wms::Bidirectional),
                reinterpret_cast<void**>(next->connection.GetAddressOf()))))
            return Status::device_unavailable;
        const auto id = wide(r.endpoint.id);
        if (id.empty())
            return Status::invalid_argument;
        wms::TransportParams params;
        DWORD mmcss = 0;
        if (FAILED(next->connection->Initialize(id.c_str(), &params, &mmcss, next.get(), 0,
                                                next->session)) ||
            params.format != 2)
            return Status::device_unavailable;
        result = std::move(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
} // namespace cydsp::detail

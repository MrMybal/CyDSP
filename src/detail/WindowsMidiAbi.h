#pragma once
// Windows MIDI Services COM declarations adapted from the pinned official IDL.
// Copyright (c) Microsoft Corporation.
// Complete MIT notice: third_party/licenses/windows-midi.txt
#include <unknwn.h>
#include <windows.h>
namespace cydsp::detail::wms {
struct TransportParams {
    unsigned options = 0, format = 2;
    GUID component{};
};
static_assert(sizeof(TransportParams) == 24);
struct __declspec(uuid("EA264200-3328-49E5-8815-73649A8748BE")) Transport : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Activate(REFIID, void**) = 0;
};
struct __declspec(uuid("4D6A29E5-DF4F-4A2D-A923-9B23B3F2D6F6")) Callback : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Receive(unsigned, void*, UINT, LONGLONG, LONGLONG) = 0;
};
struct __declspec(uuid("B89BBB45-7001-4BEA-BBD8-C7CC26E7836C")) Bidirectional : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCWSTR, TransportParams*, DWORD*, Callback*,
                                                 LONGLONG, GUID) = 0;
    virtual HRESULT STDMETHODCALLTYPE Shutdown() = 0;
    virtual HRESULT STDMETHODCALLTYPE SendMidiMessage(unsigned, void*, UINT, LONGLONG) = 0;
};
struct __declspec(uuid("194c2746-3ae5-419a-94d9-20416c7dbefe")) SessionTracker : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Initialize() = 0;
    virtual HRESULT STDMETHODCALLTYPE AddClientSession(GUID, LPCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateClientSessionName(GUID, LPCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveClientSession(GUID) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSessionList(LPWSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Shutdown() = 0;
    virtual BOOL STDMETHODCALLTYPE VerifyConnectivity() = 0;
};
inline constexpr GUID transportClass{
    0x2ba15e4e, 0x5417, 0x4a66, {0x85, 0xb8, 0x2b, 0x22, 0x60, 0xef, 0xbc, 0x84}};
inline constexpr GUID endpointInterface{
    0xe7cce071, 0x3c03, 0x423f, {0x88, 0xd3, 0xf1, 0x04, 0x5d, 0x02, 0x55, 0x2b}};
} // namespace cydsp::detail::wms

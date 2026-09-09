#include "nvapi_private.h"
#include "nvapi_globals.h"
#include "nvapi/nvapi_d3d_low_latency_device.h"
#include "util/util_statuscode.h"
#include "util/util_env.h"
#include "nvapi/nvapi_d3d11_device.h"

using namespace dxvk;

NVAPI_FUNCTION NvAPI_D3D_RegisterDevice(IUnknown* pDev) {
    static constexpr auto n = FUNC;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDev));

    // Just acknowledge the request since there is nothing to do here
    return Ok(n);
}

NVAPI_FUNCTION NvAPI_D3D_GetObjectHandleForResource(IUnknown* pDevice, IUnknown* pResource, NVDX_ObjectHandle* pHandle) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pResource), log::fmt::ptr(pHandle));

    if (!pResource || !pHandle)
        return InvalidArgument(n);

    // Fake-implement with a dumb passthrough, though no other NvAPI entry points
    // we're likely to implement should care about the actual handle value.
    *pHandle = reinterpret_cast<NVDX_ObjectHandle>(pResource);
    return Ok(n, alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_SetResourceHint(IUnknown* pDev, NVDX_ObjectHandle obj, NVAPI_D3D_SETRESOURCEHINT_CATEGORY dwHintCategory, NvU32 dwHintName, NvU32* pdwHintValue) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDev), log::fmt::hnd(obj), dwHintCategory, dwHintName, log::fmt::ptr(pdwHintValue));

    return NoImplementation(n, alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_BeginResourceRendering(IUnknown* pDeviceOrContext, NVDX_ObjectHandle obj, NvU32 Flags) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDeviceOrContext), log::fmt::hnd(obj), log::fmt::flags(Flags));

    // Synchronisation hints for SLI...
    return Ok(n, alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_EndResourceRendering(IUnknown* pDeviceOrContext, NVDX_ObjectHandle obj, NvU32 Flags) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDeviceOrContext), log::fmt::hnd(obj), log::fmt::flags(Flags));

    return Ok(n, alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_GetCurrentSLIState(IUnknown* pDevice, NV_GET_CURRENT_SLI_STATE* pSliState) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedNoSli = false;
    thread_local bool alreadyLoggedOk = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pSliState));

    if (!pDevice || !pSliState)
        return InvalidArgument(n);

    if (pSliState->version != NV_GET_CURRENT_SLI_STATE_VER1 && pSliState->version != NV_GET_CURRENT_SLI_STATE_VER2)
        return IncompatibleStructVersion(n, pSliState->version);

    // The docs don't list NVAPI_NO_ACTIVE_SLI_TOPOLOGY as a return value,
    // but testing on Windows yielded this exact status, it only returns that for D3D9 devices though.
    Com<IDirect3DDevice9> device9;
    if (SUCCEEDED(pDevice->QueryInterface(IID_PPV_ARGS(&device9)))) {
        if (pSliState->version == NV_GET_CURRENT_SLI_STATE_VER2)
            // Tests on Windows show that this is the only field that gets initialized in this case.
            pSliState->numVRSLIGpus = 0;

        return NoActiveSliTopology(n, alreadyLoggedNoSli);
    }

    // Calling it with an object that is neither a D3D9 device, nor a D3D11 device returns INVALID_ARGUMENT.
    // Hello, Fallout New Vegas.
    Com<ID3D11Device> device11;
    if (FAILED(pDevice->QueryInterface(IID_PPV_ARGS(&device11))))
        return InvalidArgument(n);

    switch (pSliState->version) {
        case NV_GET_CURRENT_SLI_STATE_VER1: {
            auto pSliStateV1 = reinterpret_cast<NV_GET_CURRENT_SLI_STATE_V1*>(pSliState);
            // Report that SLI is not available
            pSliStateV1->maxNumAFRGroups = 1;
            pSliStateV1->numAFRGroups = 1;
            pSliStateV1->currentAFRIndex = 0;
            pSliStateV1->nextFrameAFRIndex = 0;
            pSliStateV1->previousFrameAFRIndex = 0;
            pSliStateV1->bIsCurAFRGroupNew = false;
            break;
        }
        case NV_GET_CURRENT_SLI_STATE_VER2:
            // Report that SLI is not available
            pSliState->maxNumAFRGroups = 1;
            pSliState->numAFRGroups = 1;
            pSliState->currentAFRIndex = 0;
            pSliState->nextFrameAFRIndex = 0;
            pSliState->previousFrameAFRIndex = 0;
            pSliState->bIsCurAFRGroupNew = false;
            pSliState->numVRSLIGpus = 0;
            break;
        default:
            break; // Handled above
    }

    return Ok(n, alreadyLoggedOk);
}

NVAPI_FUNCTION NvAPI_D3D_ImplicitSLIControl(IMPLICIT_SLI_CONTROL implicitSLIControl) {
    static constexpr auto n = FUNC;

    if (log::tracing())
        log::trace(n, implicitSLIControl);

    if (implicitSLIControl == ENABLE_IMPLICIT_SLI)
        return Error(n); // No SLI with this implementation

    return Ok(n);
}

NVAPI_FUNCTION NvAPI_D3D1x_GetGraphicsCapabilities(IUnknown* pDevice, NvU32 structVersion, NV_D3D1x_GRAPHICS_CAPS* pGraphicsCaps) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedOk = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), structVersion, log::fmt::ptr(pGraphicsCaps));

    if (!pGraphicsCaps)
        return InvalidArgument(n);

    switch (structVersion) {
        case NV_D3D1x_GRAPHICS_CAPS_VER1: {
            auto pGraphicsCapsV1 = reinterpret_cast<NV_D3D1x_GRAPHICS_CAPS_V1*>(pGraphicsCaps);
            *pGraphicsCapsV1 = {};
            pGraphicsCapsV1->bExclusiveScissorRectsSupported = 0;
            pGraphicsCapsV1->bVariablePixelRateShadingSupported = 0;
            break;
        }
        case NV_D3D1x_GRAPHICS_CAPS_VER2:
            *pGraphicsCaps = {};
            // bFastUAVClearSupported is reported mostly for the sake of DLSS.
            // All NVIDIA Vulkan drivers support this.
            pGraphicsCaps->bFastUAVClearSupported = 1;
            // dummy SM version number (unused by DLSS):
            pGraphicsCaps->majorSMVersion = 0;
            pGraphicsCaps->minorSMVersion = 0;
            pGraphicsCaps->bExclusiveScissorRectsSupported = 0;
            pGraphicsCaps->bVariablePixelRateShadingSupported = 0;
            break;
        default:
            return IncompatibleStructVersion(n, structVersion);
    }

    return Ok(n, alreadyLoggedOk);
}

NVAPI_FUNCTION NvAPI_D3D1x_Present(IUnknown* pDevice, IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedDeviceBusy = false;
    thread_local bool alreadyLoggedOk = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pSwapChain), SyncInterval, Flags);

    if (!pSwapChain)
        return InvalidArgument(n);

    switch (pSwapChain->Present(SyncInterval, Flags)) {
        case S_OK:
            return Ok(n, alreadyLoggedOk);
        case DXGI_STATUS_OCCLUDED:
        case DXGI_ERROR_DEVICE_RESET:
        case DXGI_ERROR_DEVICE_REMOVED:
        case MAKE_HRESULT(1, 0x876, 2160): // 0x88760870, D3DDDIERR_DEVICEREMOVED
            return DeviceBusy(n, alreadyLoggedDeviceBusy);
        default:
            return Error(n);
    }
}

NVAPI_FUNCTION NvAPI_D3D_Sleep(IUnknown* pDevice) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedOk = false;
    thread_local bool alreadyLoggedNoImplementation = false;
    thread_local bool alreadyLoggedError = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDevice)
        return InvalidArgument(n);

    if (env::needsUnsupportedLowLatencyDevice())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    auto lowLatencyDevice = NvapiD3dLowLatencyDevice::GetOrCreate(pDevice);
    if (!lowLatencyDevice || !lowLatencyDevice->SupportsLowLatency())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    switch (lowLatencyDevice->LatencySleep()) {
        case S_OK:
            return Ok(n, alreadyLoggedOk);
        case E_NOTIMPL:
            return NoImplementation(n, alreadyLoggedNoImplementation);
        default:
            return Error(n, alreadyLoggedError);
    }
}

NVAPI_FUNCTION NvAPI_D3D_SetSleepMode(IUnknown* pDevice, NV_SET_SLEEP_MODE_PARAMS* pSetSleepModeParams) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedOk = false;
    thread_local bool alreadyLoggedNoImplementation = false;
    thread_local bool alreadyLoggedError = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pSetSleepModeParams));

    static bool lastLowLatencyMode = false;
    static uint32_t lastMinimumIntervalUs = UINT32_MAX;

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDevice || !pSetSleepModeParams)
        return InvalidArgument(n);

    if (pSetSleepModeParams->version != NV_SET_SLEEP_MODE_PARAMS_VER1)
        return IncompatibleStructVersion(n, pSetSleepModeParams->version);

    if (env::needsUnsupportedLowLatencyDevice())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    auto lowLatencyDevice = NvapiD3dLowLatencyDevice::GetOrCreate(pDevice);
    if (!lowLatencyDevice || !lowLatencyDevice->SupportsLowLatency())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    switch (lowLatencyDevice->SetLatencySleepMode(pSetSleepModeParams->bLowLatencyMode, pSetSleepModeParams->bLowLatencyBoost, pSetSleepModeParams->minimumIntervalUs)) {
        case S_OK:
            if (lastLowLatencyMode != pSetSleepModeParams->bLowLatencyMode || lastMinimumIntervalUs != pSetSleepModeParams->minimumIntervalUs) {
                lastLowLatencyMode = pSetSleepModeParams->bLowLatencyMode;
                lastMinimumIntervalUs = pSetSleepModeParams->minimumIntervalUs;
                return Ok(str::format(n, " (", pSetSleepModeParams->bLowLatencyMode ? (str::format("Enabled/", pSetSleepModeParams->minimumIntervalUs, "us")) : "Disabled", ")"));
            }
            return Ok(n, alreadyLoggedOk);
        case E_NOTIMPL:
            return NoImplementation(n, alreadyLoggedNoImplementation);
        default:
            return Error(n, alreadyLoggedError);
    }
}

NVAPI_FUNCTION NvAPI_D3D_GetSleepStatus(IUnknown* pDevice, NV_GET_SLEEP_STATUS_PARAMS* pGetSleepStatusParams) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedOk = false;
    thread_local bool alreadyLoggedNoImplementation = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pGetSleepStatusParams));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDevice || !pGetSleepStatusParams)
        return InvalidArgument(n);

    if (pGetSleepStatusParams->version != NV_GET_SLEEP_STATUS_PARAMS_VER1)
        return IncompatibleStructVersion(n, pGetSleepStatusParams->version);

    if (env::needsUnsupportedLowLatencyDevice())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    auto lowLatencyDevice = NvapiD3dLowLatencyDevice::GetOrCreate(pDevice);
    if (!lowLatencyDevice || !lowLatencyDevice->SupportsLowLatency())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    pGetSleepStatusParams->bLowLatencyMode = lowLatencyDevice->GetLowLatencyMode();

    return Ok(n, alreadyLoggedOk);
}

NVAPI_FUNCTION NvAPI_D3D_GetLatency(IUnknown* pDev, NV_LATENCY_RESULT_PARAMS* pGetLatencyParams) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedOk = false;
    thread_local bool alreadyLoggedNoImplementation = false;
    thread_local bool alreadyLoggedError = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDev), log::fmt::ptr(pGetLatencyParams));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDev || !pGetLatencyParams)
        return InvalidArgument(n);

    if (pGetLatencyParams->version != NV_LATENCY_RESULT_PARAMS_VER1)
        return IncompatibleStructVersion(n, pGetLatencyParams->version);

    if (env::needsUnsupportedLowLatencyDevice())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    auto lowLatencyDevice = NvapiD3dLowLatencyDevice::GetOrCreate(pDev);
    if (!lowLatencyDevice || !lowLatencyDevice->SupportsLowLatency())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    switch (lowLatencyDevice->GetLatencyInfo(reinterpret_cast<D3D_LATENCY_RESULTS*>(pGetLatencyParams))) {
        case S_OK:
            return Ok(n, alreadyLoggedOk);
        case E_NOTIMPL:
            return NoImplementation(n, alreadyLoggedNoImplementation);
        default:
            return Error(n, alreadyLoggedError);
    }
}

NVAPI_FUNCTION NvAPI_D3D_SetLatencyMarker(IUnknown* pDev, NV_LATENCY_MARKER_PARAMS* pSetLatencyMarkerParams) {
    static constexpr auto n = FUNC;
    thread_local bool alreadyLoggedOk = false;
    thread_local bool alreadyLoggedNoImplementation = false;
    thread_local bool alreadyLoggedError = false;
    thread_local bool alreadyLoggedMarkerTypeNotSupported = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDev), log::fmt::nv_latency_marker_params(pSetLatencyMarkerParams));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDev || !pSetLatencyMarkerParams)
        return InvalidArgument(n);

    if (pSetLatencyMarkerParams->version != NV_LATENCY_MARKER_PARAMS_VER1)
        return IncompatibleStructVersion(n, pSetLatencyMarkerParams->version);

    if (env::needsUnsupportedLowLatencyDevice())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    auto lowLatencyDevice = NvapiD3dLowLatencyDevice::GetOrCreate(pDev);
    if (!lowLatencyDevice || !lowLatencyDevice->SupportsLowLatency())
        return NoImplementation(n, alreadyLoggedNoImplementation);

    auto markerType = NvapiD3dLowLatencyDevice::ToMarkerType(pSetLatencyMarkerParams->markerType);
    if (!markerType.has_value()) {
        // Silently drop unsupported marker types
        if (!std::exchange(alreadyLoggedMarkerTypeNotSupported, true))
            log::info(str::format("Not supported NV_LATENCY_MARKER_TYPE: ", pSetLatencyMarkerParams->markerType));

        return Ok(n, alreadyLoggedOk);
    }

    switch (lowLatencyDevice->SetLatencyMarker(pSetLatencyMarkerParams->frameID, markerType.value())) {
        case S_OK:
            return Ok(n, alreadyLoggedOk);
        case E_NOTIMPL:
            return NoImplementation(n, alreadyLoggedNoImplementation);
        default:
            return Error(n, alreadyLoggedError);
    }
}

// ---------------------------------------------------------------------------
// Simultaneous Multi-Projection (SMP) / Multi-View support
//
// These entry points let an application ask whether multiview rendering is
// available and switch the mode on.  iRacing uses them for its "Nvidia
// Simultaneous Multi-Projection" option on triple screens.
//
// The work is done by DXVK, so multiview availability depends on both the GPU
// generation and the DXVK build.
//
// Six entry points of query and set, the multiview pair, the SPS pair, the
// ModifiedW pair.  Only multiview is implemented at this time.  The others
// exist as the shape for the next person who may be looking at VR
// implementation.
//
// All three queries follow the same rule, reporting supported only if the
// stack can deliver and ModifiedW gates the check on DXVK multiview instead of
// the card capabilities.
// ---------------------------------------------------------------------------

// Resolves the adapter behind a device or context, so capability queries can
// answer from the GPU actually in use rather than the first one enumerated.
//
// Architecture thresholds, matching NVIDIA's Windows driver:
//   Single-Pass Stereo (2 views)   Pascal (GTX 10xx) or newer
//   Multi-View (up to 4 views)     Turing (RTX 20xx) or newer
//
// Architecture is necessary but not sufficient: see
// NvAPI_D3D_QueryMultiViewSupport and NvAPI_D3D_QueryModifiedWSupport,
// which also require DXVK to implement the interfaces that do the work.
static NvapiAdapter* GetMultiViewAdapter(IUnknown* pDeviceOrContext) {
    if (!pDeviceOrContext || !nvapiAdapterRegistry)
        return nullptr;

    // The game may pass an ID3D11Device or an ID3D11DeviceContext,
    // normalise to the device first
    Com<ID3D11Device> device;
    if (FAILED(pDeviceOrContext->QueryInterface(IID_PPV_ARGS(&device)))) {
        Com<ID3D11DeviceContext> context;
        if (FAILED(pDeviceOrContext->QueryInterface(IID_PPV_ARGS(&context))))
            return nullptr;

        context->GetDevice(&device);
    }

    // Walk D3D11 device -> DXGI device -> DXGI adapter -> adapter LUID,
    // then look that LUID up in dxvk-nvapi's adapter registry
    Com<IDXGIDevice> dxgiDevice;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
        return nullptr;

    Com<IDXGIAdapter> dxgiAdapter;
    if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter)))
        return nullptr;

    DXGI_ADAPTER_DESC adapterDesc{};
    if (FAILED(dxgiAdapter->GetDesc(&adapterDesc)))
        return nullptr;

    return nvapiAdapterRegistry->FindAdapter(adapterDesc.AdapterLuid);
}

// Multi-view needs Turing or newer for the hardware, and DXVK's revision 2
// interfaces for anything to act on it. Both are required.  Every entry point
// that depends on multi-view availability answers from here, so a capability
// query and the setter behind it cannot reach different conclusions.
static bool IsMultiViewAvailable(IUnknown* pDeviceOrContext) {
    auto adapter = GetMultiViewAdapter(pDeviceOrContext);
    if (!adapter || adapter->GetArchitectureId() < NV_GPU_ARCHITECTURE_TU100)
        return false;

    auto device = NvapiD3d11Device::GetOrCreate(pDeviceOrContext);
    return device && device->SupportsMultiview();
}

NVAPI_FUNCTION NvAPI_D3D_QueryMultiViewSupport(IUnknown* pDevice, NV_QUERY_MULTIVIEW_SUPPORT_PARAMS* pQueryMultiViewSupportedParams) {
    constexpr auto n = __func__;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pQueryMultiViewSupportedParams));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDevice || !pQueryMultiViewSupportedParams)
        return InvalidArgument(n);

    if (pQueryMultiViewSupportedParams->version != NV_QUERY_MULTIVIEW_SUPPORT_PARAMS_VER1)
        return IncompatibleStructVersion(n, pQueryMultiViewSupportedParams->version);

    // Capability has to describe the whole stack, not just the card.  The
    // hardware threshold is necessary but not sufficient: the work is done by
    // DXVK, so without its multi-view interfaces this would be telling an
    // application a feature is available that nothing would act on.
    auto multiViewSupported = IsMultiViewAvailable(pDevice);

    // To ensure all fields in the caller's struct are populated they are all
    // written to for every call.  Single-Pass Stereo is not implemented, so
    // it is reported unsupported regardless of the card. See
    // NvAPI_D3D_QuerySinglePassStereoSupport.
    pQueryMultiViewSupportedParams->bMultiViewSupported = multiViewSupported;
    pQueryMultiViewSupportedParams->bSinglePassStereoSupported = false;
    pQueryMultiViewSupportedParams->bSinglePassStereoXYZWSupported = false;

    return Ok(str::format(n, " (MultiView=", multiViewSupported ? "supported" : "unsupported", ")"), alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_SetMultiViewMode(IUnknown* pDevOrContext, NV_MULTIVIEW_PARAMS* pMultiViewParams) {
    constexpr auto n = __func__;
    thread_local bool alreadyLogged = false;
    thread_local bool alreadyLoggedNoImplementation = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevOrContext), log::fmt::ptr(pMultiViewParams));

    if (!pDevOrContext || !pMultiViewParams)
        return InvalidArgument(n);

    if (pMultiViewParams->version != NV_MULTIVIEW_PARAMS_VER1)
        return IncompatibleStructVersion(n, pMultiViewParams->version);

    if (pMultiViewParams->numViews == 0 || pMultiViewParams->numViews > NV_MULTIVIEW_MAX_SUPPORTED_VIEWS)
        return InvalidArgument(n);

    // Nothing to forward to where the capability query reports unsupported,
    // and forwarding anyway would have the two disagree.
    if (!IsMultiViewAvailable(pDevOrContext))
        return NoImplementation(n, alreadyLoggedNoImplementation);

    // Forward the toggle to DXVK when it implements the extended interfaces
    if (auto device = NvapiD3d11Device::GetOrCreate(pDevOrContext);
        device && device->SetMultiviewMode(pMultiViewParams->numViews, pMultiViewParams->independentViewportMaskEnable != 0))
        return Ok(str::format(n, " (numViews=", pMultiViewParams->numViews, ") (forwarded)"), alreadyLogged);

    // No DXVK multi-view support, so nothing can act on this. Refuse rather
    // than returning OK: an application has no way to detect a lie, and
    // NvAPI_D3D_QueryMultiViewSupport reports unsupported in the same
    // case, so this is the consistent answer rather than a surprising one.
    return NoImplementation(n, alreadyLoggedNoImplementation);
}

NVAPI_FUNCTION NvAPI_D3D_QuerySinglePassStereoSupport(IUnknown* pDevice, NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS* pQuerySinglePassStereoSupportedParams) {
    constexpr auto n = __func__;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevice), log::fmt::ptr(pQuerySinglePassStereoSupportedParams));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDevice || !pQuerySinglePassStereoSupportedParams)
        return InvalidArgument(n);

    // Single-Pass Stereo is not implemented, so this reports false whatever
    // the card can do. Reporting the hardware capability instead would send
    // an application down a path that NvAPI_D3D_SetSinglePassStereoMode then
    // refuses, which is worse than a plain no.
    //
    // The route in is short, and matches what multi-view already does here:
    // add the entry points to ID3D11VkExtDevice2 / ID3D11VkExtContext2 in
    // DXVK, forward through NvapiD3d11Device the way SetMultiviewMode does,
    // and report true once the device confirms support. Contributions welcome,
    // VR titles are the obvious beneficiary.
    switch (pQuerySinglePassStereoSupportedParams->version) {
        case NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_VER1: {
            auto pParamsV1 = reinterpret_cast<NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V1*>(pQuerySinglePassStereoSupportedParams);
            pParamsV1->bSinglePassStereoSupported = false;
            break;
        }
        case NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_VER2:
            pQuerySinglePassStereoSupportedParams->bSinglePassStereoSupported = false;
            pQuerySinglePassStereoSupportedParams->bSinglePassStereoXYZWSupported = false;
            break;
        default:
            return IncompatibleStructVersion(n, pQuerySinglePassStereoSupportedParams->version);
    }

    return Ok(str::format(n, " (SPS=unsupported, not implemented)"), alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_SetSinglePassStereoMode(IUnknown* pDevOrContext, NvU32 numViews, NvU32 renderTargetIndexOffset, NvU8 independentViewportMaskEnable) {
    constexpr auto n = __func__;
    thread_local bool alreadyLoggedNoImplementation = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevOrContext), numViews, renderTargetIndexOffset, independentViewportMaskEnable);

    if (!pDevOrContext || numViews == 0)
        return InvalidArgument(n);

    // Not implemented, and NvAPI_D3D_QuerySinglePassStereoSupport says so, so
    // a well-behaved application should never reach here. Refusing keeps the
    // two consistent for one that does anyway.
    return NoImplementation(n, alreadyLoggedNoImplementation);
}

NVAPI_FUNCTION NvAPI_D3D_QueryModifiedWSupport(IUnknown* pDev, NV_QUERY_MODIFIED_W_SUPPORT_PARAMS* pQueryModifiedWSupportedParams) {
    constexpr auto n = __func__;
    thread_local bool alreadyLogged = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDev), log::fmt::ptr(pQueryModifiedWSupportedParams));

    if (!nvapiAdapterRegistry)
        return ApiNotInitialized(n);

    if (!pDev || !pQueryModifiedWSupportedParams)
        return InvalidArgument(n);

    if (pQueryModifiedWSupportedParams->version != NV_QUERY_MODIFIED_W_SUPPORT_PARAMS_VER1)
        return IncompatibleStructVersion(n, pQueryModifiedWSupportedParams->version);

    // Modified-W (Lens Matched Shading) needs Pascal or newer
    auto pascalOrNewer = false;
    if (auto adapter = GetMultiViewAdapter(pDev))
        pascalOrNewer = adapter->GetArchitectureId() >= NV_GPU_ARCHITECTURE_GP100;

    // iRacing's SMP toggle queries this, FALSE greys the toggle
    // and TRUE brings it back. iRacing will use SetModifiedWMode when SMP is
    // on and Multiview is not actually available.
    // Pascal is the Modified-W hardware requirement, but the gate here only
    // holds while an application that sees this reported supported takes the
    // multi-view route instead and never enters the Modified-W path. Below
    // Turing multi-view is unavailable, so that no longer holds and this must
    // report unsupported.
    auto modifiedWSupported = pascalOrNewer && IsMultiViewAvailable(pDev);

    pQueryModifiedWSupportedParams->bModifiedWSupported = modifiedWSupported;

    return Ok(str::format(n, " (ModifiedW=", modifiedWSupported ? "supported" : "unsupported", ")"), alreadyLogged);
}

NVAPI_FUNCTION NvAPI_D3D_SetModifiedWMode(IUnknown* pDevOrContext, NV_MODIFIED_W_PARAMS* psModifiedWParams) {
    constexpr auto n = __func__;
    thread_local bool alreadyLoggedNoImplementation = false;

    if (log::tracing())
        log::trace(n, log::fmt::ptr(pDevOrContext), log::fmt::ptr(psModifiedWParams));

    if (!pDevOrContext || !psModifiedWParams)
        return InvalidArgument(n);

    if (psModifiedWParams->version != NV_MODIFIED_W_PARAMS_VER1)
        return IncompatibleStructVersion(n, psModifiedWParams->version);

    if (psModifiedWParams->numEntries > NV_MODIFIED_W_MAX_VIEWPORTS)
        return InvalidArgument(n);

    // iRacing's SMP capability checks route here for calls only when
    // multiview is unavailable.  With a stock DXVK for SMP there
    // will be thousands of calls per session with the
    // coefficient shape of A=-2.45156, 0, +2.45156 and B=0,
    // in w' = w + Ax + By for a W warp which appears to be symmetrical
    // but this is due to how iRacing interprets one value entered for
    // side monitor angles, change this in the ini file and these values
    // change.  Nothing is implemented with the gate in
    // QueryModifiedWSupport checking SupportsMultiview() meaning this
    // cannot be reached but the refusal remains for consistency and
    // honesty.  A future implementation would be self contained here
    // as the coefficient shape is mentioned above.
    return NoImplementation(n, alreadyLoggedNoImplementation);
}

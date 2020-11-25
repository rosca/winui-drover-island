/*
 *  Copyright 2020 Adobe Systems Incorporated. All rights reserved.
 *  This file is licensed to you under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License. You may obtain a copy
 *  of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under
 *  the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 *  OF ANY KIND, either express or implied. See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 */

#pragma once

#include <Windows.h>
#include <system_error>
#include "winrt/Windows.Foundation.h"

namespace winui_drover_island {

struct Logger {
    static void warn(const std::string& msg);
    static void warn(const char* msg);
};

enum class DpiRounding { kFloor, kRound, kCeiling };
constexpr float kDefaultDpi = 96.0f;

int32_t dipsToPixels(float dips, float dpi, DpiRounding dpiRounding);

inline float pixelsToDips(int32_t pixels, float dpi) {
    return pixels * kDefaultDpi / dpi;
}

int32_t sizeDipsToPixels(float dips, float dpi);

winrt::Windows::Foundation::Rect toRect(RECT const& rect, float dpi);

RECT toRECT(const winrt::Windows::Foundation::Rect& rect, float dpi);

#define ReturnIfFailed(v)   \
    {                       \
        HRESULT __hr = (v); \
        if (FAILED(__hr)) { \
            return __hr;    \
        }                   \
    }

__declspec(noreturn) __declspec(noinline) inline void ThrowHR(HRESULT hr) {
    throw winrt::hresult_error{ hr };
}

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        // Set a breakpoint on this line to catch Win32 API errors.
        ThrowHR(hr);
    }
}

//
// Converts exceptions in the callable code into HRESULTs.
//
__declspec(noinline) inline HRESULT ThrownExceptionToHResult() {
    try {
        throw;
    }
#if TORQ_WINRT
    catch (Platform::Exception^ exc) {
        return exc->HResult;
    }
#endif
    catch (const winrt::hresult_error& e) {
        return e.code();
    }
    catch (std::system_error const& e) {
        return e.code().value();
    }
    catch (...) {
        // rethrow all the other exceptions
        throw;
    }
}

template <typename CALLABLE>
HRESULT ComExceptionBoundary(CALLABLE&& fn) {
    try {
        fn();
        return S_OK;
    }
    catch (...) {
        return ThrownExceptionToHResult();
    }
}

template <typename CALLABLE>
void ComExceptionBoundaryWithLog(CALLABLE&& fn, const char* log) {
    auto hResult = ComExceptionBoundary(std::move(fn));
    if (FAILED(hResult)) {
        Logger::warn("[ComException][CanvasControl] " + std::string(log) + " function has thrown an exception " + std::to_string(hResult));
    }
}

inline void LogIfFailed(HRESULT hResult, const char* log) {
    if (FAILED(hResult)) {
        Logger::warn("[ComErrir][CanvasControl] " + std::string(log) + " function has failed " + std::to_string(hResult));
    }
}

bool isDeviceLostHResult(HRESULT hr);

template <typename T>
inline winrt::com_ptr<T> objectAs(winrt::Windows::Foundation::IInspectable obj) {
    winrt::com_ptr<T> native;
    auto unknown = winrt::get_unknown(obj);
    assert(unknown);
    auto hResult = unknown->QueryInterface(__uuidof(native), native.put_void());
    LogIfFailed(hResult, "unknown->QueryInterface");
    assert(native.get());
    return native;
}

}  // namespace winui_drover_island

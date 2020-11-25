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

#include "pch.h"

#include "./GfxUtils.h"

namespace winui_drover_island {

void Logger::warn(const std::string& msg) {
    warn(msg.c_str());
}
void Logger::warn(const char* msg) {
    OutputDebugStringA(msg);
}

int32_t dipsToPixels(float dips, float dpi, DpiRounding dpiRounding) {
    float scaled = dips * dpi / kDefaultDpi;
    switch (dpiRounding) {
    case DpiRounding::kFloor: scaled = floorf(scaled); break;
    case DpiRounding::kRound: scaled = roundf(scaled); break;
    case DpiRounding::kCeiling: scaled = ceilf(scaled); break;
    }

    return static_cast<int32_t>(scaled);
}

int32_t sizeDipsToPixels(float dips, float dpi) {
    int32_t result = dipsToPixels(dips, dpi, DpiRounding::kRound);

    // Zero versus non-zero is pretty important for things like control sizes, so we want
    // to avoid ever rounding non-zero input sizes down to zero during conversion to pixels.
    // If the input value was small but positive, it's safer to round up to one instead.
    if (result == 0 && dips > 0) {
        return 1;
    }

    return result;
}

winrt::Windows::Foundation::Rect toRect(RECT const& rect, float dpi) {
    auto x = pixelsToDips(rect.left, dpi);
    auto y = pixelsToDips(rect.top, dpi);
    auto width = pixelsToDips(rect.right - rect.left, dpi);
    auto height = pixelsToDips(rect.bottom - rect.top, dpi);

    winrt::Windows::Foundation::Rect rc;
    rc.X = x;
    rc.Y = y;
    rc.Width = width;
    rc.Height = height;
    return rc;
}

RECT toRECT(const winrt::Windows::Foundation::Rect& rect, float dpi) {
    auto left = dipsToPixels(rect.X, dpi, DpiRounding::kRound);
    auto top = dipsToPixels(rect.Y, dpi, DpiRounding::kRound);
    auto right = dipsToPixels(rect.X + rect.Width, dpi, DpiRounding::kRound);
    auto bottom = dipsToPixels(rect.Y + rect.Height, dpi, DpiRounding::kRound);

    if (right == left && rect.Width > 0)
        right++;

    if (bottom == top && rect.Height > 0)
        bottom++;

    return RECT{ left, top, right, bottom };
}

bool isDeviceLostHResult(HRESULT hr) {
    switch (hr) {
    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DEVICE_RESET:
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    case DXGI_ERROR_INVALID_CALL:
    case D2DERR_RECREATE_TARGET: return true;

    default: return false;
    }
}

}  // namespace winui_drover_island

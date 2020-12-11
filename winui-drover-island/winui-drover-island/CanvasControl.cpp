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

#include "pch.h"

#include "./CanvasControl.h"
#include "./GfxUtils.h"

#include <cassert>
#include <ddraw.h>

#include <algorithm>

#include "winrt/base.h"
#include "winrt/Microsoft.UI.Xaml.Automation.Peers.h"
#include "winrt/Microsoft.System.h"

#include "./GfxUtils.h"
#include "CanvasControl.g.cpp"

namespace winrt {
using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::System;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI::Xaml::Automation;
}  // namespace winrt


using namespace winui_drover_island;

namespace winrt::winui_drover_island::implementation {

namespace {

class VirtualSurfaceCallback : public winrt::implements<VirtualSurfaceCallback, IVirtualSurfaceUpdatesCallbackNative> {
public:
    explicit VirtualSurfaceCallback(std::function<HRESULT()>&& fn) : callback_(std::move(fn)) {}

    ~VirtualSurfaceCallback() {
        callback_ = nullptr;
    }

    HRESULT UpdatesNeeded() { return callback_(); }

private:
    std::function<HRESULT()> callback_;
};

}

CanvasControl::CanvasControl(bool useVSIS) : containerDpi_(kDefaultDpi), useVSIS_(useVSIS) {
    Image image;
    Content(image);
    image.Stretch(winrt::Stretch::Fill);

    winrt::AutomationProperties::SetAccessibilityView(image, winrt::Peers::AccessibilityView::Raw);

    loadedHandler_ = Loaded(winrt::auto_revoke, { this, &CanvasControl::onContainerLoaded });
    SizeChanged({ this, &CanvasControl::onContainerSizeChanged });
}

CanvasControl::~CanvasControl() {
    if (renderingPending_) {
        removeRenderingCallback();
    }
    resetRenderTarget();
}

void CanvasControl::onContainerLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&) {
    auto container = sender.try_as<FrameworkElement>();
    // We only want to get a single loaded event.
    // In Xaml Loaded / Unloaded event are async, and are not called in a deterministic order,
    // so we don't want to rely on them. We only register all the events on load.
    loadedHandler_.revoke();

    containerDpi_ = static_cast<float>(container.XamlRoot().RasterizationScale() * kDefaultDpi);
    rootChangedHandler_ = container.XamlRoot().Changed(winrt::auto_revoke, {this, &CanvasControl::onRootChanged});

    compositorSurfaceLostHandler_ =
        winrt::CompositionTarget::SurfaceContentsLost(winrt::auto_revoke, {this, &CanvasControl::onCompositorSurfaceContentsLost});

    winrt::DependencyObject parent = container;
    bool isRenderingInDialog = false;
    while (!isRenderingInDialog && parent) {
        parent = winrt::VisualTreeHelper::GetParent(parent);
        if (auto dialog = parent.try_as<winrt::ContentDialog>()) {
            isRenderingInDialog = true;
        }
    }

    loaded_ = true;
    invalidateDueToInternalChange();
}

void CanvasControl::onContainerSizeChanged(
    const winrt::IInspectable&, const winrt::SizeChangedEventArgs& e) {
    auto newSize = e.NewSize();
    if (newSize != containerSize_) {
        containerSize_ = newSize;
        invalidateDueToInternalChange();
    }
}

void CanvasControl::onRootChanged(const XamlRoot& root, const winrt::Microsoft::UI::Xaml::XamlRootChangedEventArgs&) {
    float newDpi = static_cast<float>(root.RasterizationScale() * kDefaultDpi);
    if (newDpi != containerDpi_) {
        containerDpi_ = newDpi;
        invalidateDueToInternalChange();
    }
}

void CanvasControl::setupRenderingCallback() {
    assert(!renderingPending_);
    compositorRenderingHandler_ = winrt::CompositionTarget::Rendering(winrt::auto_revoke, {this, &CanvasControl::onCompositorDraw});
    renderingPending_ = true;
}

void CanvasControl::removeRenderingCallback() {
    assert(renderingPending_);
    compositorRenderingHandler_.revoke();
    renderingPending_ = false;
}

winrt::Image CanvasControl::containerImage() {
    return Content().try_as<winrt::Image>();
}

void CanvasControl::setImageSource(winrt::Imaging::SurfaceImageSource newSource) {
    if (auto image = containerImage()) {
        image.Source(newSource);
    }
}

void CanvasControl::resetImageSource() {
    setImageSource(nullptr);
}

void CanvasControl::setRenderTarget(const RenderTarget& newTarget) {
    auto oldTarget = currentTarget_;
    currentTarget_ = newTarget;

    if (oldTarget.surface_) {
        auto sisNative = objectAs<ISurfaceImageSourceNativeWithD2D>(oldTarget.surface_);
        LogIfFailed(sisNative->SetDevice(nullptr), "sisNative->SetDevice(nullptr)");
    }
    if (newTarget.surface_) {
        auto sisNative = objectAs<ISurfaceImageSourceNativeWithD2D>(newTarget.surface_);
        LogIfFailed(sisNative->SetDevice(nullptr), "sisNative->SetDevice(nullptr)");
        assert(device_.get());
        LogIfFailed(sisNative->SetDevice(device_->d2dDevice().get()), "sisNative->SetDevice(device_)");
    }
}

void CanvasControl::resetRenderTarget() {
    // We don't really expect this to fail, but let's wrap it in a com exception bondary.
    setRenderTarget({});
}

HRESULT CanvasControl::runWithDevice(std::function<HRESULT()>&& fn) {
    if (!device_) {
        device_ = GfxD2DDeviceManager::instance().sharedDevice();
        if (!device_) {
            Logger::warn("Failed to get the shared device");
            return E_FAIL;
        }
        ComExceptionBoundaryWithLog([&] { createResources(device_); }, "createResources");
    }
    HRESULT hr = fn();
    if (FAILED(hr)) {
        if (isDeviceLostHResult(hr) || hr == E_SURFACE_CONTENTS_LOST) {
            handleDeviceLost();
        }
    }
    return hr;
}

std::shared_ptr<GfxD2DDevice> CanvasControl::device() {
    return device_;
}

void CanvasControl::ensureSurfaceImageSource() {
    assert(!asyncResetPending_);
    const auto& newSize = containerSize_;
    const auto& newDpi = containerDpi_;

    bool surfaceNotCreated = (currentTarget_.surface_ == nullptr);
    bool dpiChanged = (currentTarget_.dpi_ != newDpi);
    bool sizeChanged = (currentTarget_.size_ != newSize);
    if (!surfaceNotCreated && !dpiChanged && !sizeChanged) {
        return;
    }
    assert(newSize.Width > 0 && newSize.Height > 0);

    auto actualPixelsWidth = sizeDipsToPixels(newSize.Width, newDpi);
    auto actualPixelsHeight = sizeDipsToPixels(newSize.Height, newDpi);

    auto imgSource = winrt::Imaging::SurfaceImageSource(actualPixelsWidth, actualPixelsHeight, false);

    RenderTarget target{imgSource, newSize, newDpi};
    setRenderTarget(target);
    setImageSource(imgSource);
    assert(currentTarget_.surface_);
}

HRESULT CanvasControl::performD2DDraw(ISurfaceImageSourceNativeWithD2D* sisNative, const RECT& updateRect) {
    assert(!asyncResetPending_);
    winrt::com_ptr<ID2D1DeviceContext> context;
    POINT offset = {};
    ReturnIfFailed(sisNative->BeginDraw(updateRect, __uuidof(context), context.put_void(), &offset));

    const auto dpi = currentTarget_.dpi_;

    offset.x -= updateRect.left;
    offset.y -= updateRect.top;
    float offsetX = pixelsToDips(offset.x, dpi);
    float offsetY = pixelsToDips(offset.y, dpi);

    context->Clear();
    context->SetTransform(D2D1::Matrix3x2F::Translation(offsetX, offsetY));
    context->SetDpi(dpi, dpi);
    context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    // Call user's draw callback
    auto rc = toRect(updateRect, dpi);
    ComExceptionBoundaryWithLog(
        [&]() {
            draw(context, D2D_RECT_F{rc.X, rc.Y, rc.X + rc.Width, rc.Y + rc.Height});
        },
        "draw function");

    return sisNative->EndDraw();
}

HRESULT CanvasControl::performImageSourceDraw() {
    assert(currentTarget_.surface_);

    auto sisNative = objectAs<ISurfaceImageSourceNativeWithD2D>(currentTarget_.surface_);
    auto rc = winrt::Rect{0.f, 0.f, currentTarget_.size_.Width, currentTarget_.size_.Height};
    RECT updateRect = toRECT(rc, currentTarget_.dpi_);
    return performD2DDraw(sisNative.get(), updateRect);
}

void CanvasControl::onCompositorSurfaceContentsLost(const winrt::IInspectable&, const winrt::IInspectable&) {
    handleDeviceLost();
}

void CanvasControl::onCompositorDraw(const winrt::IInspectable&, const winrt::IInspectable&) {
    removeRenderingCallback();

    if (asyncResetPending_) {
        return;
    }

    auto result = runWithDevice([&]() {
        ensureSurfaceImageSource();
        return S_OK;
    });
    if (FAILED(result)) {
        LogIfFailed(result, "ensureSurfaceImageSource");
        return;
    }
    result = runWithDevice([&]() { return performImageSourceDraw(); });

    LogIfFailed(result, "performImageSourceDraw");
}

void CanvasControl::handleDeviceLost() {
    if (device_) {
        ComExceptionBoundaryWithLog([&] { destroyResources(); }, "destroyResources");
        device_.reset();
    }

    postAsyncReset();
}

void CanvasControl::postAsyncReset() {
    // If the device was lost during paint, the stack must be pretty big,
    // and likely to crash if we reset the device during paint,
    // so we are posting an async request for this.
    if (asyncResetPending_) {
        return;
    }
    asyncResetPending_ = true;
    auto wThis = get_weak();
    DispatcherQueue().TryEnqueue(winrt::DispatcherQueuePriority::Normal, [wThis]() {
        if (auto pThis = wThis.get()) {
            pThis->resetNowIfNeeded();
        }
    });
}

void CanvasControl::resetNowIfNeeded() {
    if (!asyncResetPending_) {
        return;
    }
    asyncResetPending_ = false;
    resetRenderTarget();
    resetImageSource();
    invalidateDueToInternalChange();
}

void CanvasControl::invalidateDueToInternalChange() {
    if (!loaded_ || asyncResetPending_) {
        return;
    }

    if (useVSIS_) {
        auto result = runWithDevice([this]() { return ensureVirtualSurfaceImageSource(); });
        LogIfFailed(result, "ensureVirtualSurfaceImageSource");
    }

    invalidate();
}

void CanvasControl::invalidate() {
    if (!loaded_ || asyncResetPending_ || renderingPending_) {
        return;
    }

    if (containerSize_.Width == 0 || containerSize_.Height == 0) {
        return;
    }

    if (useVSIS_) {
        if (currentTarget_.surface_) {
            if (auto vsisNative = currentTarget_.surface_.as<IVirtualSurfaceImageSourceNative>()) {
                winrt::Rect rc{ 0, 0, currentTarget_.size_.Width, currentTarget_.size_.Height };
                LogIfFailed(vsisNative->Invalidate(toRECT(rc, currentTarget_.dpi_)), "Invalidate");
            }
        }
    }
    else {
        setupRenderingCallback();
    }
}


HRESULT CanvasControl::ensureVirtualSurfaceImageSource() {
    assert(useVSIS_);

    const auto& newSize = containerSize_;
    const auto& newDpi = containerDpi_;

    bool surfaceNotCreated = (currentTarget_.surface_ == nullptr);
    bool dpiChanged = (currentTarget_.dpi_ != newDpi);
    bool sizeChanged = (currentTarget_.size_ != newSize);

    if (!surfaceNotCreated && !dpiChanged && !sizeChanged) {
        return S_OK;
    }

    assert(newSize.Width > 0 && newSize.Height > 0);

    auto actualPixelsWidth = sizeDipsToPixels(newSize.Width, newDpi);
    auto actualPixelsHeight = sizeDipsToPixels(newSize.Height, newDpi);

    assert(actualPixelsWidth > 0 && actualPixelsHeight > 0);

    winrt::Imaging::VirtualSurfaceImageSource surface{ nullptr };
    if (surfaceNotCreated) {
        surface = winrt::Imaging::VirtualSurfaceImageSource{ actualPixelsWidth, actualPixelsHeight, false };
    }
    else {
        surface = currentTarget_.surface_.as<winrt::Imaging::VirtualSurfaceImageSource>();
    }

    auto sisNative = objectAs<IVirtualSurfaceImageSourceNative>(surface);
    HRESULT hResult = S_OK;
    if (surfaceNotCreated) {
        auto wThis = get_weak();
        auto callback = winrt::make_self<VirtualSurfaceCallback>([wThis]() -> HRESULT {
            // This function can throw, since the exceptions will be caught in VirtualSurfaceCallback
            auto pThis = wThis.get();

            if (!pThis || !pThis->useVSIS_ || !pThis->currentTarget_.surface_ || pThis->asyncResetPending_) {
                return E_FAIL;
            }
            return pThis->runWithDevice([&]() { return pThis->performVirtualImageSourceDraw(); });
        });
        hResult = sisNative->RegisterForUpdatesNeeded(callback.get());
        RenderTarget target{ surface, newSize, newDpi };
        setRenderTarget(target);
        setImageSource(surface);
    }
    else {
        assert(dpiChanged || sizeChanged);
        assert(!surfaceNotCreated);

        hResult = sisNative->Resize(actualPixelsWidth, actualPixelsHeight);
        if (SUCCEEDED(hResult)) {
            RECT updateRect = { 0, 0, actualPixelsWidth, actualPixelsHeight };
            LogIfFailed(sisNative->Invalidate(updateRect), "sisNative->Invalidate(updateRect)");
            currentTarget_.dpi_ = newDpi;
            currentTarget_.size_ = newSize;
        }
    }

    assert(currentTarget_.surface_);

    return hResult;
}

HRESULT CanvasControl::performVirtualImageSourceDraw() {
    assert(useVSIS_);
    assert(currentTarget_.surface_);
    assert(!asyncResetPending_);

    auto vsisNative = objectAs<IVirtualSurfaceImageSourceNative>(currentTarget_.surface_);
    auto sisNative = vsisNative.as<ISurfaceImageSourceNativeWithD2D>();
    DWORD updateRectCount = 0;
    ReturnIfFailed(vsisNative->GetUpdateRectCount(&updateRectCount));
    if (updateRectCount == 0) {
        return S_OK;
    }

    std::vector<RECT> updateRECTs(updateRectCount);
    ReturnIfFailed(vsisNative->GetUpdateRects(updateRECTs.data(), updateRectCount));

    std::vector<winrt::Rect> updateRects;
    updateRects.reserve(updateRECTs.size());
    std::transform(updateRECTs.begin(), updateRECTs.end(), std::back_inserter(updateRects), [&](const RECT& r) {
        return toRect(r, currentTarget_.dpi_);
    });

    RECT visibleBounds;
    ReturnIfFailed(vsisNative->GetVisibleBounds(&visibleBounds));

    for (auto& updateRect : updateRECTs) {
        ReturnIfFailed(performD2DDraw(sisNative.get(), updateRect));
    }
    return S_OK;
}

}  // namespace winrt::winui_drover_island::implementation

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

#include <d2d1_1.h>

#include <functional>
#include <memory>

#include "CanvasControl.g.h"

#include "./GfxD2DDeviceManager.h"
#include "Microsoft.UI.Xaml.Media.DXInterop.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Microsoft.UI.Xaml.Media.Imaging.h"
#include "winrt/Windows.Foundation.h"

namespace winrt::winui_drover_island::implementation {

class CanvasControl : public CanvasControlT<CanvasControl> {
 protected:
    using Image = Microsoft::UI::Xaml::Controls::Image;
    using XamlRoot = Microsoft::UI::Xaml::XamlRoot;
    using CompositionTarget = Microsoft::UI::Xaml::Media::CompositionTarget;
    using UserControl = Microsoft::UI::Xaml::Controls::UserControl;
    using FrameworkElement = Microsoft::UI::Xaml::FrameworkElement;
    using SurfaceImageSource = Microsoft::UI::Xaml::Media::Imaging::SurfaceImageSource;
    using IInspectable = Windows::Foundation::IInspectable;
    template<typename T>
    using EventHandler = Windows::Foundation::EventHandler<T>;
    using GfxD2DDevice = ::winui_drover_island::GfxD2DDevice;

 public:
    virtual ~CanvasControl();

    void invalidate();

 protected:
    explicit CanvasControl(bool useVSIS);

    virtual void draw(const com_ptr<ID2D1DeviceContext>&, const D2D_RECT_F& updateRect) = 0;
    virtual void createResources(const std::shared_ptr<GfxD2DDevice>&) {}
    virtual void destroyResources() {}

 private:
    std::shared_ptr<GfxD2DDevice> device();

    struct RenderTarget {
        SurfaceImageSource surface_{nullptr};
        Windows::Foundation::Size size_;
        float dpi_ = 0;
    };

    HRESULT runWithDevice(std::function<HRESULT()>&&);

    void onContainerLoaded(const IInspectable& sender, const Microsoft::UI::Xaml::RoutedEventArgs& e);
    void onContainerSizeChanged(const IInspectable& sender, const Microsoft::UI::Xaml::SizeChangedEventArgs& e);
    void onRootChanged(const XamlRoot&, const Microsoft::UI::Xaml::XamlRootChangedEventArgs&);
    void onCompositorDraw(const IInspectable&, const IInspectable&);
    void onCompositorSurfaceContentsLost(const IInspectable&, const IInspectable&);

    Image containerImage();

    void ensureSurfaceImageSource();
    HRESULT performD2DDraw(ISurfaceImageSourceNativeWithD2D* sisNative, const RECT& updateRect);
    HRESULT performImageSourceDraw();
    void setImageSource(SurfaceImageSource source);
    void resetImageSource();
    void setRenderTarget(const RenderTarget&);
    void resetRenderTarget();

    void invalidateDueToInternalChange();
    void postAsyncReset();
    void resetNowIfNeeded();
    void handleDeviceLost();

    void setupRenderingCallback();
    void removeRenderingCallback();

    // Virtual surface specific methods
    HRESULT ensureVirtualSurfaceImageSource();
    HRESULT performVirtualImageSourceDraw();

    FrameworkElement::Loaded_revoker loadedHandler_;
    XamlRoot::Changed_revoker rootChangedHandler_;
    CompositionTarget::Rendering_revoker compositorRenderingHandler_;
    CompositionTarget::SurfaceContentsLost_revoker compositorSurfaceLostHandler_;

    Windows::Foundation::Size containerSize_;
    float containerDpi_ = 0;

    RenderTarget currentTarget_;

    std::shared_ptr<GfxD2DDevice> device_;

    bool renderingPending_ = false;
    bool loaded_ = false;

    bool asyncResetPending_ = false;

    const bool useVSIS_ = false;
};

}  // namespace winui_drover_island

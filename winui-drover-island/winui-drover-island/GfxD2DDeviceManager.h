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

#include <d2d1_2.h>
#include <dxgi1_3.h>
#include <winrt/base.h>

#include <memory>
#include <mutex>
#include <vector>

namespace winui_drover_island {

class GfxD2DContextPool;

class GfxD2DContextLease {
    GfxD2DContextPool* owner_;
    winrt::com_ptr<ID2D1DeviceContext1> deviceContext_;

 public:
    GfxD2DContextLease();
    explicit GfxD2DContextLease(winrt::com_ptr<ID2D1DeviceContext1>&& deviceContext);
    GfxD2DContextLease(GfxD2DContextLease&& other);
    GfxD2DContextLease& operator=(GfxD2DContextLease&& other);

    GfxD2DContextLease(GfxD2DContextLease const&) = delete;
    GfxD2DContextLease& operator=(GfxD2DContextLease const&) = delete;

    ~GfxD2DContextLease();

    const winrt::com_ptr<ID2D1DeviceContext1>& context() const { return deviceContext_; }

 private:
    GfxD2DContextLease(GfxD2DContextPool* owner, winrt::com_ptr<ID2D1DeviceContext1>&& deviceContext);
    void returnLease();

    friend class GfxD2DContextPool;
};

class GfxD2DContextPool {
    ID2D1Device1* d2dDevice_ = nullptr;
    std::mutex mutex_;
    std::vector<winrt::com_ptr<ID2D1DeviceContext1>> deviceContexts_;

 public:
    explicit GfxD2DContextPool(ID2D1Device1* d2dDevice);

    GfxD2DContextPool(GfxD2DContextPool const&) = delete;
    GfxD2DContextPool& operator=(GfxD2DContextPool const&) = delete;

    GfxD2DContextLease takeLease();

    void close();

 private:
    void returnLease(winrt::com_ptr<ID2D1DeviceContext1>&& deviceContext);

    friend class GfxD2DContextLease;
};

class GfxD2DDevice {
 public:
    GfxD2DDevice(const winrt::com_ptr<IDXGIDevice3>&, const winrt::com_ptr<ID2D1Device1>&, bool isSoftware);

    bool isValid() const;

    HRESULT deviceRemovedErrorCode() const;

    static std::shared_ptr<GfxD2DDevice> create(bool software);

    const winrt::com_ptr<IDXGIDevice3>& dxgiDevice() const { return dxgiDevice_; }
    const winrt::com_ptr<ID2D1Device1>& d2dDevice() const { return d2dDevice_; }

    GfxD2DContextLease leaseResourceCreationDeviceContext();

    void trim();
    void close();

    bool isSoftware() const { return isSoftware_; }

    uint32_t maximumBitmapSizeInPixels();

    enum class DebugLevel {
        kNone = 0,
        kError = 1,
        kWarning = 2,
        kInformation = 3,
    };

    static void setDebugLevel(DebugLevel level) { sDebugLevel_ = level; }

 private:
    winrt::com_ptr<IDXGIDevice3> dxgiDevice_;
    winrt::com_ptr<ID2D1Device1> d2dDevice_;
    GfxD2DContextPool contextPool_;

    static DebugLevel sDebugLevel_;

    bool isSoftware_ = false;
};

class GfxD2DDeviceManager {
 public:
    ~GfxD2DDeviceManager();

    static GfxD2DDeviceManager& instance();

    std::shared_ptr<GfxD2DDevice> sharedDevice(bool software = false);

 private:
    GfxD2DDeviceManager();

    std::weak_ptr<GfxD2DDevice> sharedHardwareDevice_;
    std::weak_ptr<GfxD2DDevice> sharedSoftwareDevice_;
    std::mutex mutex_;
};

}  // namespace winui_drover_island

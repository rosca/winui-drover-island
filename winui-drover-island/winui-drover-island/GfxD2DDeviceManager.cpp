/*
 *  Copyright 2020 Adobe Systems Incorporated. All rights reserved.
 *  This file is licensed to you under the Apache License, Version 2.0 (the "License")
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

#include "./GfxD2DDeviceManager.h"

#include <d2d1_2.h>
#include <d3d11.h>
#include <algorithm>

namespace winui_drover_island {

namespace {

winrt::com_ptr<ID2D1Factory2> create2D2Factory(GfxD2DDevice::DebugLevel debugLevel) {
    D2D1_FACTORY_OPTIONS factoryOptions;
    factoryOptions.debugLevel = static_cast<D2D1_DEBUG_LEVEL>(debugLevel);
    winrt::com_ptr<ID2D1Factory2> createdFactory;
    winrt::check_hresult(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory2), &factoryOptions, createdFactory.put_void()));
    return createdFactory;
}

winrt::com_ptr<ID3D11Device> tryCreateD3DDevice(bool useSoftwareDevice, bool useDebugDevice) {
    D3D_DRIVER_TYPE driverType = useSoftwareDevice ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE;
    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (useDebugDevice) {
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    winrt::com_ptr<ID3D11Device> createdDevice;
    auto hr = D3D11CreateDevice(NULL,  // adapter
        driverType,
        NULL,  // software handle
        deviceFlags,
        nullptr,  // feature levels default
        0,        // feature level count
        D3D11_SDK_VERSION,
        createdDevice.put(),
        NULL,
        NULL);
    if (SUCCEEDED(hr)) {
        return createdDevice;
    }
    return {};
}

winrt::com_ptr<ID3D11Device> makeD3D11Device(bool forceSoftware, bool useDebugDevice) {
    winrt::com_ptr<ID3D11Device> d3dDevice;
    auto device = tryCreateD3DDevice(forceSoftware, useDebugDevice);
    if (!device && !forceSoftware) {
        device = tryCreateD3DDevice(true, useDebugDevice);
    }
    assert(device);
    return device;
}

}  // namespace

GfxD2DDevice::DebugLevel GfxD2DDevice::sDebugLevel_ = GfxD2DDevice::DebugLevel::kNone;

GfxD2DDevice::GfxD2DDevice(
    const winrt::com_ptr<IDXGIDevice3>& dxgiDevice, const winrt::com_ptr<ID2D1Device1>& d2dDevice, bool isSoftware)
    : dxgiDevice_(dxgiDevice), d2dDevice_(d2dDevice), contextPool_(d2dDevice.get()), isSoftware_(isSoftware) {}

std::shared_ptr<GfxD2DDevice> GfxD2DDevice::create(bool software) {
    auto d2dFactory = create2D2Factory(sDebugLevel_);
    auto d3dDevice = makeD3D11Device(software, sDebugLevel_ != DebugLevel::kNone);
    assert(d3dDevice);
    winrt::com_ptr<IDXGIDevice3> dxgiDevice;
    dxgiDevice = d3dDevice.as<IDXGIDevice3>();
    winrt::com_ptr<ID2D1Device1> d2dDevice;
    winrt::check_hresult(d2dFactory->CreateDevice(dxgiDevice.get(), d2dDevice.put()));

    return std::make_shared<GfxD2DDevice>(dxgiDevice, d2dDevice, software);
}

GfxD2DContextLease GfxD2DDevice::leaseResourceCreationDeviceContext() {
    return contextPool_.takeLease();
}

uint32_t GfxD2DDevice::maximumBitmapSizeInPixels() {
    auto lease = leaseResourceCreationDeviceContext();
    return lease.context()->GetMaximumBitmapSize();
}

bool GfxD2DDevice::isValid() const {
    return dxgiDevice_.get() && d2dDevice_.get();
}

HRESULT GfxD2DDevice::deviceRemovedErrorCode() const {
    assert(dxgiDevice_.get());
    auto d3dDevice = dxgiDevice_.as<ID3D11Device>();
    return d3dDevice->GetDeviceRemovedReason();
}

void GfxD2DDevice::trim() {
    if (d2dDevice_) {
        d2dDevice_->ClearResources();
    }
    if (dxgiDevice_) {
        dxgiDevice_->Trim();
    }
}

void GfxD2DDevice::close() {
    contextPool_.close();
    dxgiDevice_ = nullptr;
    d2dDevice_ = nullptr;
}

GfxD2DDeviceManager::GfxD2DDeviceManager() {}

GfxD2DDeviceManager::~GfxD2DDeviceManager() {}

GfxD2DDeviceManager& GfxD2DDeviceManager::instance() {
    static GfxD2DDeviceManager obj;
    return obj;
}

std::shared_ptr<GfxD2DDevice> GfxD2DDeviceManager::sharedDevice(bool software) {
    std::unique_lock<std::mutex> guard(mutex_);

    std::shared_ptr<GfxD2DDevice> lostDevice;
    auto device = software ? sharedSoftwareDevice_.lock() : sharedHardwareDevice_.lock();
    if (device) {
        if (!device->isValid()) {
            device.reset();
        } else if (FAILED(device->deviceRemovedErrorCode())) {
            lostDevice = device;
            device.reset();
        }
    }
    if (!device) {
        device = GfxD2DDevice::create(software);
        if (software) {
            sharedSoftwareDevice_ = device;
        } else {
            sharedHardwareDevice_ = device;
        }
    }
    guard.unlock();

    if (lostDevice) {
        // dispatch an event with the device lost.
    }
    assert(device);
    return device;
}

GfxD2DContextLease::GfxD2DContextLease() : owner_(nullptr) {}

GfxD2DContextLease::GfxD2DContextLease(winrt::com_ptr<ID2D1DeviceContext1>&& deviceContext)
    : owner_(nullptr), deviceContext_(std::move(deviceContext)) {}

GfxD2DContextLease::GfxD2DContextLease(GfxD2DContextLease&& other)
    : owner_(other.owner_), deviceContext_(std::move(other.deviceContext_)) {}

GfxD2DContextLease& GfxD2DContextLease::operator=(GfxD2DContextLease&& other) {
    returnLease();
    owner_ = other.owner_;
    deviceContext_ = std::move(other.deviceContext_);
    return *this;
}

GfxD2DContextLease::~GfxD2DContextLease() {
    returnLease();
}

GfxD2DContextLease::GfxD2DContextLease(GfxD2DContextPool* owner, winrt::com_ptr<ID2D1DeviceContext1>&& deviceContext)
    : owner_(owner), deviceContext_(std::move(deviceContext)) {
    assert(owner_);
}

void GfxD2DContextLease::returnLease() {
    if (owner_) {
        owner_->returnLease(std::move(deviceContext_));
        owner_ = nullptr;
    } else {
        deviceContext_ = nullptr;
    }
}

GfxD2DContextPool::GfxD2DContextPool(ID2D1Device1* d2dDevice) : d2dDevice_(d2dDevice) {}

GfxD2DContextLease GfxD2DContextPool::takeLease() {
    std::lock_guard<std::mutex> guard(mutex_);
    assert(d2dDevice_);
    if (deviceContexts_.empty()) {
        winrt::com_ptr<ID2D1DeviceContext1> deviceContext;
        winrt::check_hresult(d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, deviceContext.put()));
        return GfxD2DContextLease(this, std::move(deviceContext));
    }
    GfxD2DContextLease newLease(this, std::move(deviceContexts_.back()));
    deviceContexts_.pop_back();
    return newLease;
}

void GfxD2DContextPool::close() {
    std::lock_guard<std::mutex> guard(mutex_);
    deviceContexts_.clear();
    d2dDevice_ = nullptr;
}

void GfxD2DContextPool::returnLease(winrt::com_ptr<ID2D1DeviceContext1>&& deviceContext) {
    if (!deviceContext) {
        return;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    if (!d2dDevice_) {
        return;
    }
    static const auto kMaxPoolSize = std::max(std::thread::hardware_concurrency(), 1U);
    if (deviceContexts_.size() < kMaxPoolSize) {
        deviceContexts_.emplace_back(std::move(deviceContext));
    }
}

}  // namespace winui_drover_island

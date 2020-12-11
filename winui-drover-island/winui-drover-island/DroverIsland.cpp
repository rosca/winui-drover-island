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

#include "./DroverIsland.h"
#include "./GfxUtils.h"

namespace winrt {
using namespace winrt::Windows::Foundation;
}  // namespace winrt


namespace winui_drover_island {

DroverIsland::DroverIsland(bool useVSIS) : CanvasControl(useVSIS) {

}


void DroverIsland::draw(const winrt::com_ptr<ID2D1DeviceContext>& context, const D2D_RECT_F&) {
	// beginDraw and endDraw are not needed. The DPI, offset translation has been taken care of.
	// It is ok to throw, the control will handle the exceptions.
	
	// Nothing here yet... 😢
	// The code to draw the drover controls will be here.
	context->Clear(D2D1::ColorF(D2D1::ColorF::Red));
}

void DroverIsland::destroyResources() {
}

}  // namespace winrt::winui_drover_island::implementation

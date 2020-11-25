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

#include "./EllipseShape.h"
#include "./GfxUtils.h"

namespace winrt {
using namespace winrt::Windows::Foundation;
}  // namespace winrt


namespace winui_drover_island {

void EllipseShape::draw(const winrt::com_ptr<ID2D1DeviceContext>& context, const D2D_RECT_F& rect) {
	// beginDraw and endDraw are not needed. The DPI, offset translation has been taken care of.
	// It is ok to throw, the control will handle the exceptions.
	context->Clear(D2D1::ColorF(D2D1::ColorF::Aqua, 1.0));
	float rx = (rect.right - rect.left) / 2.f;
	float ry = (rect.bottom - rect.top) / 2.f;
	auto center = D2D1::Point2F(rx, ry);
	auto ellipse = D2D1::Ellipse(center, rx, ry);

	if (!mEllipseBrush) {
		winrt::com_ptr<ID2D1SolidColorBrush> brush;
		ThrowIfFailed(context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Purple), brush.put()));
		mEllipseBrush = brush;
	}

	context->FillEllipse(ellipse, mEllipseBrush.get());
}

void EllipseShape::destroyResources() {
	mEllipseBrush = nullptr;
}

}  // namespace winrt::winui_drover_island::implementation

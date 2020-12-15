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

#include <winrt/Microsoft.UI.Xaml.h>

namespace winui_drover_island {

class DroverIslandDemo {
public:
	DroverIslandDemo();
	winrt::Microsoft::UI::Xaml::UIElement createContent();

private:
	enum class Type { None, Ellipse, DroverSample, Last = DroverSample };

	void renderCanvasControl(Type);
	Type pickNextControl();

	winrt::Microsoft::UI::Xaml::Controls::Grid mGrid{ nullptr };
	winrt::Microsoft::UI::Xaml::Controls::Button::Click_revoker mClickRevoker;
	winrt::Microsoft::UI::Xaml::Controls::CheckBox::Checked_revoker mCheckedRevoker;
	winrt::Microsoft::UI::Xaml::Controls::CheckBox::Unchecked_revoker mUncheckedRevoker;
	Type mControlType = Type::None;
	winrt::Microsoft::UI::Xaml::Controls::Border mCanvasContainer{ nullptr };
	winrt::Microsoft::UI::Xaml::Controls::TextBlock mDescription{ nullptr };
	bool mUseVSIS = false;
};

}
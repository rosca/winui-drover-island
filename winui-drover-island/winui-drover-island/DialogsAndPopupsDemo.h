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

class DialogsAndPopupsDemo {
	using Click_revoker = winrt::Microsoft::UI::Xaml::Controls::Button::Click_revoker;

	Click_revoker mRevokeDialogButton;
	Click_revoker mRevokePopupButton;
	winrt::Microsoft::UI::Xaml::Controls::ContentDialog mDialog;
	winrt::Microsoft::UI::Xaml::Controls::Primitives::Popup mPopup;
	winrt::Microsoft::UI::Xaml::Controls::LayoutPanel mErrorPanel;
	winrt::Microsoft::UI::Xaml::Controls::TextBlock mErrorText;
	winrt::Microsoft::UI::Xaml::Controls::StackPanel mRoot;

	template <typename Fn>
	void tryFn(Fn&&);
public:
	DialogsAndPopupsDemo();
	~DialogsAndPopupsDemo();
	winrt::Microsoft::UI::Xaml::UIElement content() const;
};

}
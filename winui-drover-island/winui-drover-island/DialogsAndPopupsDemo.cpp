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
#include "DialogsAndPopupsDemo.h"

#include <cstdint>

namespace Xaml = ::winrt::Microsoft::UI::Xaml;
using ::winrt::Microsoft::UI::Colors;

namespace {

constexpr double kPhi = 1.618033988749895;
constexpr double kSpacing = 20.0;

void closeEverything(
		Xaml::Controls::UIElementCollection& children,
		Xaml::Controls::ContentDialog& dialog,
		Xaml::Controls::Primitives::Popup& popup,
		Xaml::Controls::LayoutPanel& mErrorPanel) {
	dialog.Hide();

	uint32_t index;
	if (children.IndexOf(popup, index)) {
		popup.IsOpen(false);
		children.RemoveAt(index);
	}
	
	mErrorPanel.Visibility(Xaml::Visibility::Collapsed);
}
}

namespace winui_drover_island {

template <typename Fn>
void DialogsAndPopupsDemo::tryFn(Fn&& fn) {
	try {
		closeEverything(mRoot.Children(), mDialog, mPopup, mErrorPanel);
		fn();
	}
	catch (const winrt::hresult_error& error) {
		mErrorText.Text(error.message());
		mErrorPanel.Visibility(Xaml::Visibility::Visible);
	}
}

DialogsAndPopupsDemo::DialogsAndPopupsDemo() {
	mRoot.BorderBrush(Xaml::Media::SolidColorBrush(Colors::Silver()));
	mRoot.BorderThickness(Xaml::Thickness{1, 1, 1, 1});
	mRoot.Orientation(Xaml::Controls::Orientation::Vertical);
	mRoot.Padding(Xaml::Thickness{kSpacing, kSpacing, kSpacing, 0});
	mRoot.Spacing(kSpacing);
	mRoot.HorizontalAlignment(Xaml::HorizontalAlignment::Stretch);
	mRoot.VerticalAlignment(Xaml::VerticalAlignment::Top);
	auto children = mRoot.Children();

	Xaml::Controls::Button dialogButton;
	dialogButton.Content(winrt::box_value(L"Show Dialog"));
	dialogButton.HorizontalAlignment(Xaml::HorizontalAlignment::Center);
	mRevokeDialogButton = 
		dialogButton.Click(
			winrt::auto_revoke,
			[this](auto&&, auto&&) { 
				tryFn([this] { 
					/*
						dialog throws if the line below is not uncommented.
					*/
					//mDialog.XamlRoot(mRoot.XamlRoot()); // uncomment to make it show
					mDialog.ShowAsync();
				}); 
			});
	children.Append(std::move(dialogButton));

	Xaml::Controls::Button popupButton;
	popupButton.Content(winrt::box_value(L"Show Popup"));
	popupButton.HorizontalAlignment(Xaml::HorizontalAlignment::Center);
	mRevokePopupButton = 
		popupButton.Click(
			winrt::auto_revoke, 
			[this](auto&&, auto&&) {
				tryFn([this] {
					mRoot.Children().Append(mPopup);
					mPopup.IsOpen(true);
				});
			});
	children.Append(std::move(popupButton));

	static const auto errorBrush = [] {
		Xaml::Media::SolidColorBrush brush(Colors::Red());
		brush.Opacity(0.5);
		return brush;
	}();
	mErrorPanel.Background(errorBrush);
	mErrorPanel.Margin(Xaml::Thickness{0, 0, 0, kSpacing});
	mErrorPanel.Padding(Xaml::Thickness{kSpacing, kSpacing, kSpacing, kSpacing});
	mErrorPanel.Visibility(Xaml::Visibility::Collapsed);
	mErrorPanel.Children().Append(mErrorText);
	children.Append(mErrorPanel);

	Xaml::Controls::StackPanel popup;
	popup.BorderThickness(Xaml::Thickness{1, 1, 1, 1});
	popup.BorderBrush(Xaml::Media::SolidColorBrush(Colors::LightSeaGreen()));
	popup.Background(Xaml::Media::SolidColorBrush(Colors::Aquamarine()));
	popup.HorizontalAlignment(Xaml::HorizontalAlignment::Center);
	popup.Padding(Xaml::Thickness{kSpacing, kSpacing, kSpacing, kSpacing});
	
	Xaml::Controls::TextBlock popupText;
	popupText.Foreground(Xaml::Media::SolidColorBrush(Colors::Black()));
	popupText.Text(L"Popup Content");
	popup.Children().Append(std::move(popupText));

	mPopup.Child(std::move(popup));
	mPopup.IsLightDismissEnabled(true);
	mPopup.LightDismissOverlayMode(Xaml::Controls::LightDismissOverlayMode::On);

	mDialog.CloseButtonText(L"Close");
	mDialog.Content(winrt::box_value(L"Dialog Content"));
}

DialogsAndPopupsDemo::~DialogsAndPopupsDemo() {
	/* 
		Crash in 
		            ContentDialog::DetachEventHandlersForOpenDialog()
		called from ContentDialog::~ContentDialog()
	*/
}

Xaml::UIElement DialogsAndPopupsDemo::content() const {
	return mRoot;
}

}
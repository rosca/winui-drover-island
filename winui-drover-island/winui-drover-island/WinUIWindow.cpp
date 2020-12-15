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
#include "WinUIWindow.h"

#include <winrt/Windows.UI.h>

using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation;

namespace winui_drover_island {

WinUIWindow::WinUIWindow() {	
}

void WinUIWindow::create() {
	// Add some content
	mWindow = Window{};
}

void WinUIWindow::addContent() {
	PivotItem droverIslandDemo;
	droverIslandDemo.Header(winrt::box_value(L"Drover Island"));
	droverIslandDemo.Content(mDroverIslandDemo.createContent());

	PivotItem dialogsAndPopupsDemo;
	dialogsAndPopupsDemo.Header(winrt::box_value(L"Dialogs & Popups"));
	dialogsAndPopupsDemo.Content(mDialogsAndPopupsDemo.content());

	Pivot demos;
	demos.Items().Append(std::move(droverIslandDemo));
	demos.Items().Append(std::move(dialogsAndPopupsDemo));
	mWindow.Content(demos);
}

void WinUIWindow::show() {
	mWindow.Activate();
}

}  // namespace winui_drover_island

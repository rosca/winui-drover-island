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
#include "DroverIslandDemo.h"

#include <winrt/Windows.UI.h>
#include "DroverIsland.h"
#include "EllipseShape.h"

using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation;

namespace {
Grid createGrid() {
	// Setup the grid.
	auto column1 = ColumnDefinition{};
	column1.Width(GridLength{ 1, GridUnitType::Star });
	auto column2 = ColumnDefinition{};
	column2.Width(GridLength{ 2, GridUnitType::Star });
	auto column3 = ColumnDefinition{};
	column3.Width(GridLength{ 1, GridUnitType::Star });
	auto row1 = RowDefinition{};
	row1.Height(GridLength{ 0, GridUnitType::Auto });
	auto row2 = RowDefinition{};
	row2.Height(GridLength{ 0, GridUnitType::Auto });
	auto row3 = RowDefinition{};
	row3.Height(GridLength{ 0, GridUnitType::Auto });
	auto row4 = RowDefinition{};
	row4.Height(GridLength{ 1, GridUnitType::Star });

	Grid grid;
	grid.ColumnDefinitions().Append(column1);
	grid.ColumnDefinitions().Append(column2);
	grid.ColumnDefinitions().Append(column3);
	grid.RowDefinitions().Append(row1);
	grid.RowDefinitions().Append(row2);
	grid.RowDefinitions().Append(row3);
	grid.RowDefinitions().Append(row4);
	return grid;
}
}

namespace winui_drover_island {

DroverIslandDemo::DroverIslandDemo() = default;

UIElement DroverIslandDemo::createContent() {
	mGrid = createGrid();

	auto checkbox = CheckBox{};
	checkbox.Margin({ 20, 0, 0, 0 });
	checkbox.Content(winrt::box_value(L"Enable Virtual Surface"));
	mCheckedRevoker = checkbox.Checked(winrt::auto_revoke, [this](const IInspectable&, const RoutedEventArgs&) { mUseVSIS = true; });
	mUncheckedRevoker = checkbox.Unchecked(winrt::auto_revoke, [this](const IInspectable&, const RoutedEventArgs&) { mUseVSIS = false; });
	mGrid.Children().Append(checkbox);

	auto title = TextBlock{};
	title.Text(L"Playground for supporting Drover Islands in WinUI");
	mGrid.Children().Append(title);
	Grid::SetColumn(title, 1);
	Grid::SetRow(title, 0);
	title.HorizontalAlignment(HorizontalAlignment::Center);
	title.Margin({ 0, 20, 0, 20 });
	title.FontSize(32);
	title.TextWrapping(TextWrapping::Wrap);

	auto button = Button{};
	button.Content(winrt::box_value(L"Click me to render the next control!"));
	mGrid.Children().Append(button);
	Grid::SetColumn(button, 1);
	Grid::SetRow(button, 1);
	button.HorizontalAlignment(HorizontalAlignment::Center);
	mClickRevoker = button.Click(winrt::auto_revoke, [this](const IInspectable&, const RoutedEventArgs&) {
		renderCanvasControl(pickNextControl());
		});

	auto description = TextBlock{};
	mGrid.Children().Append(description);
	Grid::SetColumn(description, 1);
	Grid::SetRow(description, 2);
	description.HorizontalAlignment(HorizontalAlignment::Center);
	description.Margin({ 0, 20, 0, 20 });
	description.TextWrapping(TextWrapping::Wrap);
	mDescription = description;

	mCanvasContainer = Border{};
	mGrid.Children().Append(mCanvasContainer);
	Grid::SetColumn(mCanvasContainer, 1);
	Grid::SetRow(mCanvasContainer, 3);
	Media::SolidColorBrush brush;
	brush.Color(winrt::Windows::UI::Colors::Gray());
	mCanvasContainer.Margin({ 0, 0, 0, 20 });
	mCanvasContainer.BorderBrush(brush);
	mCanvasContainer.BorderThickness(Thickness{ 2, 2, 2, 2 });

	renderCanvasControl(mControlType);
	return mGrid;
}

DroverIslandDemo::Type DroverIslandDemo::pickNextControl() {
	switch (mControlType) {
	case Type::Ellipse: return Type::DroverSample;
	case Type::DroverSample: return Type::None;
	default: return Type::Ellipse;
	}
}

void DroverIslandDemo::renderCanvasControl(Type type) {
	switch (type) {
	case Type::None: {
		mCanvasContainer.Child(nullptr);
		break;
	}
	case Type::Ellipse: {
		auto ellipse = winrt::make_self<winui_drover_island::EllipseShape>(mUseVSIS);
		mCanvasContainer.Child(*ellipse);
		break;
	}
	case Type::DroverSample: {
		auto droverIsland = winrt::make_self<winui_drover_island::DroverIsland>(mUseVSIS);
		mCanvasContainer.Child(*droverIsland);
		break;
	}
	}

	const wchar_t* descriptions[] = {
		L"No control is currently rendering.",
		L"This is a sample ellipse rendered using a sample class EllipseShape inheriting from CanvasControl. Resize the window to trigger redraws on the control.",
		L"This is a the placeholder that is supposed to host a Drover Island. It is using the class DroverIsland inheriting from CanvasControl. Resize the window to trigger redraws on the control.",
	};
	static_assert(_countof(descriptions) == static_cast<size_t>(Type::Last) + 1);
	mDescription.Text(descriptions[static_cast<size_t>(type)]);

	mControlType = type;
}

}

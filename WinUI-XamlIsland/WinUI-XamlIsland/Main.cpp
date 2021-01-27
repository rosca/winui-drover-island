#include "pch.h"

#include "microsoft.ui.xaml.hosting.desktopwindowxamlsource.h"
#include "winrt/Microsoft.UI.Xaml.Hosting.h"
#include "winrt/Windows.UI.Xaml.Hosting.h"

namespace {
HWND _hWnd;
HINSTANCE _hInstance;

auto win32ClassName = L"UXP Win32 XamlIsland Window";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }

  return 0;
}

int fail(const char* message) {
  OutputDebugStringA(message);
  return 1;
}

void assertMsg(bool condition, const char* message) {
  if (!condition) {
#if defined _DEBUG
    if (IsDebuggerPresent()) { __debugbreak(); }
#endif
    exit(fail(message));
  }
}

}  // namespace

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int cmdShow) {
  _hInstance = hInstance;

  {
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
    wcex.lpszClassName = win32ClassName;

    auto windowClass = RegisterClassExW(&wcex);
    assertMsg(windowClass, "Cannot register window class");
  }

  auto hwnd = CreateWindowExW(
      WS_EX_OVERLAPPEDWINDOW,
      win32ClassName,
      L"UXP Demo WinUI Xaml Island",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      nullptr,
      nullptr,
      hInstance,
      nullptr);

  assertMsg(hwnd, "Cannot create the main window");

  winrt::init_apartment(winrt::apartment_type::single_threaded);
  winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::
      InitializeForCurrentThread();
  winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource desktopSource;
  auto interop = desktopSource.as<IDesktopWindowXamlSourceNative>();
  winrt::check_hresult(interop->AttachToWindow(hwnd));

  HWND hWndXamlIsland = nullptr;
  interop->get_WindowHandle(&hWndXamlIsland);
  SetWindowPos(hWndXamlIsland, 0, 200, 100, 800, 200, SWP_SHOWWINDOW);

  ShowWindow(hwnd, cmdShow);
  UpdateWindow(hwnd);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
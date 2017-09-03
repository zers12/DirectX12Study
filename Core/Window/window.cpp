#include "window.h"


namespace KLeaf {

HWND Window::native_window_handle_ = nullptr;

bool Window::initialize(Desc desc)
{
	// Initialize th window class.
	WNDCLASSEX window_class = { 0 };
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = CS_HREDRAW | CS_VREDRAW;

#if 0
	window_class.lpfnWndProc = 

	native_window_handle_ = CreateWindow(
		window_class.lpszClassName,

	);
#endif

	return false;
}
LRESULT Window::process(HWND h_window, UINT message, WPARAM w_param, LPARAM l_param)
{



	return LRESULT();
}
}
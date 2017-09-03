#pragma once

#include <Windows.h>
#include <cstdint>

namespace KLeaf {

	// あとでシングルトンにする? (マルチウィンドウとかあまり気にしてない)

class Window {
public:

	struct Desc{
		uint32_t width;
		uint32_t height;
	};

	bool initialize(Desc desc);

	static HWND get_window_handle() { return native_window_handle_; }


private:
	LRESULT CALLBACK process(HWND h_window, UINT message, WPARAM w_param, LPARAM l_param);

	static HWND native_window_handle_;

};

}
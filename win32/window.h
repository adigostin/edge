
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "edge_win32.h"

namespace edge
{
	struct wnd_class_params
	{
		LPCWSTR lpszClassName;
		UINT    style;
		LPCWSTR lpszMenuName;
		LPCWSTR lpIconName;
		LPCWSTR lpIconSmName;
	};

	class window : public event_manager, public win32_window_i
	{
		void register_class (HINSTANCE hInstance, const wnd_class_params& class_params);

	public:
		window (HINSTANCE hInstance, DWORD exStyle, DWORD style, const RECT& rect, HWND hWndParent, int child_control_id);
		window (HINSTANCE hInstance, const wnd_class_params& class_params, DWORD exStyle, DWORD style, int x, int y, int width, int height, HWND hWndParent, HMENU hMenu);
	protected:
		virtual ~window();

		virtual std::optional<LRESULT> window_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		static modifier_key GetModifierKeys();

		static constexpr UINT WM_NEXT = WM_APP;

	public:
		SIZE client_size_pixels() const { return _clientSize; }
		LONG client_width_pixels() const { return _clientSize.cx; }
		LONG client_height_pixels() const { return _clientSize.cy; }
		RECT client_rect_pixels() const { return { 0, 0, _clientSize.cx, _clientSize.cy }; }

		HWND hwnd() const { return _hwnd; }

		LONG dpi() const { return _dpi; }

	private:
		HWND _hwnd = nullptr;
		SIZE _clientSize;
		LONG _dpi;

		static LRESULT CALLBACK WindowProcStatic (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

	struct GdiObjectDeleter
	{
		void operator() (HGDIOBJ object) { ::DeleteObject(object); }
	};
	typedef std::unique_ptr<std::remove_pointer<HFONT>::type, GdiObjectDeleter> HFONT_unique_ptr;
}

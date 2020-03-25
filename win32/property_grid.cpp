
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#include "pch.h"
#include "property_grid.h"
#include "utility_functions.h"

using namespace edge;

static constexpr float indent_width = 10;
static constexpr float separator_height = 4;
static constexpr float description_min_height = 20;

namespace edge
{
	class property_grid;
}

#pragma warning (disable: 4250)

class edge::property_grid : public edge::event_manager, public property_grid_i
{
	d2d_window_i* const _window;
	com_ptr<IDWriteTextFormat> _text_format;
	com_ptr<IDWriteTextFormat> _bold_text_format;
	com_ptr<IDWriteTextFormat> _wingdings;
	std::unique_ptr<text_editor_i> _text_editor;
	D2D1_RECT_F _rect;
	float _name_column_factor = 0.6f;
	float _description_height = 120;
	std::vector<std::unique_ptr<root_item>> _root_items;
	pgitem* _selected_item = nullptr;
	std::optional<float> _description_resize_offset;

public:
	property_grid (d2d_window_i* window, const D2D1_RECT_F& rect)
		: _window(window), _rect(rect)
	{
		auto hr = _window->dwrite_factory()->CreateTextFormat (L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
												   DWRITE_FONT_STRETCH_NORMAL, font_size, L"en-US", &_text_format); assert(SUCCEEDED(hr));

		hr = _window->dwrite_factory()->CreateTextFormat (L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
											  DWRITE_FONT_STRETCH_NORMAL, font_size, L"en-US", &_bold_text_format); assert(SUCCEEDED(hr));

		hr = _window->dwrite_factory()->CreateTextFormat (L"Wingdings", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
											  DWRITE_FONT_STRETCH_NORMAL, font_size, L"en-US", &_wingdings); assert(SUCCEEDED(hr));
	}

	virtual IDWriteFactory* dwrite_factory() const override final { return _window->dwrite_factory(); }

	virtual IDWriteTextFormat* text_format() const override final { return _text_format; }

	virtual IDWriteTextFormat* bold_text_format() const override final { return _bold_text_format; }

	virtual void invalidate() override final { ::InvalidateRect (_window->hwnd(), nullptr, FALSE); } // TODO: optimize
	/*
	virtual std::optional<LRESULT> window_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override
	{
		auto resultBaseClass = base::window_proc (hwnd, msg, wParam, lParam);

		if (msg == WM_GETDLGCODE)
		{
			if (_text_editor)
				return DLGC_WANTALLKEYS;

			return resultBaseClass;
		}

		if ((msg == WM_SETFOCUS) || (msg == WM_KILLFOCUS))
		{
			::InvalidateRect (hwnd, nullptr, 0);
			return 0;
		}

<<<<<<< HEAD
=======
		if (((msg == WM_LBUTTONDOWN) || (msg == WM_RBUTTONDOWN))
			|| ((msg == WM_LBUTTONUP) || (msg == WM_RBUTTONUP)))
		{
			auto button = ((msg == WM_LBUTTONDOWN) || (msg == WM_LBUTTONUP)) ? mouse_button::left : mouse_button::right;
			auto pt = POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			auto dip = pointp_to_pointd(pt) + D2D1_SIZE_F{ pixel_width() / 2, pixel_width() / 2 };
			if ((msg == WM_LBUTTONDOWN) || (msg == WM_RBUTTONDOWN))
			{
				if (msg == WM_LBUTTONDOWN)
					::SetCapture(hwnd);
				process_mouse_button_down (button, (modifier_key)wParam, pt, dip);
			}
			else
			{
				process_mouse_button_up (button, (modifier_key)wParam, pt, dip);
				if (msg == WM_LBUTTONUP)
					::ReleaseCapture();
			}
			return 0;
		}

		if (msg == WM_MOUSEMOVE)
		{
			modifier_key mks = (modifier_key)wParam;
			if (::GetKeyState(VK_MENU) < 0)
				mks |= modifier_key::alt;
			auto pt = POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			auto dip = pointp_to_pointd(pt) + D2D1_SIZE_F{ pixel_width() / 2, pixel_width() / 2 };
			process_mouse_move (mks, pt, dip);
		}

		if ((msg == WM_KEYDOWN) || (msg == WM_SYSKEYDOWN))
		{
			auto handled = process_virtual_key_down ((UINT) wParam, GetModifierKeys());
			if (handled == handled::yes)
				return 0;

			return std::nullopt;
		}

		if ((msg == WM_KEYUP) || (msg == WM_SYSKEYUP))
		{
			auto handled = process_virtual_key_up ((UINT) wParam, GetModifierKeys());
			if (handled == handled::yes)
				return 0;

			return std::nullopt;
		}

		if (msg == WM_CHAR)
		{
			if (_text_editor)
			{
				auto handled = _text_editor->process_character_key ((uint32_t) wParam);
				if (handled == handled::yes)
					return 0;
				else
					return std::nullopt;
			}

			return std::nullopt;
		}

		if (msg == WM_SETCURSOR)
		{
			if (_description_resize_offset)
			{
				SetCursor (LoadCursor(nullptr, IDC_SIZENS));
				return TRUE;
			}

			if (((HWND) wParam == hwnd) && (LOWORD (lParam) == HTCLIENT))
			{
				// Let's check the result because GetCursorPos fails when the input desktop is not the current desktop
				// (happens for example when the monitor goes to sleep and then the lock screen is displayed).
				POINT pt;
				if (::GetCursorPos (&pt))
				{
					if (ScreenToClient (hwnd, &pt))
					{
						auto dip = pointp_to_pointd(pt.x, pt.y) + D2D1_SIZE_F{ pixel_width() / 2, pixel_width() / 2 };
						process_wm_setcursor({ pt.x, pt.y }, dip);
						return TRUE;
					}
				}
			}

			return std::nullopt;
		}

>>>>>>> 320e168d387f2214942c65a628462c29a966e3ca
		return resultBaseClass;
	}
	*/
	virtual HCURSOR cursor_at (POINT pointp, D2D1_POINT_2F dip) const override
	{
		if ((_description_height > separator_height) && point_in_rect(description_separator_rect(), dip))
			return LoadCursor(nullptr, IDC_SIZENS);

		auto item = item_at(dip);
		if (item.first != nullptr)
		{
			if (dip.x >= item.second.x_value)
				return item.first->cursor();
		}

		return ::LoadCursor (nullptr, IDC_ARROW);
	}

	void enum_items (const std::function<void(pgitem*, const item_layout&, bool& cancel)>& callback) const
	{
		std::function<void(pgitem* item, float& y, size_t indent, bool& cancel)> enum_items_inner;

		enum_items_inner = [this, &enum_items_inner, &callback, vcx=value_column_x()](pgitem* item, float& y, size_t indent, bool& cancel)
		{
			auto item_height = item->content_height();
			if (item_height > 0)
			{
				auto horz_line_y = y + item_height;
				horz_line_y = ceilf (horz_line_y / _window->pixel_width()) * _window->pixel_width();

				item_layout il;
				il.y_top = y;
				il.y_bottom = horz_line_y;
				il.x_left = _rect.left;
				il.x_name = _rect.left + indent * indent_width;
				il.x_value = _rect.left + vcx;
				il.x_right = _rect.right;

				callback(item, il, cancel);
				if (cancel)
					return;

				y = horz_line_y + line_thickness();
			}

			if (auto ei = dynamic_cast<expandable_item*>(item); ei && ei->expanded())
			{
				for (auto& child : ei->children())
				{
					enum_items_inner (child.get(), y, indent + 1, cancel);
					if (cancel)
						break;
				}
			}
		};

		float y = _rect.top;
		bool cancel = false;
		for (auto& root_item : _root_items)
		{
			if (y >= _rect.bottom)
				break;
			enum_items_inner (root_item.get(), y, 0, cancel);
			if (cancel)
				break;
		}
	}

	void create_render_resources (root_item* ri)
	{
		auto vcx = value_column_x();
		auto vcw = std::max (75.0f, _rect.right - _rect.left - vcx);

		std::function<void(pgitem* item, size_t indent)> create_inner;

		create_inner = [&create_inner, this, vcx, vcw] (pgitem* item, size_t indent)
		{
			item_layout_horz il;
			il.x_left = 0;
			il.x_name = indent * indent_width;
			il.x_value = vcx;
			il.x_right = std::max (_rect.right - _rect.left, vcx + 75.0f);

			item->create_render_resources (il);

			if (auto ei = dynamic_cast<expandable_item*>(item); ei && ei->expanded())
			{
				for (auto& child : ei->children())
					create_inner (child.get(), indent + 1);
			}
		};

		create_inner (ri, 0);

		invalidate();
	}

	virtual void render (ID2D1DeviceContext* dc) const override
	{
		dc->SetTransform (_window->dpi_transform());

		com_ptr<ID2D1SolidColorBrush> back_brush;
		dc->CreateSolidColorBrush (GetD2DSystemColor(COLOR_WINDOW), &back_brush);
		dc->FillRectangle(_rect, back_brush);

		if (_root_items.empty())
		{
			auto tl = text_layout_with_metrics (dwrite_factory(), _text_format, "(no selection)");
			D2D1_POINT_2F p = { (_rect.left + _rect.right) / 2 - tl.width() / 2, (_rect.top + _rect.bottom) / 2 - tl.height() / 2};
			com_ptr<ID2D1SolidColorBrush> brush;
			dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_WINDOWTEXT), &brush);
			dc->DrawTextLayout (p, tl, brush);
			return;
		}

		bool focused = GetFocus() == _window->hwnd();

		render_context rc;
		rc.dc = dc;
		dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_WINDOW), &rc.back_brush);
		dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_WINDOWTEXT), &rc.fore_brush);
		dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_MENUHILIGHT), &rc.selected_back_brush_focused);
		dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_MENU), &rc.selected_back_brush_not_focused);
		dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_WINDOWTEXT), &rc.selected_fore_brush);
		dc->CreateSolidColorBrush (GetD2DSystemColor (COLOR_GRAYTEXT), &rc.disabled_fore_brush);

		float items_bottom = _rect.bottom - ((_description_height > separator_height) ? _description_height : 0);
		dc->PushAxisAlignedClip({ _rect.left, _rect.top, _rect.right, items_bottom}, D2D1_ANTIALIAS_MODE_ALIASED);
		enum_items ([&, this](pgitem* item, const item_layout& layout, bool& cancel)
		{
			bool selected = (item == _selected_item);
			item->render (rc, layout, selected, focused);

			if (selected && _text_editor)
				_text_editor->render(dc);

			if (layout.y_bottom + line_thickness() >= items_bottom)
				cancel = true;

			D2D1_POINT_2F p0 = { _rect.left, layout.y_bottom + line_thickness() / 2 };
			D2D1_POINT_2F p1 = { _rect.right, layout.y_bottom + line_thickness() / 2 };
			dc->DrawLine (p0, p1, rc.disabled_fore_brush, line_thickness());
		});
		dc->PopAxisAlignedClip();

		if (_description_height > separator_height)
		{
			auto separator_color = interpolate(GetD2DSystemColor(COLOR_WINDOW), GetD2DSystemColor (COLOR_WINDOWTEXT), 80);
			com_ptr<ID2D1SolidColorBrush> brush;
			dc->CreateSolidColorBrush (separator_color, &brush);
			dc->FillRectangle(description_separator_rect(), brush);

			if (_selected_item)
			{
				auto desc_rect = description_rect();
				float lr_padding = 3;
				auto title_layout = text_layout_with_metrics (dwrite_factory(), _bold_text_format, _selected_item->description_title(), _rect.right - _rect.left - 2 * lr_padding);
				dc->DrawTextLayout({ desc_rect.left + lr_padding, desc_rect.top }, title_layout, rc.fore_brush);

				auto desc_layout = text_layout(dwrite_factory(), _text_format, _selected_item->description_text(), _rect.right - _rect.left - 2 * lr_padding);
				dc->DrawTextLayout({ desc_rect.left + lr_padding, desc_rect.top + title_layout.height() }, desc_layout, rc.fore_brush);
			}
		}
	}

	D2D1_RECT_F description_rect() const
	{
		assert(_description_height > separator_height);
		D2D1_RECT_F rect = { 0, _rect.bottom - _rect.top - _description_height + separator_height, _rect.right - _rect.left, _rect.bottom - _rect.top };
		rect = align_to_pixel (rect, _window->dpi());
		return rect;
	}

	D2D1_RECT_F description_separator_rect() const
	{
		assert(_description_height > separator_height);
		D2D1_RECT_F separator = { _rect.left, _rect.bottom - _description_height, _rect.right, _rect.bottom - _description_height + separator_height };
		separator = align_to_pixel (separator, _window->dpi());
		return separator;
	}

	virtual d2d_window_i* window() const override { return _window; }

	virtual const D2D1_RECT_F& rect() const override { return _rect; }

	virtual void set_rect (const D2D1_RECT_F& r) override
	{
		if (_rect != r)
		{
			_window->invalidate(_rect);
			_rect = r;
			if (!_root_items.empty())
			{
				_text_editor = nullptr;
				for (auto& ri : _root_items)
					create_render_resources(ri.get());
			}
			_window->invalidate(_rect);
		}
	}

	virtual void on_dpi_changed() override
	{
		if (!_root_items.empty())
		{
			_text_editor = nullptr;
			for (auto& ri : _root_items)
				create_render_resources(ri.get());
		}
		_window->invalidate(_rect);
	}

	virtual void clear() override
	{
		_text_editor = nullptr;
		_selected_item = nullptr;
		_root_items.clear();
		invalidate();
	}

	virtual void add_section (const char* heading, object* const* objects, size_t size) override
	{
		_root_items.push_back (std::make_unique<root_item>(this, heading, objects, size));
		create_render_resources(_root_items.back().get());
		invalidate();
	}

	virtual void set_description_height (float height) override
	{
		_description_height = height;
		invalidate();
	}

	virtual bool read_only() const override { return false; }

	virtual property_edited_e::subscriber property_changed() override { return property_edited_e::subscriber(this); }

	virtual description_height_changed_e::subscriber description_height_changed() override { return description_height_changed_e::subscriber(this); }

	virtual text_editor_i* show_text_editor (const D2D1_RECT_F& rect, bool bold, float lr_padding, std::string_view str) override final
	{
		uint32_t fill_argb = 0xFF00'0000u | GetSysColor(COLOR_WINDOW);
		uint32_t text_argb = 0xFF00'0000u | GetSysColor(COLOR_WINDOWTEXT);
		_text_editor = text_editor_factory (_window, dwrite_factory(), bold ? _bold_text_format : _text_format, fill_argb, text_argb, rect, lr_padding, str);
		return _text_editor.get();
	}

	virtual int show_enum_editor (D2D1_POINT_2F dip, const nvp* nameValuePairs) override final
	{
		_text_editor = nullptr;
		POINT ptScreen = _window->pointd_to_pointp(dip, 0);
		::ClientToScreen (_window->hwnd(), &ptScreen);

		HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr (_window->hwnd(), GWLP_HINSTANCE);

		static constexpr wchar_t ClassName[] = L"GIGI-{655C4EA9-2A80-46D7-A7FB-D510A32DC6C6}";
		static constexpr UINT WM_CLOSE_POPUP = WM_APP;

		static ATOM atom = 0;
		if (atom == 0)
		{
			static const auto WndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				if ((msg == WM_COMMAND) && (HIWORD(wParam) == BN_CLICKED))
				{
					::PostMessage (hwnd, WM_CLOSE_POPUP, LOWORD(wParam), 0);
					return 0;
				}

				return DefWindowProc (hwnd, msg, wParam, lParam);
			};

			WNDCLASS EditorWndClass =
			{
				0, // style
				WndProc,
				0, // cbClsExtra
				0, // cbWndExtra
				hInstance,
				nullptr, // hIcon
				::LoadCursor(nullptr, IDC_ARROW), // hCursor
				(HBRUSH) (COLOR_3DFACE + 1), // hbrBackground
				nullptr, // lpszMenuName
				ClassName, // lpszClassName
			};

			atom = ::RegisterClassW (&EditorWndClass); assert (atom != 0);
		}

		auto hwnd = CreateWindowEx (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, ClassName, L"aaa", WS_POPUP | WS_BORDER, 0, 0, 0, 0, _window->hwnd(), nullptr, hInstance, nullptr); assert (hwnd != nullptr);

		LONG maxTextWidth = 0;
		LONG maxTextHeight = 0;

		NONCLIENTMETRICS ncMetrics = { sizeof(NONCLIENTMETRICS) };
		auto proc_addr = GetProcAddress(GetModuleHandleA("user32.dll"), "SystemParametersInfoForDpi");
		if (proc_addr != nullptr)
		{
			auto proc = reinterpret_cast<BOOL(WINAPI*)(UINT, UINT, PVOID, UINT, UINT)>(proc_addr);
			proc(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncMetrics, 0, _window->dpi());
		}
		else
			SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncMetrics, 0);
		HFONT_unique_ptr font (CreateFontIndirect (&ncMetrics.lfMenuFont));

		auto hdc = ::GetDC(hwnd);
		auto oldFont = ::SelectObject (hdc, font.get());
		size_t count;
		for (count = 0; nameValuePairs[count].name != nullptr; count++)
		{
			RECT rc = { };
			DrawTextA (hdc, nameValuePairs[count].name, -1, &rc, DT_CALCRECT);
			maxTextWidth = std::max (maxTextWidth, rc.right);
			maxTextHeight = std::max (maxTextHeight, rc.bottom);
		}
		::SelectObject (hdc, oldFont);
		::ReleaseDC (hwnd, hdc);

		int lrpadding = 7 * _window->dpi() / 96;
		int udpadding = ((count <= 5) ? 5 : 0) * _window->dpi() / 96;
		LONG buttonWidth = std::max (100l * (LONG)_window->dpi() / 96, maxTextWidth + 2 * lrpadding) + 2 * GetSystemMetrics(SM_CXEDGE);
		LONG buttonHeight = maxTextHeight + 2 * udpadding + 2 * GetSystemMetrics(SM_CYEDGE);

		int margin = 4 * _window->dpi() / 96;
		int spacing = 2 * _window->dpi() / 96;
		int y = margin;
		for (size_t nvp_index = 0; nameValuePairs[nvp_index].name != nullptr;)
		{
			constexpr DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_NOTIFY | BS_FLAT;
			auto button = CreateWindowExA (0, "Button", nameValuePairs[nvp_index].name, dwStyle, margin, y, buttonWidth, buttonHeight, hwnd, (HMENU) nvp_index, hInstance, nullptr);
			::SendMessage (button, WM_SETFONT, (WPARAM) font.get(), FALSE);
			nvp_index++;
			y += buttonHeight + (nameValuePairs[nvp_index].name ? spacing : margin);
		}
		RECT wr = { 0, 0, margin + buttonWidth + margin, y };
		::AdjustWindowRectEx (&wr, (DWORD) GetWindowLongPtr(hwnd, GWL_STYLE), FALSE, (DWORD) GetWindowLongPtr(hwnd, GWL_EXSTYLE));
		::SetWindowPos (hwnd, nullptr, ptScreen.x, ptScreen.y, wr.right - wr.left, wr.bottom - wr.top, SWP_NOACTIVATE | SWP_SHOWWINDOW);

		int selected_nvp_index = -1;
		MSG msg;
		while (GetMessage(&msg, 0, 0, 0))
		{
			if ((msg.hwnd == hwnd) && (msg.message == WM_CLOSE_POPUP))
			{
				selected_nvp_index = (int) msg.wParam;
				break;
			}

			if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE))
				break;

			bool exitLoop = false;
			if ((msg.hwnd != hwnd) && (::GetParent(msg.hwnd) != hwnd))
			{
				if ((msg.message == WM_LBUTTONDOWN) || (msg.message == WM_RBUTTONDOWN) || (msg.message == WM_MBUTTONDOWN)
					|| (msg.message == WM_NCLBUTTONDOWN) || (msg.message == WM_NCRBUTTONDOWN) || (msg.message == WM_NCMBUTTONDOWN))
				{
					ShowWindow (hwnd, SW_HIDE);
					exitLoop = true;
				}
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (exitLoop)
				break;
		}

		::DestroyWindow (hwnd);
		return selected_nvp_index;
	}

	std::pair<pgitem*, item_layout> item_at (D2D1_POINT_2F dip) const
	{
		std::pair<pgitem*, item_layout> result = { };

		enum_items ([&](pgitem* item, const item_layout& layout, bool& cancel)
		{
			if (dip.y < layout.y_bottom)
			{
				result.first = item;
				result.second = layout;
				cancel = true;
			}
		});

		return result;
	}

	virtual handled process_mouse_down (mouse_button button, modifier_key mks, POINT pixel, D2D1_POINT_2F dip) override final
	{
		if (_text_editor && (_text_editor->mouse_captured() || point_in_rect(_text_editor->rect(), dip)))
			return _text_editor->process_mouse_button_down(button, mks, dip);

		if (_description_height > separator_height)
		{
			auto ds_rect = description_separator_rect();
			if (point_in_rect(ds_rect, dip))
			{
				_text_editor = nullptr;
				_description_resize_offset = dip.y - ds_rect.top;
				return handled(true);
			}

			if (point_in_rect(description_rect(), dip))
				return handled(true);
		}

		auto clicked_item = item_at(dip);

		auto new_selected_item = (clicked_item.first && clicked_item.first->selectable()) ? clicked_item.first : nullptr;
		if (_selected_item != new_selected_item)
		{
			_text_editor = nullptr;
			_selected_item = new_selected_item;
			invalidate();
		}

		if (clicked_item.first != nullptr)
		{
			clicked_item.first->process_mouse_button_down (button, mks, pixel, dip, clicked_item.second);
			return handled(true);
		}

		return handled(false);
	}

	virtual handled process_mouse_up (mouse_button button, modifier_key mks, POINT pixel, D2D1_POINT_2F dip) override final
	{
		if (_text_editor && _text_editor->mouse_captured())
			return _text_editor->process_mouse_button_up (button, mks, dip);

		if (_description_resize_offset)
		{
			_description_resize_offset.reset();
			event_invoker<description_height_changed_e>()(_description_height);
			return handled(true);
		}

		auto clicked_item = item_at(dip);
		if (clicked_item.first != nullptr)
		{
			clicked_item.first->process_mouse_button_up (button, mks, pixel, dip, clicked_item.second);
			return handled(true);
		}

		return handled(false);
	}

	virtual void process_mouse_move (modifier_key mks, POINT pixel, D2D1_POINT_2F dip) override final
	{
		if (_text_editor && _text_editor->mouse_captured())
			return _text_editor->process_mouse_move (mks, dip);

		if (_description_resize_offset)
		{
			_description_height = _rect.bottom - _rect.top - dip.y + _description_resize_offset.value();
			_description_height = std::max (description_min_height, _description_height);
			invalidate();
			return;
		}
	}

	void try_commit_editor()
	{
		if (_text_editor == nullptr)
			return;

		auto prop_item = dynamic_cast<value_item*>(_selected_item); assert(prop_item);
		auto text_utf16 = _text_editor->wstr();
		auto text_utf8 = utf16_to_utf8(text_utf16);
		try
		{
			change_property (prop_item->parent()->parent()->objects(), prop_item->property(), text_utf8);
		}
		catch (const std::exception& ex)
		{
			auto message = utf8_to_utf16(ex.what());
			::MessageBox (_window->hwnd(), message.c_str(), L"Error setting property", 0);
			::SetFocus (_window->hwnd());
			_text_editor->select_all();
			return;
		}

		_window->invalidate(_text_editor->rect());
		_text_editor = nullptr;
	}

	virtual void change_property (const std::vector<object*>& objects, const value_property* prop, std::string_view new_value_str) override final
	{
		std::vector<std::string> old_values;
		old_values.reserve(objects.size());
		for (auto o : objects)
			old_values.push_back (prop->get_to_string(o));

		for (size_t i = 0; i < objects.size(); i++)
		{
			try
			{
				prop->set_from_string (new_value_str, objects[i]);
			}
			catch (const std::exception&)
			{
				for (size_t j = 0; j < i; j++)
					prop->set_from_string(old_values[j], objects[j]);

				throw;
			}
		}

		property_edited_args args = { objects, std::move(old_values), std::string(new_value_str) };
		this->event_invoker<property_edited_e>()(std::move(args));
	}

	virtual float line_thickness() const override
	{
		static constexpr float line_thickness_not_aligned = 0.6f;
		auto lt = roundf(line_thickness_not_aligned / _window->pixel_width()) * _window->pixel_width();
		return lt;
	}

	virtual float value_column_x() const override
	{
		float w = (_rect.right - _rect.left) * _name_column_factor;
		w = roundf (w / _window->pixel_width()) * _window->pixel_width();
		return std::max (75.0f, w);
	}

	virtual handled process_key_down (uint32_t key, modifier_key mks) override
	{
		if ((key == VK_RETURN) || (key == VK_UP) || (key == VK_DOWN))
		{
			try_commit_editor();

			if (key == VK_UP)
			{
				// select previous item
			}
			else if (key == VK_DOWN)
			{
				// select next item
			}

			return handled(true);
		}

		if ((key == VK_ESCAPE) && _text_editor)
		{
			_text_editor = nullptr;
			return handled(true);
		}

		if (_text_editor)
			return _text_editor->process_virtual_key_down (key, mks);

		return handled(false);
	}

	virtual handled process_key_up (uint32_t key, modifier_key mks) override
	{
		if (_text_editor)
			return _text_editor->process_virtual_key_up (key, mks);

		return handled(false);
	}

	virtual handled process_char_key (uint32_t key) override
	{
		if (_text_editor)
			return _text_editor->process_character_key (key);

		return handled(false);
	}
};

extern std::unique_ptr<property_grid_i> edge::property_grid_factory (d2d_window_i* window, const D2D1_RECT_F& rect)
{
	return std::make_unique<property_grid>(window, rect);
};

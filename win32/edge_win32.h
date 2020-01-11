#pragma once
#include "../events.h"
#include "../minspan.h"

namespace edge
{
	enum class handled { no = 0, yes = 1 };

	enum class mouse_button { left, right, middle, };

	enum class modifier_key
	{
		none    = 0,
		shift   = 4,    // MK_SHIFT
		control = 8,    // MK_CONTROL
		alt     = 0x20, // MK_ALT
		lbutton = 1,    // MK_LBUTTON
		rbutton = 2,    // MK_RBUTTON
		mbutton = 0x10, // MK_MBUTTON
	};
	//DEFINE_ENUM_FLAG_OPERATORS(modifier_key);
	inline constexpr modifier_key operator& (modifier_key a, modifier_key b) noexcept { return (modifier_key) ((std::underlying_type_t<modifier_key>)a & (std::underlying_type_t<modifier_key>)b); }
	inline constexpr modifier_key& operator |= (modifier_key& a, modifier_key b) noexcept { return (modifier_key&) ((std::underlying_type_t<modifier_key>&)a |= (std::underlying_type_t<modifier_key>)b); }
	inline constexpr bool operator== (modifier_key a, std::underlying_type_t<modifier_key> b) noexcept { return (std::underlying_type_t<modifier_key>)a == b; }
	inline constexpr bool operator!= (modifier_key a, std::underlying_type_t<modifier_key> b) noexcept { return (std::underlying_type_t<modifier_key>)a != b; }

	struct __declspec(novtable) win32_window_i
	{
		virtual ~win32_window_i() { }

		virtual HWND hwnd() const = 0;

		bool visible() const;

		RECT client_rect_pixels() const;
		SIZE client_size_pixels() const;

		RECT GetRect() const;
		LONG GetX() const { return GetRect().left; }
		LONG GetY() const { return GetRect().top; }
		POINT GetLocation() const;
		LONG GetWidth() const;
		LONG GetHeight() const;
		SIZE GetSize() const;

		void SetRect (const RECT& rect)
		{
			BOOL bRes = ::MoveWindow (hwnd(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			assert(bRes);
		}

		void invalidate()
		{
			::InvalidateRect (hwnd(), nullptr, FALSE);
		}
	};

	struct zoomable_i : public win32_window_i
	{
		struct zoom_transform_changed_e : event<zoom_transform_changed_e, zoomable_i*> { };

		virtual D2D1::Matrix3x2F zoom_transform() const = 0;
		virtual D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const = 0;
		virtual void pointw_to_pointd (std::span<D2D1_POINT_2F> locations) const = 0;
		virtual float lengthw_to_lengthd (float  wlength) const = 0;
		virtual zoom_transform_changed_e::subscriber zoom_transform_changed() = 0;
		virtual void zoom_to (const D2D1_RECT_F& rect, float min_margin, float min_zoom, float max_zoom, bool smooth) = 0;

		D2D1_POINT_2F pointw_to_pointd (D2D1_POINT_2F location) const;
		bool hit_test_line (D2D1_POINT_2F dLocation, float tolerance, D2D1_POINT_2F p0w, D2D1_POINT_2F p1w, float lineWidth) const;
	};

}

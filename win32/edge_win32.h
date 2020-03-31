
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "../events.h"

namespace edge
{
	using handled = bool;

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
	inline constexpr modifier_key operator| (modifier_key a, modifier_key b) noexcept { return (modifier_key) ((std::underlying_type_t<modifier_key>)a | (std::underlying_type_t<modifier_key>)b); }
	inline constexpr modifier_key& operator |= (modifier_key& a, modifier_key b) noexcept { return (modifier_key&) ((std::underlying_type_t<modifier_key>&)a |= (std::underlying_type_t<modifier_key>)b); }
	inline constexpr bool operator== (modifier_key a, std::underlying_type_t<modifier_key> b) noexcept { return (std::underlying_type_t<modifier_key>)a == b; }
	inline constexpr bool operator!= (modifier_key a, std::underlying_type_t<modifier_key> b) noexcept { return (std::underlying_type_t<modifier_key>)a != b; }

	struct gdi_object_deleter
	{
		void operator() (HGDIOBJ object) { ::DeleteObject(object); }
	};
	using HFONT_unique_ptr = std::unique_ptr<std::remove_pointer<HFONT>::type, gdi_object_deleter>;

	struct __declspec(novtable) win32_window_i
	{
		virtual ~win32_window_i() = default;

		virtual HWND hwnd() const = 0;

		bool visible() const;

		RECT client_rect_pixels() const;
		SIZE client_size_pixels() const;
		LONG client_width_pixels() const;
		LONG client_height_pixels() const;

		RECT GetRect() const;
		LONG GetX() const { return GetRect().left; }
		LONG GetY() const { return GetRect().top; }
		POINT GetLocation() const;
		LONG width_pixels() const;
		LONG height_pixels() const;
		SIZE size_pixels() const;
		void move_window (const RECT& rect);
		void invalidate();
		void invalidate (const RECT& rect);
	};

	struct __declspec(novtable) dpi_aware_window_i : virtual win32_window_i
	{
		virtual uint32_t dpi() const = 0;

		D2D1::Matrix3x2F dpi_transform() const;
		float pixel_width() const;
		D2D1_RECT_F client_rect() const;
		D2D1_SIZE_F client_size() const;
		float client_width() const;
		float client_height() const;
		float lengthp_to_lengthd (LONG lengthp) const;
		LONG lengthd_to_lengthp (float lengthd, int round_style) const;
		D2D1_POINT_2F pointp_to_pointd (POINT locationPixels) const;
		D2D1_POINT_2F pointp_to_pointd (long xPixels, long yPixels) const;
		POINT pointd_to_pointp (float xDips, float yDips, int round_style) const;
		POINT pointd_to_pointp (D2D1_POINT_2F locationDips, int round_style) const;
		D2D1_SIZE_F sizep_to_sized (SIZE size_pixels) const;
		//SIZE sized_to_sizep (D2D1_SIZE_F size_dips) const;
		D2D1_RECT_F rectp_to_rectd (RECT rp) const;
		using win32_window_i::invalidate;
		void invalidate (const D2D1_RECT_F& rect);
	};

	struct __declspec(novtable) d2d_window_i : virtual dpi_aware_window_i
	{
		virtual ID2D1DeviceContext* dc() const = 0;
		virtual IDWriteFactory* dwrite_factory() const = 0;
		virtual void show_caret (const D2D1_RECT_F& bounds, D2D1_COLOR_F color, const D2D1_MATRIX_3X2_F* transform = nullptr) = 0;
		virtual void hide_caret() = 0;
	};

	struct __declspec(novtable) zoomable_i : virtual d2d_window_i
	{
		struct zoom_transform_changed_e : event<zoom_transform_changed_e, zoomable_i*> { };

		virtual D2D1_POINT_2F aimpoint() const = 0;
		virtual float zoom() const = 0;
		virtual zoom_transform_changed_e::subscriber zoom_transform_changed() = 0;
		virtual void zoom_to (const D2D1_RECT_F& rect, float min_margin, float min_zoom, float max_zoom, bool smooth) = 0;

		D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const;
		void pointw_to_pointd (std::span<D2D1_POINT_2F> locations) const;
		float lengthw_to_lengthd (float lengthw) const { return lengthw * zoom(); }
		D2D1_SIZE_F pixel_aligned_window_center() const;
		D2D1_POINT_2F pointw_to_pointd (D2D1_POINT_2F location) const;
		D2D1::Matrix3x2F zoom_transform() const;
		bool hit_test_line (D2D1_POINT_2F dLocation, float tolerance, D2D1_POINT_2F p0w, D2D1_POINT_2F p1w, float lineWidth) const;
	};

}

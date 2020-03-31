
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "../object.h"
#include "text_editor.h"
#include "property_grid_items.h"

namespace edge
{
	struct property_editor_i
	{
		virtual ~property_editor_i() = default;
		virtual bool show (win32_window_i* parent) = 0; // return IDOK, IDCANCEL, -1 (some error), 0 (hWndParent invalid or closed)
		virtual void cancel() = 0;
	};

	struct __declspec(novtable) custom_editor_property_i
	{
		virtual std::unique_ptr<property_editor_i> create_editor (std::span<object* const> objects) const = 0;
	};

	struct __declspec(novtable) property_grid_i
	{
		virtual ~property_grid_i() = default;
		virtual d2d_window_i* window() const = 0;
		virtual RECT rectp() const = 0;
		virtual D2D1_RECT_F rectd() const = 0;
		virtual void set_rect (const RECT& rectp) = 0;
		virtual void set_border_width (float bw) = 0;
		virtual float border_width() const = 0;
		virtual void on_dpi_changed() = 0;
		virtual void clear() = 0;
		virtual void add_section (const char* heading, std::span<object* const> objects) = 0;
		void add_section (const char* heading, object* obj) { add_section(heading, { &obj, 1 }); }
		virtual std::span<const std::unique_ptr<root_item>> sections() const = 0;
		virtual bool read_only() const = 0;
		virtual void render (ID2D1DeviceContext* dc) const = 0;
		virtual handled on_mouse_down (mouse_button button, modifier_key mks, POINT pp, D2D1_POINT_2F pd) = 0;
		virtual handled on_mouse_up   (mouse_button button, modifier_key mks, POINT pp, D2D1_POINT_2F pd) = 0;
		virtual void    on_mouse_move (modifier_key mks, POINT pp, D2D1_POINT_2F pd) = 0;
		virtual handled on_key_down (uint32_t vkey, modifier_key mks) = 0;
		virtual handled on_key_up (uint32_t vkey, modifier_key mks) = 0;
		virtual handled on_char_key (uint32_t ch) = 0;
		virtual HCURSOR cursor_at (POINT pp, D2D1_POINT_2F pd) const = 0;
		virtual void enable_input_output(bool enable) = 0;
		virtual bool input_output_enabled() const = 0;
		virtual D2D1_POINT_2F input_of (value_item* vi) const = 0;

		enum class htcode { none, input, expand, name, value, output };

		struct htresult
		{
			pgitem* item;
			float   y;
			htcode  code;
		};

		virtual htresult item_at (D2D1_POINT_2F pd) const = 0;

		struct property_edited_args
		{
			const std::vector<object*>& objects;
			std::vector<std::string> old_values;
			std::string new_value;
		};

		struct property_edited_e : event<property_edited_e, property_edited_args&&> { };
		virtual property_edited_e::subscriber property_changed() = 0;

		// TODO: make these internal to property_grid.cpp / property_grid_items.cpp
		virtual IDWriteFactory* dwrite_factory() const = 0;
		virtual IDWriteTextFormat* text_format() const = 0;
		virtual IDWriteTextFormat* bold_text_format() const = 0;
		virtual void invalidate() = 0;
		virtual text_editor_i* show_text_editor (const D2D1_RECT_F& rect, bool bold, float lr_padding, std::string_view str) = 0;
		virtual int show_enum_editor (D2D1_POINT_2F dip, const nvp* nvps) = 0;
		virtual void change_property (const std::vector<object*>& objects, const value_property* prop, std::string_view new_value_str) = 0;
		virtual float line_thickness() const = 0;
		virtual float name_column_x (size_t indent) const = 0;
		virtual float value_column_x() const = 0;
		float width() const { auto r = rectd(); return r.right - r.left; }
		float height() const { auto r = rectd(); return r.bottom - r.top; }
	};

	std::unique_ptr<property_grid_i> property_grid_factory (d2d_window_i* window, const RECT& rectp);
}

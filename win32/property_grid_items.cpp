
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#include "pch.h"
#include "property_grid_items.h"
#include "property_grid.h"
#include "utility_functions.h"

using namespace D2D1;

namespace edge
{
	root_item* pgitem::root()
	{
		return parent()->root();
	}

	void expandable_item::expand()
	{
		assert (!_expanded);
		_children = this->create_children();
		_expanded = true;
	}

	#pragma region object_item
	object_item::object_item (object* const* objects, size_t size)
		: _objects(objects, objects + size)
	{
		for (auto obj : _objects)
		{
			obj->property_changing().add_handler(&on_property_changing, this);
			obj->property_changed().add_handler(&on_property_changed, this);
		}
	}

	object_item::~object_item()
	{
		for (auto obj : _objects)
		{
			obj->property_changed().remove_handler(&on_property_changed, this);
			obj->property_changing().remove_handler(&on_property_changing, this);
		}
	}

	//static
	void object_item::on_property_changing (void* arg, object* obj, const property_change_args& args)
	{
	}

	//static
	void object_item::on_property_changed (void* arg, object* obj, const property_change_args& args)
	{
		if (args.property->_ui_visible == ui_visible::no)
			return;

		auto oi = static_cast<object_item*>(arg);
		auto root_item = oi->root();

		if (auto prop = dynamic_cast<const value_property*>(args.property))
		{
			for (auto& gi : oi->children())
			{
				for (auto& child_item : static_cast<group_item*>(gi.get())->children())
				{
					if (auto vi = dynamic_cast<value_item*>(child_item.get()); vi->property() == prop)
					{
						vi->on_value_changed();
						break;
					}
				}
			}
		}
		else if (auto prop = dynamic_cast<const value_collection_property*>(args.property))
		{
			assert(false); // not implemented
		}
		else
		{
			assert(false); // not implemented
		}
	}

	pgitem* object_item::find_child (const property* prop) const
	{
		for (auto& item : children())
		{
			if (auto value_item = dynamic_cast<default_value_pgitem*>(item.get()); value_item->property() == prop)
				return item.get();
		}

		return nullptr;
	}

	std::vector<std::unique_ptr<pgitem>> object_item::create_children()
	{
		if (_objects.empty())
			return { };

		auto type = _objects[0]->type();
		if (!std::all_of (_objects.begin(), _objects.end(), [type](object* o) { return o->type() == type; }))
			return { };

		struct group_comparer
		{
			bool operator() (const property_group* g1, const property_group* g2) const { return g1->prio < g2->prio; }
		};

		std::set<const property_group*, group_comparer> groups;

		for (auto prop : type->make_property_list())
		{
			if (groups.find(prop->_group) == groups.end())
				groups.insert(prop->_group);
		}

		std::vector<std::unique_ptr<pgitem>> items;
		std::transform (groups.begin(), groups.end(), std::back_inserter(items),
						[this](const property_group* n) { return std::make_unique<group_item>(this, n); });
		return items;
	}
	#pragma endregion

	#pragma region group_item
	group_item::group_item (object_item* parent, const property_group* group)
		: _parent(parent), _group(group)
	{
		expand();
	}

	std::vector<std::unique_ptr<pgitem>> group_item::create_children()
	{
		std::vector<std::unique_ptr<pgitem>> items;

		auto type = parent()->objects().front()->type();

		for (auto prop : type->make_property_list())
		{
			if (prop->_ui_visible == ui_visible::yes)
			{
				if (prop->_group == _group)
				{
					std::unique_ptr<pgitem> item;

					auto factories = prop->editor_factories();
					auto it = std::find_if (factories.begin(), factories.end(), [](auto f) { return dynamic_cast<const pgitem_factory_i*>(f) != nullptr; });
					if (it != factories.end())
					{
						auto f = static_cast<const pgitem_factory_i*>(*it);
						item = f->create_item(this, prop);
					}
					else if (auto value_prop = dynamic_cast<const value_property*>(prop))
					{
						item = std::make_unique<default_value_pgitem>(this, value_prop);
					}
					else
					{
						// TODO: placeholder pg item for unknown types of properties
						assert(false);
					}

					items.push_back(std::move(item));
				}
			}
		}

		return items;
	}

	void group_item::create_render_resources (const item_layout_horz& l)
	{
		auto factory = root()->_grid->dwrite_factory();
		com_ptr<IDWriteTextFormat> tf;
		auto hr = factory->CreateTextFormat (L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
											 DWRITE_FONT_STRETCH_NORMAL, font_size, L"en-US", &tf);
		float layout_width = std::max (0.0f, l.x_right -l.x_name - 2 * title_lr_padding);
		_layout = text_layout_with_metrics (factory, tf, _group->name, layout_width);
	}

	void group_item::render (const render_context& rc, const item_layout& l, bool selected, bool focused) const
	{
		rc.dc->FillRectangle ({ l.x_left, l.y_top, l.x_right, l.y_bottom }, rc.back_brush);
		rc.dc->DrawTextLayout ({ l.x_name + text_lr_padding, l.y_top }, _layout, rc.fore_brush);
	}

	float group_item::content_height() const
	{
		return _layout.height();
	}
	#pragma endregion

	#pragma region root_item
	root_item::root_item (property_grid_i* grid, const char* heading, object* const* objects, size_t size)
		: base(objects, size), _grid(grid), _heading(heading ? heading : "")
	{
		expand();
	}

	void root_item::create_render_resources (const item_layout_horz& l)
	{
		_text_layout = nullptr;
		if (!_heading.empty())
		{
			auto factory = root()->_grid->dwrite_factory();
			// TODO: padding
			com_ptr<IDWriteTextFormat> tf;
			auto hr = factory->CreateTextFormat (L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
												 DWRITE_FONT_STRETCH_NORMAL, font_size, L"en-US", &tf);
			float layout_width = std::max (0.0f, l.x_right -l.x_left - 2 * title_lr_padding);
			_text_layout = text_layout_with_metrics (factory, tf, _heading, layout_width);
		}
	}

	void root_item::render (const render_context& rc, const item_layout& l, bool selected, bool focused) const
	{
		if (_text_layout)
		{
			com_ptr<ID2D1SolidColorBrush> brush;
			rc.dc->CreateSolidColorBrush (GetD2DSystemColor(COLOR_ACTIVECAPTION), &brush);
			D2D1_RECT_F rect = { l.x_left, l.y_top, l.x_right, l.y_bottom };
			rc.dc->FillRectangle (&rect, brush);
			brush->SetColor (GetD2DSystemColor(COLOR_CAPTIONTEXT));
			rc.dc->DrawTextLayout ({ l.x_left + title_lr_padding, l.y_top + title_ud_padding }, _text_layout, brush);
		}
	}

float root_item::content_height() const
	{
		if (_text_layout)
			return _text_layout.height() + 2 * title_ud_padding;
		else
			return 0;
	}
	#pragma endregion

	#pragma region value_item
	bool value_item::multiple_values() const
	{
		auto& objs = _parent->parent()->objects();
		for (size_t i = 1; i < objs.size(); i++)
		{
			if (!property()->equal(objs[0], objs[i]))
				return true;
		}

		return false;
	}

	bool value_item::can_edit() const
	{
		return !root()->_grid->read_only() && _value.readable && (dynamic_cast<const custom_editor_property_i*>(property()) || !property()->read_only());
	}

	bool value_item::changed_from_default() const
	{
		for (auto o : parent()->parent()->objects())
		{
			if (property()->changed_from_default(o))
				return true;
		}

		return false;
	}

	value_item::value_layout value_item::create_value_layout_internal() const
	{
		auto grid = root()->_grid;
		auto factory = grid->dwrite_factory();

		float width = std::max (0.0f, grid->width() - grid->value_column_x() - grid->line_thickness() - 2 * text_lr_padding);

		auto format = changed_from_default() ? grid->bold_text_format() : grid->text_format();

		text_layout_with_metrics tl;
		bool readable;
		try
		{
			if (multiple_values())
				tl = text_layout_with_metrics (factory, format, "(multiple values)", width);
			else
				tl = text_layout_with_metrics (factory, format, property()->get_to_string(parent()->parent()->objects().front()), width);
			readable = true;
		}
		catch (const std::exception& ex)
		{
			tl = text_layout_with_metrics (factory, format, ex.what(), width);
			readable = false;
		}

		return { std::move(tl), readable };
	}

	void value_item::create_render_resources (const item_layout_horz& l)
	{
		auto grid = root()->_grid;
		auto factory = grid->dwrite_factory();
		auto format = grid->text_format();
		auto line_thickness = grid->line_thickness();
		_name = text_layout_with_metrics (factory, format, property()->_name, l.x_value - l.x_name - line_thickness - 2 * text_lr_padding);
		_value = create_value_layout_internal();
	}

	float value_item::content_height() const
	{
		return std::max (_name.height(), _value.tl.height());
	}

	void value_item::render (const render_context& rc, const item_layout& l, bool selected, bool focused) const
	{
		auto line_thickness = root()->_grid->line_thickness();

		if (selected)
		{
			D2D1_RECT_F rect = { l.x_left, l.y_top, l.x_right, l.y_bottom };
			rc.dc->FillRectangle (&rect, focused ? rc.selected_back_brush_focused.get() : rc.selected_back_brush_not_focused.get());
		}

		float name_line_x = l.x_name + line_thickness / 2;
		rc.dc->DrawLine ({ name_line_x, l.y_top }, { name_line_x, l.y_bottom }, rc.disabled_fore_brush, line_thickness);
		auto fore = selected ? rc.selected_fore_brush.get() : rc.fore_brush.get();
		rc.dc->DrawTextLayout ({ l.x_name + line_thickness + text_lr_padding, l.y_top }, name(), fore);

		float linex = l.x_value + line_thickness / 2;
		rc.dc->DrawLine ({ linex, l.y_top }, { linex, l.y_bottom }, rc.disabled_fore_brush, line_thickness);
		this->render_value (rc, l, selected, focused);
	}

	std::string value_item::description_title() const
	{
		std::stringstream ss;
		ss << property()->_name << " (" << property()->type_name() << ")";
		return ss.str();
	}

	std::string value_item::description_text() const
	{
		return property()->_description ? std::string(property()->_description) : std::string();
	}

	void value_item::on_value_changed()
	{
		_value = create_value_layout_internal();
		root()->_grid->invalidate();
	}
	#pragma endregion

	#pragma region default_value_pgitem
	void default_value_pgitem::render_value (const render_context& rc, const item_layout& l, bool selected, bool focused) const
	{
		auto line_thickness = root()->_grid->line_thickness();
		auto fore = !can_edit() ? rc.disabled_fore_brush.get() : (selected ? rc.selected_fore_brush.get() : rc.fore_brush.get());
		rc.dc->DrawTextLayout ({ l.x_value + line_thickness + text_lr_padding, l.y_top }, value().tl, fore);
	}

	HCURSOR default_value_pgitem::cursor() const
	{
		if (!can_edit())
			return ::LoadCursor(nullptr, IDC_ARROW);

		if (auto bool_p = dynamic_cast<const edge::bool_p*>(property()))
			return ::LoadCursor(nullptr, IDC_HAND);

		if (property()->nvps() || dynamic_cast<const custom_editor_property_i*>(property()))
			return ::LoadCursor(nullptr, IDC_HAND);

		return ::LoadCursor (nullptr, IDC_IBEAM);
	}

	static const nvp bool_nvps[] = {
		{ "False", 0 },
		{ "True", 1 },
		{ nullptr, -1 },
	};

	void default_value_pgitem::process_mouse_button_down (mouse_button button, modifier_key mks, POINT pt, D2D1_POINT_2F dip, const item_layout& layout)
	{
		if (dip.x < layout.x_value)
			return;

		if (auto cep = dynamic_cast<const custom_editor_property_i*>(property()))
		{
			auto editor = cep->create_editor(parent()->parent()->objects());
			editor->show(root()->_grid->window());
			return;
		}

		if (property()->read_only())
			return;

		if (property()->nvps() || dynamic_cast<const edge::bool_p*>(property()))
		{
			auto nvps = property()->nvps() ? property()->nvps() : bool_nvps;
			int selected_nvp_index = root()->_grid->show_enum_editor(dip, nvps);
			if (selected_nvp_index >= 0)
			{
				auto new_value_str = nvps[selected_nvp_index].name;
				auto changed = [new_value_str, prop=property()](object* o) { return prop->get_to_string(o) != new_value_str; };
				auto& objects = parent()->parent()->objects();
				if (std::any_of(objects.begin(), objects.end(), changed))
				{
					try
					{
						root()->_grid->change_property (objects, property(), new_value_str);
					}
					catch (const std::exception& ex)
					{
						auto message = utf8_to_utf16(ex.what());
						::MessageBox (root()->_grid->window()->hwnd(), message.c_str(), L"Error setting property", 0);
					}
				}

			}
		}
		else
		{
			auto lt = root()->_grid->line_thickness();
			D2D1_RECT_F editor_rect = { layout.x_value + lt, layout.y_top, layout.x_right, layout.y_bottom };
			bool bold = changed_from_default();
			auto editor = root()->_grid->show_text_editor (editor_rect, bold, text_lr_padding, multiple_values() ? "" : property()->get_to_string(parent()->parent()->objects().front()));
			editor->process_mouse_button_down (button, mks, dip);
		}
	}

	void default_value_pgitem::process_mouse_button_up (mouse_button button, modifier_key mks, POINT pt, D2D1_POINT_2F dip, const item_layout& layout)
	{
	}
	#pragma endregion

	std::unique_ptr<pgitem> create_default_pgitem (expandable_item* parent, std::span<object* const> objects, const property* prop)
	{
		if (auto value_prop = dynamic_cast<const value_property*>(prop))
		{
			auto gi = static_cast<group_item*>(parent);
			assert (gi == dynamic_cast<group_item*>(parent));
			return std::make_unique<default_value_pgitem>(gi, value_prop);
		}

		// TODO: placeholder pg item for unknown types of properties
		assert(false);
		return nullptr;
	}
}

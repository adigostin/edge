
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "..\object.h"
#include "com_ptr.h"
#include "edge_win32.h"
#include "text_layout.h"

namespace edge
{
	struct property_grid_i;
	class root_item;

	static constexpr float font_size = 12;

	struct render_context
	{
		ID2D1DeviceContext* dc;
		com_ptr<ID2D1SolidColorBrush> back_brush;
		com_ptr<ID2D1SolidColorBrush> fore_brush;
		com_ptr<ID2D1SolidColorBrush> selected_back_brush_focused;
		com_ptr<ID2D1SolidColorBrush> selected_back_brush_not_focused;
		com_ptr<ID2D1SolidColorBrush> selected_fore_brush;
		com_ptr<ID2D1SolidColorBrush> disabled_fore_brush;
	};

	struct item_layout_horz
	{
		float x_left;
		float x_name;
		float x_value;
		float x_right;
	};

	struct item_layout : item_layout_horz
	{
		float y_top;
		float y_bottom;
	};

	class expandable_item;

	class pgitem
	{
		pgitem (const pgitem&) = delete;
		pgitem& operator= (const pgitem&) = delete;

	public:
		pgitem() = default;
		virtual ~pgitem() = default;

		static constexpr float text_lr_padding = 3;
		static constexpr float title_lr_padding = 4;
		static constexpr float title_ud_padding = 2;

		virtual root_item* root();
		const root_item* root() const { return const_cast<pgitem*>(this)->root(); }

		virtual expandable_item* parent() const = 0;
		virtual void create_render_resources (const item_layout_horz& l) = 0;
		// TODO: move line_thickness into render_context
		virtual void render (const render_context& rc, const item_layout& l, bool selected, bool focused) const = 0;
		virtual float content_height() const = 0;
		virtual HCURSOR cursor() const { return nullptr; }
		virtual bool selectable() const = 0;
		virtual void on_mouse_down (mouse_button button, modifier_key mks, POINT pt, D2D1_POINT_2F dip, const item_layout& layout) { }
		virtual void on_mouse_up   (mouse_button button, modifier_key mks, POINT pt, D2D1_POINT_2F dip, const item_layout& layout) { }
		virtual std::string description_title() const = 0;
		virtual std::string description_text() const = 0;
	};

	class expandable_item : public pgitem
	{
		using base = pgitem;
		using base::pgitem;

		bool _expanded = false;
		std::vector<std::unique_ptr<pgitem>> _children;

	public:
		const std::vector<std::unique_ptr<pgitem>>& children() const { return _children; }
		void expand();
		void collapse();
		bool expanded() const { return _expanded; }

	protected:
		virtual std::vector<std::unique_ptr<pgitem>> create_children() = 0;
	};

	class object_item abstract : public expandable_item
	{
		using base = expandable_item;

		std::vector<object*> const _objects;

	public:
		object_item (std::span<object* const> objects);
		virtual ~object_item();

		const std::vector<object*>& objects() const { return _objects; }

		pgitem* find_child (const property* prop) const;

	private:
		void on_property_changing (object* obj, const property_change_args& args);
		void on_property_changed (object* obj, const property_change_args& args);
		virtual std::vector<std::unique_ptr<pgitem>> create_children() override final;
	};

	class group_item : public expandable_item
	{
		using base = expandable_item;

		object_item* const _parent;
		const property_group* const _group;

		text_layout_with_metrics _layout;

	public:
		group_item (object_item* parent, const property_group* group);
		virtual object_item* parent() const { return _parent; }

		virtual std::vector<std::unique_ptr<pgitem>> create_children() override;
		virtual void create_render_resources (const item_layout_horz& l) override;
		virtual void render (const render_context& rc, const item_layout& l, bool selected, bool focused) const override;
		virtual float content_height() const override;
		virtual bool selectable() const override { return false; }
		virtual std::string description_title() const override final { return { }; }
		virtual std::string description_text() const override final { return { }; }
	};

	class root_item : public object_item
	{
		using base = object_item;

		std::string _heading;
		text_layout_with_metrics _text_layout;

	public:
		property_grid_i* const _grid;

		root_item (property_grid_i* grid, const char* heading, std::span<object* const> objects);

		virtual expandable_item* parent() const override { assert(false); return nullptr; }
		virtual root_item* root() override final { return this; }
		virtual void create_render_resources (const item_layout_horz& l) override final;
		virtual void render (const render_context& rc, const item_layout& l, bool selected, bool focused) const override final;
		virtual float content_height() const override final;
		virtual bool selectable() const override final { return false; }
		virtual std::string description_title() const override final { return { }; }
		virtual std::string description_text() const override final { return { }; }
	};

	// TODO: value from collection / value from pd

	class value_item abstract : public pgitem
	{
		using base = pgitem;

	public:
		group_item* const _parent;

		struct value_layout
		{
			text_layout_with_metrics tl;
			bool readable;
		};

	private:
		text_layout_with_metrics _name;
		value_layout _value;

	public:
		value_item (group_item* parent)
			: _parent(parent)
		{ }

		virtual const value_property* property() const = 0;
		virtual void render_value (const render_context& rc, const item_layout& l, bool selected, bool focused) const = 0;

		const text_layout_with_metrics& name() const { return _name; }
		const value_layout& value() const { return _value; }
		bool can_edit() const;
		bool multiple_values() const;

		virtual group_item* parent() const override final { return _parent; }
		virtual bool selectable() const override final { return true; }
		virtual void create_render_resources (const item_layout_horz& l) override;
		virtual float content_height() const override final;
		virtual void render (const render_context& rc, const item_layout& l, bool selected, bool focused) const override final;
		virtual std::string description_title() const override final;
		virtual std::string description_text() const override final;

		bool changed_from_default() const;

	private:
		value_layout create_value_layout_internal() const;

		friend class object_item;

	protected:
		virtual void on_value_changed();
	};

	class default_value_pgitem : public value_item
	{
		using base = value_item;
		using base::base;

		const value_property* const _property;

	public:
		default_value_pgitem (group_item* parent, const value_property* property)
			: base(parent), _property(property)
		{ }

		virtual const value_property* property() const override final { return _property; }
		virtual void render_value (const render_context& rc, const item_layout& l, bool selected, bool focused) const override final;

	protected:
		virtual HCURSOR cursor() const override final;
		virtual void on_mouse_down (mouse_button button, modifier_key mks, POINT pt, D2D1_POINT_2F dip, const item_layout& layout) override;
		virtual void on_mouse_up   (mouse_button button, modifier_key mks, POINT pt, D2D1_POINT_2F dip, const item_layout& layout) override;
	};

	struct __declspec(novtable) pgitem_factory_i : property_editor_factory_i
	{
		virtual std::unique_ptr<pgitem> create_item (group_item* parent, const property* prop) const = 0;
	};
}

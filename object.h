
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "reflection.h"
#include "events.h"
#include <vector>
#include <array>
#include <tuple>

namespace edge
{
	class type
	{
		const char* const _name;
		const type* const base_type;
		std::span<const property* const> const props;

	public:
		constexpr type(const char* name, const type* base_type, std::span<const property* const> props) noexcept
			: _name(name), base_type(base_type), props(props)
		{ }

		const char* name() const { return _name; }
		std::vector<const property*> make_property_list() const;
		const property* find_property (const char* name) const;
		bool has_property (const property* p) const;
		bool is_derived_from (const type* t) const;
		bool is_derived_from (const type& t) const;

	private:
		void add_properties (std::vector<const property*>& properties) const;
	};

	struct concrete_type : type
	{
		using type::type;
		virtual std::span<const value_property* const> factory_props() const = 0;
		virtual std::unique_ptr<object> create (std::span<std::string_view> string_values) const = 0;
	};

	template<typename object_type, typename... factory_arg_property_traits>
	struct xtype : concrete_type
	{
		static constexpr size_t parameter_count = sizeof...(factory_arg_property_traits);

		// Commented out because VC++ seems to have problems on this when compiling some inline constexpr xtype constructors.
		// static_assert (std::is_base_of<object, object_type>::value);

		using factory_t = std::unique_ptr<object_type>(*)(typename factory_arg_property_traits::value_t... factory_args);

		factory_t const _factory;
		std::array<const value_property*, parameter_count> const _factory_props;

	public:
		constexpr xtype (const char* name, const type* base, std::span<const property* const> props,
			factory_t factory = nullptr, const static_value_property<factory_arg_property_traits>*... factory_props)
			: concrete_type(name, base, props)
			, _factory(factory)
			, _factory_props(std::array<const value_property*, parameter_count>{ factory_props... })
		{ }

		factory_t factory() const { return _factory; }

		virtual std::span<const value_property* const> factory_props() const override { return _factory_props; }

		std::unique_ptr<object_type> create (typename factory_arg_property_traits::value_t... factory_args) const
		{
			return std::unique_ptr<object_type>(_factory(factory_args...));
		}

	private:
		template<size_t... I>
		std::unique_ptr<object> create_internal (std::span<std::string_view> string_values, std::tuple<typename factory_arg_property_traits::value_t...>& values, std::index_sequence<I...>) const
		{
			(factory_arg_property_traits::from_string(string_values[I], std::get<I>(values)), ...);
			return std::unique_ptr<object>(_factory(std::get<I>(values)...));
		}

	public:
		virtual std::unique_ptr<object> create (std::span<std::string_view> string_values) const override
		{
			assert (_factory);
			assert (string_values.size() == parameter_count);
			std::tuple<typename factory_arg_property_traits::value_t...> values;
			return create_internal(string_values, values, std::make_index_sequence<parameter_count>());
		}
	};

	enum class collection_property_change_type { set, insert, remove };

	struct property_change_args
	{
		const struct property* property;
		size_t index;
		collection_property_change_type type;

		property_change_args (const value_property* property)
			: property(property)
		{ }

		property_change_args (const value_property& property)
			: property(&property)
		{ }

		property_change_args (const value_collection_property* property, size_t index, collection_property_change_type type)
			: property(property), index(index), type(type)
		{ }

		property_change_args (const object_collection_property* property, size_t index, collection_property_change_type type)
			: property(property), index(index), type(type)
		{ }
	};

	struct collection_i
	{
	};

	// TODO: make event_manager a member var, possibly a pointer
	class object : public event_manager
	{
		collection_i* _parent = nullptr;

		template<typename child_t, typename store_t>
		friend struct typed_collection_i;

	public:
		virtual ~object() = default;

		collection_i* parent() const { return _parent; }

		struct property_changing_e : event<property_changing_e, object*, const property_change_args&> { };
		struct property_changed_e  : event<property_changed_e , object*, const property_change_args&> { };
		// TODO: combine these four into a single ownership_change_e (enum ownership_change_type)
		struct inserting_to_parent_e : public event<inserting_to_parent_e> { };
		struct inserted_to_parent_e : public event<inserted_to_parent_e> { };
		struct removing_from_parent_e : public event<removing_from_parent_e> { };
		struct removed_from_parent_e : public event<removed_from_parent_e> { };

		property_changing_e::subscriber property_changing() { return property_changing_e::subscriber(this); }
		property_changed_e::subscriber property_changed() { return property_changed_e::subscriber(this); }
		inserting_to_parent_e::subscriber inserting_to_parent() { return inserting_to_parent_e::subscriber(this); }
		inserted_to_parent_e::subscriber inserted_to_parent() { return inserted_to_parent_e::subscriber(this); }
		removing_from_parent_e::subscriber removing_from_parent() { return removing_from_parent_e::subscriber(this); }
		removed_from_parent_e::subscriber removed_from_parent() { return removed_from_parent_e::subscriber(this); }

	protected:
		virtual void on_property_changing (const property_change_args&);
		virtual void on_property_changed (const property_change_args&);
		virtual void on_inserting_to_parent () { this->event_invoker<inserting_to_parent_e>()(); }
		virtual void on_inserted_to_parent  () { this->event_invoker<inserted_to_parent_e>()(); }
		virtual void on_removing_from_parent() { this->event_invoker<removing_from_parent_e>()(); }
		virtual void on_removed_from_parent () { this->event_invoker<removed_from_parent_e>()(); }

		static const edge::type _type;
	public:
		virtual const concrete_type* type() const = 0;
	};

	template<typename child_t, typename store_t = std::vector<std::unique_ptr<child_t>>>
	struct typed_collection_i : collection_i
	{
	private:
		virtual store_t& children_store() = 0;
		const store_t& children_store() const { return const_cast<typed_collection_i*>(this)->children_store(); }

	protected:
		virtual void on_child_inserting (size_t index, child_t* child) { }
		virtual void on_child_inserted (size_t index, child_t* child) { }
		virtual void on_child_removing (size_t index, child_t* child) { }
		virtual void on_child_removed (size_t index, child_t* child) { }

	public:
		const std::vector<std::unique_ptr<child_t>>& children() const
		{
			return const_cast<typed_collection_i*>(this)->children_store();
		}

		size_t child_count() const { return children_store().size(); }

		child_t* child_at(size_t index) const { return children_store()[index].get(); }

		void insert (size_t index, std::unique_ptr<child_t>&& o)
		{
			static_assert (std::is_base_of<object, child_t>::value);

			auto& children = children_store();
			assert (index <= children.size());
			child_t* raw = o.get();
			assert (raw->_parent == nullptr);

			raw->on_inserting_to_parent();
			this->on_child_inserting(index, raw);
			children.insert (children.begin() + index, std::move(o));
			raw->_parent = this;
			this->on_child_inserted (index, raw);
			raw->on_inserted_to_parent();
		}

		void append (std::unique_ptr<child_t>&& o)
		{
			insert (children_store().size(), std::move(o));
		}

		std::unique_ptr<child_t> remove(size_t index)
		{
			static_assert (std::is_base_of<object, child_t>::value);

			auto& children = children_store();
			assert (index < children.size());
			auto it = children.begin() + index;
			child_t* raw = (*it).get();
			assert (raw->_parent == this);

			raw->on_removing_from_parent();
			this->on_child_removing (index, raw);
			raw->_parent = nullptr;
			auto result = std::move (children[index]);
			children.erase (children.begin() + index);
			this->on_child_removed (index, raw);
			raw->on_removed_from_parent();

			return result;
		}

		std::unique_ptr<child_t> remove_last()
		{
			return remove(children_store().size() - 1);
		}

		child_t* last() const { return children_store().back().get(); }
	};
}

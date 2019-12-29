
#pragma once
#include "reflection.h"
#include "events.h"

namespace edge
{
	struct type
	{
		static std::vector<const type*>* known_types;

		const char* const name;
		const type* const base_type;
		span<const property* const> const props;

		type(const char* name, const type* base_type, span<const property* const> props);;
		~type();

		static const type* find_type (const char* name);

		std::vector<const property*> make_property_list() const;
		const property* find_property (const char* name) const;

		virtual bool has_factory() const = 0;
		virtual span<const value_property* const> factory_props() const = 0;
		virtual object* create (const span<string_view>& string_values) const = 0;
	private:
		void add_properties (std::vector<const property*>& properties) const;
	};

	template<typename object_type, typename... factory_arg_props>
	class xtype : public type
	{
		static constexpr size_t parameter_count = sizeof...(factory_arg_props);

		static_assert (std::conjunction_v<std::is_base_of<value_property, factory_arg_props>...>, "factory params must derive from value_property");
		static_assert (std::is_convertible_v<object_type*, object*>);

		using factory_t = object_type*(*)(typename factory_arg_props::param_t... factory_args);
		
		factory_t const _factory;
		array<const value_property*, parameter_count> const _factory_props;

	public:
		xtype (const char* name, const type* base, span<const property* const> props,
			factory_t factory = nullptr, const factory_arg_props*... factory_props)
			: type(name, base, props)
			, _factory(factory)
			, _factory_props(array<const value_property*, parameter_count>{ factory_props... })
		{ }

	private:
		virtual bool has_factory() const override { return _factory != nullptr; }

		virtual span<const value_property* const> factory_props() const override { return _factory_props; }
		
		template<std::size_t... I>
		static std::tuple<typename factory_arg_props::value_t...> strings_to_values (const span<string_view>& string_values, std::index_sequence<I...>)
		{
			std::tuple<typename factory_arg_props::value_t...> result;
			bool cast_ok = (true && ... && factory_arg_props::from_string(string_values[I], std::get<I>(result)));
			assert(cast_ok);
			return result;
		}

		virtual object* create (const span<string_view>& string_values) const override
		{
			assert (_factory != nullptr);
			assert (string_values.size() == parameter_count);
			auto values = strings_to_values (string_values, std::make_index_sequence<parameter_count>());
			object_type* obj = std::apply (_factory, values);
			return static_cast<object*>(obj);
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

	struct property_changing_e : event<property_changing_e, object*, const property_change_args&> { };
	struct property_changed_e  : event<property_changed_e , object*, const property_change_args&> { };

	// TODO: make event_manager a member var, possibly a pointer
	class object : public event_manager
	{
	public:
		virtual ~object() = default;

		template<typename T>
		bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

		property_changing_e::subscriber property_changing() { return property_changing_e::subscriber(this); }
		property_changed_e::subscriber property_changed() { return property_changed_e::subscriber(this); }

	protected:
		virtual void on_property_changing (const property_change_args&);
		virtual void on_property_changed (const property_change_args&);

	public:
		static const xtype<object> _type;
		virtual const type* type() const { return &_type; }
	};
}

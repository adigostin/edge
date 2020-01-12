
#pragma once
#include "reflection.h"
#ifdef _WIN32
#include "events.h"
#endif
#include <vector>
#include <array>
#include <tuple>

namespace edge
{
	struct type
	{
		const char* const name;
		const type* const base_type;
		std::span<const property* const> const props;

		constexpr type(const char* name, const type* base_type, std::span<const property* const> props) noexcept
			: name(name), base_type(base_type), props(props)
		{ }

		std::vector<const property*> make_property_list() const;
		const property* find_property (const char* name) const;

		virtual std::span<const value_property* const> factory_props() const = 0;
		virtual std::unique_ptr<object> create (std::span<std::string_view> string_values) const = 0;
	private:
		void add_properties (std::vector<const property*>& properties) const;
	};

	template<typename object_type, typename... factory_arg_props>
	class xtype : public type
	{
		static constexpr size_t parameter_count = sizeof...(factory_arg_props);

		static_assert (std::is_base_of<object, object_type>::value);
		static_assert (std::conjunction<std::is_base_of<value_property, factory_arg_props>...>::value, "factory params must derive from value_property");

		using factory_t = std::unique_ptr<object_type>(*)(typename factory_arg_props::param_t... factory_args);

		factory_t const _factory;
		std::array<const value_property*, parameter_count> const _factory_props;

	public:
		constexpr xtype (const char* name, const type* base, std::span<const property* const> props,
			factory_t factory = nullptr, const factory_arg_props*... factory_props)
			: type(name, base, props)
			, _factory(factory)
			, _factory_props(std::array<const value_property*, parameter_count>{ factory_props... })
		{ }

	private:
		virtual std::span<const value_property* const> factory_props() const override { return _factory_props; }

		template<size_t... I>
		std::unique_ptr<object> create_internal (std::span<std::string_view> string_values, std::tuple<typename factory_arg_props::value_t...>& values, std::index_sequence<I...>) const
		{
			bool all_casts_ok = (true && ... && factory_arg_props::from_string(string_values[I], std::get<I>(values)));
			if (!all_casts_ok)
				return nullptr;
			return std::unique_ptr<object>(_factory(std::get<I>(values)...));
		}

		virtual std::unique_ptr<object> create (std::span<std::string_view> string_values) const override
		{
			assert (_factory);
			assert (string_values.size() == parameter_count);
			std::tuple<typename factory_arg_props::value_t...> values;
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

	// TODO: make event_manager a member var, possibly a pointer
	#ifdef _WIN32
	class object : public event_manager
	#else
	class object
	#endif
	{
	public:
		virtual ~object() = default;

		#ifdef _WIN32
		struct property_changing_e : event<property_changing_e, object*, const property_change_args&> { };
		struct property_changed_e  : event<property_changed_e , object*, const property_change_args&> { };

		property_changing_e::subscriber property_changing() { return property_changing_e::subscriber(this); }
		property_changed_e::subscriber property_changed() { return property_changed_e::subscriber(this); }
		#endif

	protected:
		virtual void on_property_changing (const property_change_args&);
		virtual void on_property_changed (const property_change_args&);

	public:
		static const xtype<object> _type;
		virtual const struct type* type() const { return &_type; }
	};
}

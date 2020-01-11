
#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <type_traits>
#include <cstdint>
#include <cstdio>
#include <optional>
#include "minspan.h"

namespace edge
{
	class object;

	using property_editor_parent = void*; // this is a platform-specific type; on Win32 it's a pointer to a edge::win32_window_i

	struct property_editor_i
	{
		virtual ~property_editor_i() { }
		virtual bool show (property_editor_parent parent) = 0; // return IDOK, IDCANCEL, -1 (some error), 0 (hWndParent invalid or closed)
		virtual void cancel() = 0;
	};

	using property_editor_factory_t = std::unique_ptr<property_editor_i>(std::span<object* const> objects);

	struct property_group
	{
		int32_t prio;
		const char* name;
	};

	extern const property_group misc_group;

	enum class ui_visible { no, yes };

	enum class serializable { no, yes };

	struct property
	{
		const char* const _name;
		const property_group* const _group;
		const char* const _description;
		ui_visible _ui_visible;

		constexpr property (const char* name, const property_group* group, const char* description, enum ui_visible ui_visible)
			: _name(name), _group((group != nullptr) ? group : &misc_group), _description(description), _ui_visible(ui_visible)
		{ }
		property (const property&) = delete;
		property& operator= (const property&) = delete;

		virtual property_editor_factory_t* custom_editor() const { return nullptr; }
	};

	struct NVP
	{
		const char* first;
		int second;
	};

	struct value_property : property
	{
		using property::property;

		virtual const char* type_name() const = 0;
		virtual bool has_setter() const = 0;
		virtual std::string get_to_string (const object* obj) const = 0;
		virtual bool try_set_from_string (object* obj, std::string_view str) const = 0;
		virtual const NVP* nvps() const = 0;
		virtual bool equal (object* obj1, object* obj2) const = 0;
		virtual bool changed_from_default(const object* obj) const = 0;
	};

	// ========================================================================

	template<typename property_traits, property_editor_factory_t* custom_editor_ = nullptr>
	struct typed_property : value_property
	{
		using base = value_property;

		using value_t  = typename property_traits::value_t;
		using param_t  = typename property_traits::param_t;
		using return_t = typename property_traits::return_t;

		static constexpr auto from_string = &property_traits::from_string;

		using member_getter_t = return_t (object::*)() const;
		using member_setter_t = void (object::*)(param_t);
		using static_getter_t = return_t(*)(const object*);
		using static_setter_t = void(*)(object*, param_t);
		using member_var_t = value_t object::*;

		template <typename From, typename To, typename = void>
		struct is_static_castable
		{
			static constexpr bool value = false;
		};

		template <typename From, typename To>
		struct is_static_castable<From, To, std::void_t<decltype(static_cast<To>(*(From*)(nullptr)))>>
		{
			static constexpr bool value = true;
		};

		class getter_t
		{
			enum getter_type { none, member_function, static_function, member_var };

			getter_type const type;
			union
			{
				std::nullptr_t  np;
				member_getter_t mg;
				static_getter_t sg;
				member_var_t    mv;
			};

		public:
			constexpr getter_t (std::nullptr_t np) noexcept : type(none), np(np) { }

			template<typename MemberGetter, std::enable_if_t<is_static_castable<MemberGetter, member_getter_t>::value, int> = 0>
			constexpr getter_t (MemberGetter mg) noexcept : type(member_function), mg(static_cast<member_getter_t>(mg)) { }

			template<typename StaticGetter, std::enable_if_t<is_static_castable<StaticGetter, static_getter_t>::value, int> = 0>
			constexpr getter_t (StaticGetter sg) noexcept : type(static_function), sg(static_cast<static_getter_t>(sg)) { }

			constexpr getter_t (member_var_t mv) noexcept : type(member_var), mv(mv) { }

			return_t get (const object* obj) const
			{
				if (type == member_function)
					return (obj->*mg)();

				if (type == static_function)
					return sg(obj);

				if (type == member_var)
					return obj->*mv;

				assert(false);
				return { };
			}
		};

		class setter_t
		{
			enum setter_type { member_function, static_function, none };

			setter_type const type;
			union
			{
				std::nullptr_t  np;
				member_setter_t ms;
				static_setter_t ss;
			};

		public:
			constexpr setter_t (std::nullptr_t np) noexcept : type(none), np(np) { }

			template<typename MemberSetter, std::enable_if_t<is_static_castable<MemberSetter, member_setter_t>::value, int> = 0>
			constexpr setter_t (MemberSetter ms) noexcept : type(member_function), ms(static_cast<member_setter_t>(ms)) { }

			template<typename StaticSetter, std::enable_if_t<is_static_castable<StaticSetter, static_setter_t>::value, int> = 0>
			constexpr setter_t (StaticSetter ss) noexcept : type(static_function), ss(static_cast<static_setter_t>(ss)) { }

			bool try_set_from_string (object* obj, std::string_view str_in) const
			{
				value_t value;
				bool ok = property_traits::from_string(str_in, value);
				if (ok)
				{
					if (type == member_function)
						(obj->*ms)(value);
					else if (type == static_function)
						ss(obj, value);
					else
						assert(false);
				}

				return ok;
			}

			bool has_setter() const { return type != none; }
		};

		getter_t const _getter;
		setter_t const _setter;
		std::optional<value_t> const _default_value;

		constexpr typed_property (const char* name, const property_group* group, const char* description, enum ui_visible ui_visible, getter_t getter, setter_t setter, std::optional<value_t>&& default_value = std::nullopt)
			: base(name, group, description, ui_visible), _getter(getter), _setter(setter), _default_value(std::move(default_value))
		{ }

		virtual const char* type_name() const override final { return property_traits::type_name; }

		virtual bool has_setter() const override final { return _setter.has_setter(); }

		virtual property_editor_factory_t* custom_editor() const override final { return custom_editor_; }

		return_t get (const object* obj) const { return _getter.get(obj); }

		virtual std::string get_to_string (const object* obj) const override final { return property_traits::to_string(_getter.get(obj)); }

		virtual bool try_set_from_string (object* obj, std::string_view str_in) const override final
		{
			return _setter.try_set_from_string (obj, str_in);
		}

	private:

		// https://stackoverflow.com/a/17534399/451036

		template <typename T, typename = void>
		struct nvps_helper
		{
			static const NVP* nvps() { return nullptr; }
		};

		template <typename T>
		struct nvps_helper<T, typename std::enable_if<bool(sizeof(&T::nvps))>::type>
		{
			static const NVP* nvps() { return T::nvps; }
		};

	public:
		virtual const NVP* nvps() const override final { return nvps_helper<property_traits>::nvps(); }

		virtual bool equal (object* obj1, object* obj2) const override final
		{
			return get(obj1) == get(obj2);
		}

		virtual bool changed_from_default(const object* obj) const override
		{
			if (!_default_value.has_value())
				return true;

			return get(obj) != _default_value.value();
		}
	};

	// ===========================================

	struct bool_property_traits
	{
		static const char type_name[];
		using value_t = bool;
		using param_t = bool;
		using return_t = bool;
		static std::string to_string (bool from) { return from ? "True" : "False"; }
		static bool from_string (std::string_view from, bool& to);
	};
	using bool_p = typed_property<bool_property_traits>;

	template<typename t_, const char* type_name_>
	struct arithmetic_property_traits
	{
		//static_assert (std::is_arithmetic_v<t_>);
		static constexpr const char* type_name = type_name_;
		using value_t = t_;
		using param_t = t_;
		using return_t = t_;
		static std::string to_string (t_ from); // needs specialization
		static bool from_string (std::string_view from, t_& to); // needs specialization
	};

	extern const char int32_type_name[];
	using int32_property_traits = arithmetic_property_traits<int32_t, int32_type_name>;
	using int32_p = typed_property<int32_property_traits>;

	extern const char uint32_type_name[];
	using uint32_property_traits = arithmetic_property_traits<uint32_t, uint32_type_name>;
	using uint32_p = typed_property<uint32_property_traits>;

	extern const char uint64_type_name[];
	using uint64_property_traits = arithmetic_property_traits<uint64_t, uint64_type_name>;
	using uint64_p = typed_property<uint64_property_traits>;

	extern const char size_t_type_name[];
	using size_t_property_traits = arithmetic_property_traits<size_t, size_t_type_name>;
	using size_t_p = typed_property<size_t_property_traits>;

	extern const char float_type_name[];
	using float_property_traits = arithmetic_property_traits<float, float_type_name>;
	using float_p = typed_property<float_property_traits>;

	template<bool backed>
	struct string_property_traits
	{
		static constexpr const char* type_name = backed ? "backed_string" : "temp_string";
		using value_t = std::string;
		using param_t = std::string_view;
		using return_t = std::conditional_t<backed, const std::string&, std::string>;
		static std::string to_string (std::string_view from) { return std::string(from); }
		static bool from_string (std::string_view from, std::string& to) { to = from; return true; }
	};
	using temp_string_property_traits = string_property_traits<false>;
	using temp_string_p = typed_property<temp_string_property_traits>;
	using backed_string_property_traits = string_property_traits<true>;
	using backed_string_p = typed_property<backed_string_property_traits>;

	// ========================================================================

	template<typename enum_t, const char* type_name_, const NVP* nvps_, bool serialize_as_integer, const char* unknown_str>
	struct enum_property_traits
	{
		static constexpr const char* type_name = type_name_;
		using value_t = enum_t;
		using param_t = enum_t;
		using return_t = enum_t;

		static constexpr const NVP* nvps = nvps_;

		static std::string to_string (enum_t from)
		{
			if (serialize_as_integer)
				return int32_property_traits::to_string((int32_t)from);

			for (auto nvp = nvps_; nvp->first != nullptr; nvp++)
			{
				if (nvp->second == (int)from)
					return nvp->first;
			}

			return unknown_str;
		}

		static bool from_string (std::string_view from, enum_t& to)
		{
			if (serialize_as_integer)
			{
				int32_t val;
				bool ok = int32_property_traits::from_string(from, val);
				if (ok)
				{
					to = (enum_t)val;
					return true;
				}
			}

			for (auto nvp = nvps_; nvp->first != nullptr; nvp++)
			{
				if (from == nvp->first)
				{
					to = static_cast<enum_t>(nvp->second);
					return true;
				}
			}

			return false;
		}
	};

	extern const char unknown_enum_value_str[];

	template<typename enum_t, const char* type_name, const NVP* nvps_, bool serialize_as_integer = false, const char* unknown_str = unknown_enum_value_str>
	using enum_property = typed_property<enum_property_traits<enum_t, type_name, nvps_, serialize_as_integer, unknown_str>>;

	// ========================================================================

	enum class side { left, top, right, bottom };
	static constexpr const char side_type_name[] = "side";
	static constexpr const NVP side_nvps[] = {
		{ "Left",   (int) side::left },
		{ "Top",    (int) side::top },
		{ "Right",  (int) side::right },
		{ "Bottom", (int) side::bottom },
		{ 0, 0 },
	};
	using side_p = edge::enum_property<side, side_type_name, side_nvps>;

	// ========================================================================

	struct collection_property : property
	{
		using property::property;
		virtual size_t size (const object* obj) const = 0;
		virtual bool can_insert_remove() const = 0;
	};

	struct object_collection_property : collection_property
	{
		using collection_property::collection_property;
		virtual object* child_at (const object* parent, size_t index) const = 0;
		virtual void insert_child (object* parent, size_t index, std::unique_ptr<object>&& child) const = 0;
		virtual std::unique_ptr<object> remove_child (object* parent, size_t index) const = 0;
	};

	template<typename parent_t, typename child_t>
	struct typed_object_collection_property : object_collection_property
	{
		using base = object_collection_property;

		static_assert (std::is_base_of<object, child_t>::value);
		static_assert (std::is_base_of<object, parent_t>::value);

		using get_child_count_t = size_t(parent_t::*)() const;
		using get_child_t       = child_t*(parent_t::*)(size_t) const;
		using insert_child_t    = void(parent_t::*)(size_t, std::unique_ptr<child_t>&&);
		using remove_child_t    = std::unique_ptr<child_t>(parent_t::*)(size_t);

		get_child_count_t const _get_child_count;
		get_child_t       const _get_child;
		insert_child_t    const _insert_child;
		remove_child_t    const _remove_child;

		constexpr typed_object_collection_property (const char* name, const property_group* group, const char* description, enum ui_visible ui_visible,
			get_child_count_t get_child_count, get_child_t get_child, insert_child_t insert_child = nullptr, remove_child_t remove_child = nullptr)
		: base (name, group, description, ui_visible)
			, _get_child_count(get_child_count)
			, _get_child(get_child)
			, _insert_child(insert_child)
			, _remove_child(remove_child)
		{
			assert (!((insert_child == nullptr) ^ (remove_child == nullptr))); // both must be null or both must be non-null
		}

		virtual size_t size (const object* obj) const override
		{
			auto typed_parent = static_cast<const parent_t*>(obj);
			return (typed_parent->*_get_child_count)();
		}

		virtual object* child_at (const object* parent, size_t index) const override
		{
			auto typed_parent = static_cast<const parent_t*>(parent);
			return (typed_parent->*_get_child)(index);
		}

		virtual bool can_insert_remove() const override { return _insert_child != nullptr; }

		virtual void insert_child (object* parent, size_t index, std::unique_ptr<object>&& child) const override
		{
			auto typed_parent = static_cast<parent_t*>(parent);
			auto raw_child = static_cast<child_t*>(child.release());
			(typed_parent->*_insert_child)(index, std::unique_ptr<child_t>(raw_child));
		}

		virtual std::unique_ptr<object> remove_child (object* parent, size_t index) const override
		{
			auto typed_parent = static_cast<parent_t*>(parent);
			auto typed_child = (typed_parent->*_remove_child)(index);
			auto raw_child = typed_child.release();
			return std::unique_ptr<object>(raw_child);
		}
	};

	struct value_collection_property : collection_property
	{
		using collection_property::collection_property;

		virtual std::string get_value (const object* obj, size_t index) const = 0;
		virtual bool set_value (object* obj, size_t index, std::string_view value) const = 0;
		virtual bool insert_value (object* obj, size_t index, std::string_view value) const = 0;
		virtual void remove_value (object* obj, size_t index) const = 0;
		virtual bool changed (const object* obj) const = 0;
	};

	// TODO: try to get rid of object_t
	template<typename object_t, typename property_traits>
	struct typed_value_collection_property : value_collection_property
	{
		static_assert (std::is_base_of<object, object_t>::value);

		using base = value_collection_property;

		using member_get_size_t = size_t(object_t::*)() const;
		using static_get_size_t = size_t(*)(const object_t*);

		enum accessor_type { member_function, static_function };

		struct get_size_t
		{
			accessor_type const type;
			union
			{
				member_get_size_t mg;
				static_get_size_t sg;
			};

			constexpr get_size_t (member_get_size_t mg) : type(member_function), mg(mg) { }
			constexpr get_size_t (static_get_size_t sg) : type(static_function), sg(sg) { }
		};


		using get_value_t    = typename property_traits::return_t(object_t::*)(size_t) const;
		using set_value_t    = void(object_t::*)(size_t, typename property_traits::param_t);
		using insert_value_t = void(object_t::*)(size_t, typename property_traits::param_t);
		using remove_value_t = void(object_t::*)(size_t);
		using changed_t      = bool(object_t::*)() const;

		get_size_t     const _get_size;
		get_value_t    const _get_value;
		set_value_t    const _set_value;
		insert_value_t const _insert_value;
		remove_value_t const _remove_value;
		changed_t      const _changed;

		constexpr typed_value_collection_property (const char* name, const property_group* group, const char* description, enum ui_visible ui_visible,
			get_size_t get_size, get_value_t get_value, set_value_t set_value, insert_value_t insert_value, remove_value_t remove_value, changed_t changed)
			: base (name, group, description, ui_visible)
			, _get_size(get_size)
			, _get_value(get_value)
			, _set_value(set_value)
			, _insert_value(insert_value)
			, _remove_value(remove_value)
			, _changed(changed)
		{
			// At most one of "set_value" and "insert_value" must be non-null.
			//
			// If set_value is non-null, the object must pre-allocate the collection with default values,
			// and the deserializer must call set_value to overwrite some values.
			//
			// If insert_value is non-null, the object must not pre-allocate the collection,
			// and the deserializer must call insert_value to append some values.
			assert (set_value || insert_value);
		}

		virtual size_t size (const object* obj) const override
		{
			auto ot = static_cast<const object_t*>(obj);

			if (_get_size.type == accessor_type::member_function)
				return (ot->*(_get_size.mg))();

			if (_get_size.type == accessor_type::static_function)
				return _get_size.sg(ot);

			assert(false);
			return 0;
		}

		virtual std::string get_value (const object* obj, size_t index) const override
		{
			auto value = (static_cast<const object_t*>(obj)->*_get_value)(index);
			return property_traits::to_string(value);
		}

		virtual bool set_value (object* obj, size_t index, std::string_view from) const override
		{
			typename property_traits::value_t value;
			bool converted = property_traits::from_string(from, value);
			if (!converted)
				return false;
			(static_cast<object_t*>(obj)->*_set_value) (index, value);
			return true;
		}

		virtual bool insert_value (object* obj, size_t index, std::string_view from) const override
		{
			typename property_traits::value_t value;
			bool converted = property_traits::from_string(from, value);
			if (!converted)
				return false;
			(static_cast<object_t*>(obj)->*_insert_value) (index, value);
			return true;
		}

		virtual void remove_value (object* obj, size_t index) const override
		{
			assert(false); // not implemented
		}

		virtual bool can_insert_remove() const override { return _insert_value != nullptr; }

		virtual bool changed (const object* obj) const override
		{
			const object_t* ot = static_cast<const object_t*>(obj);
			return (ot->*_changed)();
		}
	};
}

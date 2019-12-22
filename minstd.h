
#ifndef EDGE_STD_H
#define EDGE_STD_H

// This file contains a few types that resemble standard ones from C++11/14/17/20.
// They're meant to be used also on embedded systems, where compilers sometimes
// don't have a C++ runtime library, and where heap allocation is not desirable.

#include "assert.h"

namespace edge
{
	template<bool Test, class Ty = void>
	struct enable_if
	{
	};

	template<class Ty>
	struct enable_if<true, Ty>
	{
		using type = Ty;
	};

	template<bool Test, class Ty = void>
	using enable_if_t = typename enable_if<Test, Ty>::type;

	// ========================================================================

	template<class... _Types>
	using void_t = void;

	template <typename From, typename To, typename = void>
	struct is_safely_castable
	{
		static constexpr bool value = false;
	};

	template <typename From, typename To>
	struct is_safely_castable<From, To, void_t<decltype(static_cast<To>(*(From*)(nullptr)))>>
	{
		static constexpr bool value = true;
	};

	template<class From, class To>
	inline constexpr bool is_safely_castable_v = is_safely_castable<From, To>::value;

	// ========================================================================

	template<typename T, size_t Size>
	class array
	{
	public:
		T _elems[Size];

		T* data() { return _elems; }
		const T* data() const { return _elems; }

		size_t size() const { return Size; }

		T* begin() { return _elems; }
		const T* begin() const { return _elems; }

		T* end() { return _elems + Size; }
		const T* end() const { return _elems + Size; }
	};

	template<typename T>
	class array<T, 0>
	{
	public:
		T* data() { return nullptr; }
		const T* data() const { return nullptr; }

		size_t size() const { return 0; }

		T* begin() { return nullptr; }
		const T* begin() const { return nullptr; }

		T* end() { return nullptr; }
		const T* end() const { return nullptr; }
	};

	// ========================================================================

	namespace detail
	{
		template<typename T, bool KnownSize> struct span_members
		{
			T* const _from = nullptr;

			constexpr span_members() = default;
			constexpr span_members(T* from) : _from(from) { }

			constexpr size_t size() const { return KnownSize; }
			constexpr T* to() const { return _from + KnownSize; }
		};

		template<typename T>
		struct span_members<T, false>
		{
			T* const _from = nullptr;
			T* const _to = nullptr;

			span_members() = default;
			span_members(T* from, T* to) : _from(from), _to(to) { }
			span_members(T* from, size_t size) : _from(from), _to(from + size) { }

			constexpr size_t size() const { return (size_t) (_to - _from); }
			constexpr T* to() const { return _to; }
		};
	}

	inline constexpr size_t dynamic_extent = SIZE_MAX;

	template<typename T, size_t Extent = dynamic_extent>
	struct span
	{
		detail::span_members<T, Extent != dynamic_extent> const _members;

	public:
		template <size_t E = Extent, typename enable_if<(E == dynamic_extent || E <= 0), int>::type = 0>
		constexpr span() noexcept
		{ }

		constexpr span (T* ptr, size_t count)
			: _members(ptr, ptr + count)
		{ }

		constexpr span (T* from, T* to) noexcept
			: _members(from, to)
		{ }

		template<size_t N, size_t E = Extent, typename enable_if<(E == dynamic_extent || N == E) /*&& ...*/, int>::type = 0>
		constexpr span (T(&from)[N]) noexcept
			: _members(from, from + N)
		{ }

		template <class Container>
		constexpr span(Container& from)
			: _members(from.data(), from.data() + from.size())
		{ }
		
		template <class Container>
		constexpr span(const Container& from)
			: _members(from.data(), from.data() + from.size())
		{ }

		constexpr T* data() const { return _members._from; }
		constexpr size_t size() const { return _members.size(); }
		constexpr bool empty() const { return _members.size() == 0; }
		
		constexpr T* from() const { return _members._from; }
		constexpr T* to() const { return _members.to(); }

		constexpr T* begin() const { return _members._from; }
		constexpr T* end() const { return _members.to(); }
		
		constexpr const T& operator[] (size_t index) const
		{
			assert(index < size());
			return _members._from[index];
		}

		constexpr T& operator[] (size_t index)
		{
			assert(index < size());
			return _members._from[index];
		}
	};

	// ========================================================================

	struct nullopt_t
	{
		struct init{};
		constexpr explicit nullopt_t(init){}
	};
	constexpr nullopt_t nullopt{nullopt_t::init()};

	template<typename T>
	class optional
	{
		bool _has_value;
		T _value;

	public:
		// 1
		constexpr optional()
			: _has_value(false)
		{ }

		// 2
		constexpr optional( nullopt_t ) noexcept
			: _has_value(false)
		{ }

		template<typename TFrom = T>
		constexpr optional (TFrom&& value)
			: _value(std::forward<TFrom>(value)), _has_value(true)
		{ }

		optional<T>& operator=(const T& value)
		{
			_value = value;
			_has_value = true;
			return *this;
		}

		void reset() { _has_value = false; }

		constexpr bool has_value() const { return _has_value; }

		operator bool() const { return _has_value; }

		T& value()
		{
			assert(_has_value);
			return _value;
		}

		const T& value() const
		{
			assert (_has_value);
			return _value;
		}

		T* operator->()
		{
			assert(_has_value);
			return &_value;
		}

		const T* operator->() const
		{
			assert (_has_value);
			return _value;
		}
	};
}

#endif

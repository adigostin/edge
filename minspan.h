
#ifndef EDGE_STD_H
#define EDGE_STD_H

#include <string>
#include "assert.h"

namespace std
{
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

			constexpr span_members() = default;
			constexpr span_members(T* from, T* to) : _from(from), _to(to) { }
			constexpr span_members(T* from, size_t size) : _from(from), _to(from + size) { }

			constexpr size_t size() const { return (size_t) (_to - _from); }
			constexpr T* to() const { return _to; }
		};
	}

	static constexpr size_t dynamic_extent = SIZE_MAX;

	template<typename T, size_t Extent = dynamic_extent>
	struct span
	{
		detail::span_members<T, Extent != dynamic_extent> const _members;

	public:
		template <size_t E = Extent, typename std::enable_if<(E == dynamic_extent || E <= 0), int>::type = 0>
		constexpr span() noexcept
		{ }

		constexpr span (T* ptr, size_t count) noexcept
			: _members(ptr, ptr + count)
		{ }

		constexpr span (T* from, T* to) noexcept
			: _members(from, to)
		{ }

		template<size_t N, size_t E = Extent, typename std::enable_if<(E == dynamic_extent || N == E) /*&& ...*/, int>::type = 0>
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
}

#endif

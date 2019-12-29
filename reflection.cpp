
#include "reflection.h"
#include "ctype.h"

namespace edge
{
	extern const property_group misc_group = { 0, "Misc" };

	std::string bool_to_string (bool from)
	{
		return from ? "true" : "false";
	}

	bool bool_property_traits::from_string (string_view from, bool& to)
	{
		if ((from.size() == 4) && (tolower(from[0]) == 't') && (tolower(from[1]) == 'r') && (tolower(from[2]) == 'u') && (tolower(from[3]) == 'e'))
		{
			to = true;
			return true;
		}

		if ((from.size() == 5) && (tolower(from[0]) == 'f') && (tolower(from[1]) == 'a') && (tolower(from[2]) == 'l') && (tolower(from[3]) == 's') && (tolower(from[4]) == 'e'))
		{
			to = false;
			return true;
		}

		return false;
	}

	bool int32_property_traits::from_string (string_view from, int32_t& to)
	{
		if (from.empty())
			return false;

		char* endPtr;
		long value = std::strtol (from.data(), &endPtr, 10);

		if (endPtr != from.data() + from.size())
			return false;

		to = value;
		return true;
	}

	bool uint32_property_traits::from_string (string_view from, uint32_t& to)
	{
		if (from.empty())
			return false;

		char* endPtr;
		unsigned long value = std::strtoul (from.data(), &endPtr, 10);

		if (endPtr != from.data() + from.size())
			return false;

		to = value;
		return true;
	}

	bool uint64_property_traits::from_string (string_view from, uint64_t& to)
	{
		if (from.empty())
			return false;

		char* endPtr;
		unsigned long long value = std::strtoull (from.data(), &endPtr, 10);

		if (endPtr != from.data() + from.size())
			return false;

		to = value;
		return true;
	}

	template<typename IntType, typename enable_if<sizeof(IntType) == 4>::type* = nullptr>
	static bool size_t_from_string (string_view from, IntType& to)
	{
		return uint32_property_traits::from_string(from, to);
	}

	template<typename IntType, typename enable_if<sizeof(IntType) == 8>::type* = nullptr>
	static bool size_t_from_string (string_view from, IntType& to)
	{
		return uint64_property_traits::from_string(from, to);
	}

	bool size_t_property_traits::from_string (string_view from, size_t&to)
	{
		return size_t_from_string (from, to);
	}

	bool float_property_traits::from_string (string_view from, float& to)
	{
		if (from.empty())
			return false;

		char* end_ptr;
		float value = std::strtof (from.data(), &end_ptr);

		if (end_ptr != from.data() + from.size())
			return false;

		to = value;
		return true;
	}

	const char unknown_enum_value_str[] = "(unknown)";
}

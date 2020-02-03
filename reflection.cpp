
#include "reflection.h"
#include <ctype.h>

namespace edge
{
	extern const property_group misc_group = { 0, "Misc" };

	// ========================================================================

	const char bool_property_traits::type_name[] = "bool";

	std::string bool_to_string (bool from)
	{
		return from ? "true" : "false";
	}

	bool bool_property_traits::from_string (std::string_view from, bool& to)
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

	// ========================================================================

	extern const char int32_type_name[] = "int32";

	template<> std::string int32_property_traits::to_string (int32_t from)
	{
		char buffer[16];
		#ifdef _MSC_VER
			sprintf_s (buffer, "%d", from);
		#else
			sprintf (buffer, "%d", from);
		#endif
		return buffer;
	}

	template<> bool int32_property_traits::from_string (std::string_view from, int32_t& to)
	{
		if (from.empty())
			return false;

		char* endPtr;
		long value = strtol (from.data(), &endPtr, 10);

		if (endPtr != from.data() + from.size())
			return false;

		to = value;
		return true;
	}

	// ========================================================================

	extern const char uint32_type_name[] = "uint32";

	template<> std::string uint32_property_traits::to_string (uint32_t from)
	{
		char buffer[16];
		#ifdef _MSC_VER
		sprintf_s (buffer, "%u", from);
		#else
		sprintf (buffer, "%u", from);
		#endif
		return buffer;
	}

	template<> bool uint32_property_traits::from_string (std::string_view from, uint32_t& to)
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

	// ========================================================================

	extern const char uint64_type_name[] = "uint64";

	template<> std::string uint64_property_traits::to_string (uint64_t from)
	{
		char buffer[32];
		#ifdef _MSC_VER
		sprintf_s (buffer, "%llu", from);
		#else
		sprintf (buffer, "%llu", from);
		#endif
		return buffer;
	}

	template<> bool uint64_property_traits::from_string (std::string_view from, uint64_t& to)
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

	// ========================================================================

	extern const char size_t_type_name[] = "size_t";

	template<> std::string size_t_property_traits::to_string (size_t from)
	{
		return uint32_property_traits::to_string((uint32_t)from);
	}

	template<> bool size_t_property_traits::from_string (std::string_view from, size_t&to)
	{
		uint32_t val;
		bool res = uint32_property_traits::from_string (from, val);
		if (res)
			to = val;
		return res;
	}

	// ========================================================================

	extern const char float_type_name[] = "float";

	template<> std::string float_property_traits::to_string (float from)
	{
		char buffer[32];
		#ifdef _MSC_VER
			sprintf_s (buffer, "%f", from);
		#else
			sprintf (buffer, "%f", from);
		#endif
		return buffer;
	}

	template<> bool float_property_traits::from_string (std::string_view from, float& to)
	{
		if (from.empty())
			return false;

		char* end_ptr;
		float value = strtof (from.data(), &end_ptr);

		if (end_ptr != from.data() + from.size())
			return false;

		to = value;
		return true;
	}

	// ========================================================================

	void backed_string_property_traits::serialize (std::string_view from, out_stream_i* to)
	{
		if (from.size() < 254)
		{
			to->write((uint8_t)from.size());
		}
		else if (from.size() < 65536)
		{
			to->write((uint8_t)254);
			to->write ((uint8_t)(from.size()));
			to->write ((uint8_t)(from.size() >> 8));
		}
		else
		{
			to->write((uint8_t)255);
			to->write ((uint8_t)(from.size()));
			to->write ((uint8_t)(from.size() >> 8));
			to->write ((uint8_t)(from.size() >> 16));
			to->write ((uint8_t)(from.size() >> 24));
		}
		
		to->write (from.data(), from.size());
	}

	std::string_view backed_string_property_traits::deserialize (binary_reader& from)
	{
		assert (from.ptr + 1 <= from.end);
		size_t len = *from.ptr++;

		if (len == 254)
		{
			assert (from.ptr + 2 <= from.end);
			len = *from.ptr++;
			len |= (*from.ptr++ << 8);
		}
		else if (len == 255)
		{
			assert (from.ptr + 4 <= from.end);
			len = *from.ptr++;
			len |= (*from.ptr++ << 8);
			len |= (*from.ptr++ << 16);
			len |= (*from.ptr++ << 24);
		}

		assert (from.ptr + len <= from.end);
		auto res = std::string_view ((const char*)from.ptr, len);
		from.ptr += len;
		return res;
	}

	// ========================================================================

	const char unknown_enum_value_str[] = "(unknown)";
}

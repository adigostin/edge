
#include "pch.h"
#include "edge_win32.h"
#include "utility_functions.h"

namespace edge
{
	#pragma region win32_window_i interface
	bool win32_window_i::visible() const
	{
		return (GetWindowLongPtr (hwnd(), GWL_STYLE) & WS_VISIBLE) != 0;
	}

	RECT win32_window_i::client_rect_pixels() const
	{
		RECT rect;
		BOOL bRes = ::GetClientRect (hwnd(), &rect); assert(bRes);
		return rect;
	};

	SIZE win32_window_i::client_size_pixels() const
	{
		RECT rect = this->client_rect_pixels();
		return SIZE { rect.right, rect.bottom };
	}

	RECT win32_window_i::GetRect() const
	{
		auto hwnd = this->hwnd();
		auto parent = ::GetParent(hwnd); assert (parent != nullptr);
		RECT rect;
		BOOL bRes = ::GetWindowRect (hwnd, &rect); assert(bRes);
		MapWindowPoints (HWND_DESKTOP, parent, (LPPOINT) &rect, 2);
		return rect;
	}

	POINT win32_window_i::GetLocation() const
	{
		auto rect = GetRect();
		return { rect.left, rect.top };
	}

	LONG win32_window_i::width_pixels() const
	{
		RECT rect;
		::GetWindowRect (hwnd(), &rect);
		return rect.right - rect.left;
	}

	LONG win32_window_i::height_pixels() const
	{
		RECT rect;
		::GetWindowRect (hwnd(), &rect);
		return rect.bottom - rect.top;
	}

	SIZE win32_window_i::size_pixels() const
	{
		RECT rect;
		::GetWindowRect (hwnd(), &rect);
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	void win32_window_i::move_window (const RECT& rect)
	{
		BOOL bRes = ::MoveWindow (hwnd(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		assert(bRes);
	}
	#pragma endregion

	#pragma region zoomable_i interface
	D2D1_POINT_2F zoomable_i::pointw_to_pointd (D2D1_POINT_2F location) const
	{
		pointw_to_pointd ({ &location, 1 });
		return location;
	}

	bool zoomable_i::hit_test_line (D2D1_POINT_2F dLocation, float tolerance, D2D1_POINT_2F p0w, D2D1_POINT_2F p1w, float lineWidth) const
	{
		auto fd = this->pointw_to_pointd(p0w);
		auto td = this->pointw_to_pointd(p1w);

		float halfw = this->lengthw_to_lengthd(lineWidth) / 2.0f;
		if (halfw < tolerance)
			halfw = tolerance;

		float angle = atan2(td.y - fd.y, td.x - fd.x);
		float s = sin(angle);
		float c = cos(angle);

		std::array<D2D1_POINT_2F, 4> vertices =
		{
			D2D1_POINT_2F { fd.x + s * halfw, fd.y - c * halfw },
			D2D1_POINT_2F { fd.x - s * halfw, fd.y + c * halfw },
			D2D1_POINT_2F { td.x - s * halfw, td.y + c * halfw },
			D2D1_POINT_2F { td.x + s * halfw, td.y - c * halfw }
		};

		return point_in_polygon (vertices, dLocation);
	}
	#pragma endregion
}

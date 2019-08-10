
#pragma once

inline D2D1_SIZE_F operator- (D2D1_POINT_2F p0, D2D1_POINT_2F p1) { return { p0.x - p1.x, p0.y - p1.y }; }
inline D2D1_POINT_2F operator- (D2D1_POINT_2F p, D2D1_SIZE_F s) {return { p.x - s.width, p.y - s.height }; }
inline D2D1_POINT_2F operator+ (D2D1_POINT_2F p, D2D1_SIZE_F s) {return { p.x + s.width, p.y + s.height }; }
inline bool operator== (D2D1_POINT_2F p0, D2D1_POINT_2F p1) { return (p0.x == p1.x) && (p0.y == p1.y); }
inline bool operator!= (D2D1_POINT_2F p0, D2D1_POINT_2F p1) { return (p0.x != p1.x) || (p0.y != p1.y); }
bool operator== (const D2D1_RECT_F& a, const D2D1_RECT_F& b);
bool operator!= (const D2D1_RECT_F& a, const D2D1_RECT_F& b);
inline bool operator== (POINT a, POINT b) { return (a.x == b.x) && (a.y == b.y); }
inline bool operator!= (POINT a, POINT b) { return (a.x != b.x) || (a.y != b.y); }
inline bool operator== (SIZE a, SIZE b) { return (a.cx == b.cx) && (a.cy == b.cy); }
inline bool operator!= (SIZE a, SIZE b) { return (a.cx != b.cx) || (a.cy != b.cy); }
inline bool operator== (const D2D1_MATRIX_3X2_F& a, const D2D1_MATRIX_3X2_F& b) { return memcmp(&a, &b, sizeof(D2D1_MATRIX_3X2_F)) == 0; }
inline bool operator== (const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) { return memcmp(&a, &b, sizeof(D2D1_COLOR_F)) == 0; }
inline bool operator!= (const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) { return memcmp(&a, &b, sizeof(D2D1_COLOR_F)) != 0; }

namespace edge
{
	bool point_in_rect(const D2D1_RECT_F& rect, D2D1_POINT_2F location);
	bool point_in_polygon(const std::array<D2D1_POINT_2F, 4>& vertices, D2D1_POINT_2F point);
	D2D1_RECT_F InflateRect (const D2D1_RECT_F& rect, float distance);
	void InflateRect (D2D1_RECT_F* rect, float distance);
	D2D1_ROUNDED_RECT InflateRoundedRect (const D2D1_ROUNDED_RECT& rr, float distance);
	void InflateRoundedRect (D2D1_ROUNDED_RECT* rr, float distance);
	D2D1::ColorF GetD2DSystemColor (int sysColorIndex);
	std::string get_window_text (HWND hwnd);
	std::array<D2D1_POINT_2F, 4> corners (const D2D1_RECT_F& rect);
	D2D1_RECT_F polygon_bounds (const std::array<D2D1_POINT_2F, 4>& points);
	D2D1_COLOR_F interpolate (const D2D1_COLOR_F& first, const D2D1_COLOR_F& second, uint32_t percent_first);
	D2D1_RECT_F align_to_pixel (const D2D1_RECT_F& rect, uint32_t dpi);
	std::string utf16_to_utf8 (std::wstring_view str_utf16);
	std::string bstr_to_utf8 (BSTR bstr);
	inline D2D1_POINT_2F location (const D2D1_RECT_F& r) { return { r.left, r.top }; }
	inline D2D1_SIZE_F size (const D2D1_RECT_F& r) { return { r.right - r.left, r.bottom - r.top }; }
	inline D2D1_POINT_2F center (const D2D1_RECT_F& r) { return { (r.left + r.right) / 2, (r.top + r.bottom) / 2 }; }
	inline LONG width (const RECT& rc) { return rc.right - rc.left; }
	inline LONG height (const RECT& rc) { return rc.bottom - rc.top; }
}

struct timer_queue_timer_deleter
{
	void operator() (HANDLE handle) { ::DeleteTimerQueueTimer(nullptr, handle, INVALID_HANDLE_VALUE); }
};
using timer_queue_timer_unique_ptr = std::unique_ptr<std::remove_pointer_t<HANDLE>, timer_queue_timer_deleter>;
inline timer_queue_timer_unique_ptr create_timer_queue_timer (WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period)
{
	HANDLE handle;
	BOOL bres = ::CreateTimerQueueTimer (&handle, nullptr, Callback, Parameter, DueTime, Period, 0);
	assert(bres);
	return timer_queue_timer_unique_ptr(handle);
}

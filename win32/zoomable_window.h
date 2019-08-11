
#pragma once
#include "d2d_window.h"

namespace edge
{
	struct zoomable_i : public win32_window_i
	{
		struct zoom_transform_changed_e : event<zoom_transform_changed_e, zoomable_i*> { };

		virtual D2D1::Matrix3x2F zoom_transform() const = 0;
		virtual D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const = 0;
		virtual D2D1_POINT_2F pointw_to_pointd (D2D1_POINT_2F wlocation) const = 0;
		virtual float lengthw_to_lengthd (float  wlength) const = 0;
		virtual zoom_transform_changed_e::subscriber zoom_transform_changed() = 0;

		bool hit_test_line (D2D1_POINT_2F dLocation, float tolerance, D2D1_POINT_2F p0w, D2D1_POINT_2F p1w, float lineWidth) const;
	};

	class zoomable_window abstract : public d2d_window, public zoomable_i
	{
		using base = d2d_window;

		D2D1_POINT_2F _aimpoint = { 0, 0 }; // workspace coordinate shown at the center of the client area
		float _zoom = 1;
		float _minDistanceBetweenGridPoints = 15;
		float _minDistanceBetweenGridLines = 40;
		bool _enableUserZoomingAndPanning = true;
		bool _panning = false;
		D2D1_POINT_2F _panningLastMouseLocation;

		struct smooth_zoom_info
		{
			LARGE_INTEGER begin_time;
			float         begin_zoom;
			D2D1_POINT_2F begin_aimpoint;
			float         end_zoom;
			D2D1_POINT_2F end_aimpoint;
		};
		std::optional<smooth_zoom_info> _smooth_zoom_info;

		struct zoomed_to_rect
		{
			D2D1_RECT_F rect;
			float min_margin;
			float min_zoom;
			float max_zoom;
		};
		std::optional<zoomed_to_rect> _zoomed_to_rect;

	public:
		using base::base;

		float zoom() const { return _zoom; }
		D2D1_POINT_2F aimpoint() const { return _aimpoint; }
		void zoom_to (const D2D1_RECT_F& rect, float min_margin, float min_zoom, float max_zoom, bool smooth);
		void zoom_to (D2D1_POINT_2F aimpoint, float zoom, bool smooth);

		using base::hwnd;
		using base::invalidate;

		// zoomable_i
		virtual D2D1::Matrix3x2F zoom_transform() const override;
		virtual D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const override;
		virtual D2D1_POINT_2F pointw_to_pointd (D2D1_POINT_2F wlocation) const override;
		virtual float lengthw_to_lengthd (float wLength) const override { return wLength * _zoom; }
		virtual zoom_transform_changed_e::subscriber zoom_transform_changed() override { return zoom_transform_changed_e::subscriber(this); }

	protected:
		virtual std::optional<LRESULT> window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual void create_render_resources (ID2D1DeviceContext* dc) override;
		virtual void release_render_resources (ID2D1DeviceContext* dc) override;
		virtual void on_zoom_transform_changed();

	private:
		void set_zoom_and_aimpoint_internal (float zoom, D2D1_POINT_2F aimpoint, bool smooth);
		void process_wm_size        (WPARAM wparam, LPARAM lparam);
		void process_wm_mbuttondown (WPARAM wparam, LPARAM lparam);
		void process_wm_mbuttonup   (WPARAM wparam, LPARAM lparam);
		void process_wm_mousewheel  (WPARAM wparam, LPARAM lparam);
		void process_wm_mousemove   (WPARAM wparam, LPARAM lparam);
	};
}

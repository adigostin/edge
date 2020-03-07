
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "d2d_window.h"

namespace edge
{
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

		D2D1_SIZE_F pixel_aligned_window_center() const;

	public:
		using base::base;

		float zoom() const { return _zoom; }
		D2D1_POINT_2F aimpoint() const { return _aimpoint; }
		void zoom_to (D2D1_POINT_2F aimpoint, float zoom, bool smooth);

		using base::invalidate;
		using zoomable_i::pointw_to_pointd;

		// zoomable_i
		virtual D2D1::Matrix3x2F zoom_transform() const override;
		virtual D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const override;
		virtual void pointw_to_pointd (std::span<D2D1_POINT_2F> locations) const override;
		virtual float lengthw_to_lengthd (float wLength) const override { return wLength * _zoom; }
		virtual zoom_transform_changed_e::subscriber zoom_transform_changed() override { return zoom_transform_changed_e::subscriber(this); }
		virtual void zoom_to (const D2D1_RECT_F& rect, float min_margin, float min_zoom, float max_zoom, bool smooth) override;

		// win32_window_i
		virtual HWND hwnd() const override { return base::hwnd(); }

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

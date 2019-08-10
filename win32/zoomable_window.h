
#pragma once
#include "d2d_window.h"

namespace edge
{
	struct zoomable_i : public win32_window_i
	{
		virtual D2D1::Matrix3x2F zoom_transform() const = 0;
		virtual D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const = 0;
		virtual D2D1_POINT_2F pointw_to_pointd (D2D1_POINT_2F wlocation) const = 0;
		virtual float lengthw_to_lengthd (float  wlength) const = 0;
		//virtual zoom_transform_changed_event::subscriber zoom_transform_changed() = 0;

		bool hit_test_line (D2D1_POINT_2F dLocation, float tolerance, D2D1_POINT_2F p0w, D2D1_POINT_2F p1w, float lineWidth) const;
	};

	class zoomable_window abstract : public d2d_window, public zoomable_i
	{
		using base = d2d_window;

		static constexpr float ZoomFactor = 1.5f;
		D2D1_POINT_2F _workspaceOrigin = { 0, 0 }; // Location in client area of the point (0;0) of the workspace
		float _zoom = 1.0f;
		float _minDistanceBetweenGridPoints = 15;
		float _minDistanceBetweenGridLines = 40;
		bool _enableUserZoomingAndPanning = true;
		bool _panning = false;
		D2D1_POINT_2F _panningLastMouseLocation;

		struct SmoothZoomInfo
		{
			float _zoomStart;
			float _woXStart;
			float _woYStart;
			float _zoomEnd;
			float _woXEnd;
			float _woYEnd;
			LARGE_INTEGER _zoomStartTime;
		};
		std::unique_ptr<SmoothZoomInfo> _smoothZoomInfo;

		struct ZoomedToRect
		{
			D2D1_RECT_F  _rect;
			float _minMarginDips;
			float _maxZoomOrZero;
		};
		std::unique_ptr<ZoomedToRect> _zoomedToRect;

	public:
		using base::base;

		float GetZoom() const { return _zoom; }
		D2D1_POINT_2F GetWorkspaceOrigin() const { return _workspaceOrigin; }
		float GetWorkspaceOriginX() const { return _workspaceOrigin.x; }
		float GetWorkspaceOriginY() const { return _workspaceOrigin.y; }
		void ZoomToRectangle(const D2D1_RECT_F& rect, float minMarginDips, float maxZoomOrZero, bool smooth);
		void SetZoomAndOrigin(float zoom, float originX, float originY, bool smooth);

		using base::hwnd;
		using base::invalidate;

		// zoomable_i
		virtual D2D1::Matrix3x2F zoom_transform() const override;
		virtual D2D1_POINT_2F pointd_to_pointw (D2D1_POINT_2F dlocation) const override;
		virtual D2D1_POINT_2F pointw_to_pointd (D2D1_POINT_2F wlocation) const override;
		virtual float lengthw_to_lengthd (float wLength) const override { return wLength * _zoom; }
		//virtual zoom_transform_changed_event::subscriber zoom_transform_changed() override { return zoom_transform_changed_event::subscriber(this); }

	protected:
		virtual std::optional<LRESULT> window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual void create_render_resources (ID2D1DeviceContext* dc) override;
		virtual void release_render_resources (ID2D1DeviceContext* dc) override;
		virtual void OnZoomTransformChanged();

	private:
		void SetZoomAndOriginInternal (float zoom, float originX, float originY, bool smooth);
		void ProcessWmSize (WPARAM wparam, LPARAM lparam);
		bool ProcessWmMButtonDown(WPARAM wparam, LPARAM lparam);
		bool ProcessWmMButtonUp(WPARAM wparam, LPARAM lparam);
		bool ProcessWmMouseWheel(WPARAM wparam, LPARAM lparam);
		void ProcessWmMouseMove(WPARAM wparam, LPARAM lparam);
	};
}

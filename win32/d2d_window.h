
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "window.h"
#include "com_ptr.h"
#include "utility_functions.h"

namespace edge
{
	#pragma warning(disable: 4250) // disable "inherits via dominance" warning

	class d2d_window abstract : public window, public virtual d2d_window_i
	{
		using base = window;
		bool _painting = false;
		bool _forceFullPresentation;
		com_ptr<IDWriteFactory> const _dwrite_factory;
		com_ptr<ID3D11Device1> _d3dDevice;
		com_ptr<ID3D11DeviceContext1> _d3d_dc;
		com_ptr<IDXGIDevice2> _dxgiDevice;
		com_ptr<IDXGIAdapter> _dxgiAdapter;
		com_ptr<IDXGIFactory2> _dxgiFactory;
		com_ptr<IDXGISwapChain1> _swapChain;
		com_ptr<ID2D1DeviceContext> _d2dDeviceContext;
		com_ptr<ID2D1Factory1> _d2dFactory;
		timer_queue_timer_unique_ptr _caret_blink_timer;
		bool _caret_blink_on = false;
		std::pair<D2D1_RECT_F, D2D1_MATRIX_3X2_F> _caret_bounds;
		D2D1_COLOR_F _caret_color;

		struct RenderPerfInfo
		{
			LARGE_INTEGER startTime;
			float durationMilliseconds;
		};

		LARGE_INTEGER _performanceCounterFrequency;
		std::deque<RenderPerfInfo> perfInfoQueue;

		enum class DebugFlag
		{
			RenderFrameDurationsAndFPS = 1,
			RenderUpdateRects = 2,
			FullClear = 4,
		};
		//virtual bool GetDebugFlag (DebugFlag flag) const = 0;
		//virtual void SetDebugFlag (DebugFlag flag, bool value) = 0;

		DebugFlag _debugFlags = (DebugFlag)0; //DebugFlag::RenderFrameDurationsAndFPS;
		com_ptr<IDWriteTextFormat> _debugTextFormat;

		static constexpr UINT WM_BLINK = base::WM_NEXT + 0;
	protected:
		static constexpr UINT WM_NEXT = base::WM_NEXT + 1;

	public:
		d2d_window (HINSTANCE hInstance, DWORD exStyle, DWORD style,
				   const RECT& rect, HWND hWndParent, int child_control_id,
				   ID3D11DeviceContext1* d3d_dc, IDWriteFactory* dwrite_factory);

		ID3D11DeviceContext1* d3d_dc() const { return _d3d_dc; }
		ID2D1Factory1* d2d_factory() const { return _d2dFactory; }

		// d2d_window_i
		virtual ID2D1DeviceContext* dc() const { return _d2dDeviceContext; }
		virtual IDWriteFactory* dwrite_factory() const override { return _dwrite_factory; }
		virtual void show_caret (const D2D1_RECT_F& bounds, D2D1_COLOR_F color, const D2D1_MATRIX_3X2_F* transform = nullptr) override;
		virtual void hide_caret() override;

		float GetFPS();
		float GetAverageRenderDuration();

	protected:
		virtual std::optional<LRESULT> window_proc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual void create_render_resources (ID2D1DeviceContext* dc) { }
		virtual void render (ID2D1DeviceContext* dc) const;
		virtual void release_render_resources (ID2D1DeviceContext* dc) { }

		virtual void d2d_dc_releasing() { }
		virtual void d2d_dc_recreated() { }

		virtual void on_client_size_changed (SIZE client_size_pixels, D2D1_SIZE_F client_size_dips) override;

	private:
		void invalidate_caret();
		void process_wm_blink();
		void create_d2d_dc();
		void release_d2d_dc();
		void process_wm_paint();
		void process_wm_set_focus();
		void process_wm_kill_focus();
	};
}

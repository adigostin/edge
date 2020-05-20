
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#include "pch.h"
#include "utility_functions.h"
#include "com_ptr.h"

#pragma comment (lib, "Shlwapi")
#pragma comment (lib, "Shell32")

using namespace D2D1;

bool operator== (const D2D1_RECT_F& a, const D2D1_RECT_F& b)
{
	return memcmp (&a, &b, sizeof(D2D1_RECT_F)) == 0;
}

bool operator!= (const D2D1_RECT_F& a, const D2D1_RECT_F& b)
{
	return memcmp (&a, &b, sizeof(D2D1_RECT_F)) != 0;
}

namespace edge
{
	bool point_in_rect(const D2D1_RECT_F& rect, D2D1_POINT_2F pt)
	{
		return (pt.x >= rect.left) && (pt.x < rect.right) && (pt.y >= rect.top) && (pt.y < rect.bottom);
	}

	bool point_in_rect (const RECT& rect, POINT pt)
	{
		return (pt.x >= rect.left) && (pt.x < rect.right) && (pt.y >= rect.top) && (pt.y < rect.bottom);
	}

	bool point_in_polygon(const std::array<D2D1_POINT_2F, 4>& vertices, D2D1_POINT_2F point)
	{
		// Taken from http://stackoverflow.com/a/2922778/451036
		bool c = false;
		size_t vertexCount = 4;
		for (size_t i = 0, j = vertexCount - 1; i < vertexCount; j = i++)
		{
			if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
				(point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) / (vertices[j].y - vertices[i].y) + vertices[i].x))
				c = !c;
		}

		return c;
	}

	D2D1_RECT_F inflate (const D2D1_RECT_F& rect, float distance)
	{
		auto result = rect;
		inflate (&result, distance);
		return result;
	}

	void inflate (D2D1_RECT_F* rect, float distance)
	{
		rect->left -= distance;
		rect->top -= distance;
		rect->right += distance;
		rect->bottom += distance;
	}

	D2D1_RECT_F inflate (D2D1_POINT_2F p, float distance)
	{
		return { p.x - distance, p.y - distance, p.x + distance, p.y + distance };
	}

	D2D1_ROUNDED_RECT inflate (const D2D1_ROUNDED_RECT& rr, float distance)
	{
		D2D1_ROUNDED_RECT result = rr;
		inflate (&result, distance);
		return result;
	}

	void inflate (D2D1_ROUNDED_RECT* rr, float distance)
	{
		inflate (&rr->rect, distance);

		rr->radiusX += distance;
		if (rr->radiusX < 0)
			rr->radiusX = 0;

		rr->radiusY += distance;
		if (rr->radiusY < 0)
			rr->radiusY = 0;
	}

	ColorF GetD2DSystemColor (int sysColorIndex)
	{
		DWORD brg = GetSysColor (sysColorIndex);
		DWORD rgb = ((brg & 0xff0000) >> 16) | (brg & 0xff00) | ((brg & 0xff) << 16);
		return ColorF (rgb);
	}

	std::string get_window_text (HWND hwnd)
	{
		int char_count = ::GetWindowTextLength(hwnd);
		auto wstr = std::make_unique<wchar_t[]>(char_count + 1);
		::GetWindowTextW (hwnd, wstr.get(), char_count + 1);
		int size_bytes = WideCharToMultiByte (CP_UTF8, 0, wstr.get(), (int) char_count, nullptr, 0, nullptr, nullptr);
		auto str = std::string (size_bytes, 0);
		WideCharToMultiByte (CP_UTF8, 0, wstr.get(), (int) char_count, str.data(), size_bytes, nullptr, nullptr);
		return str;
	};

	std::array<D2D1_POINT_2F, 4> corners (const D2D1_RECT_F& rect)
	{
		return {
			D2D1_POINT_2F{ rect.left, rect.top },
			D2D1_POINT_2F{ rect.right, rect.top },
			D2D1_POINT_2F{ rect.right, rect.bottom },
			D2D1_POINT_2F{ rect.left, rect.bottom },
		};
	}

	D2D1_RECT_F polygon_bounds (const std::array<D2D1_POINT_2F, 4>& points)
	{
		D2D1_RECT_F r = { points[0].x, points[0].y, points[0].x, points[0].y };

		for (size_t i = 1; i < 4; i++)
		{
			r.left   = std::min (r.left  , points[i].x);
			r.top    = std::min (r.top   , points[i].y);
			r.right  = std::max (r.right , points[i].x);
			r.bottom = std::max (r.bottom, points[i].y);
		}

		return r;
	}

	D2D1_COLOR_F interpolate (const D2D1_COLOR_F& first, const D2D1_COLOR_F& second, uint32_t percent_first)
	{
		assert (percent_first <= 100);
		float r = (first.r * percent_first + second.r * (100 - percent_first)) / 100;
		float g = (first.g * percent_first + second.g * (100 - percent_first)) / 100;
		float b = (first.b * percent_first + second.b * (100 - percent_first)) / 100;
		float a = (first.a * percent_first + second.a * (100 - percent_first)) / 100;
		return { r, g, b, a };
	}

	D2D1_RECT_F align_to_pixel (const D2D1_RECT_F& rect, uint32_t dpi)
	{
		float pixel_width = 96.0f / dpi;
		D2D1_RECT_F result;
		result.left   = round(rect.left   / pixel_width) * pixel_width;
		result.top    = round(rect.top    / pixel_width) * pixel_width;
		result.right  = round(rect.right  / pixel_width) * pixel_width;
		result.bottom = round(rect.bottom / pixel_width) * pixel_width;
		return result;
	}

	std::wstring utf8_to_utf16 (std::string_view str_utf8)
	{
		if (str_utf8.empty())
			return { };

		int char_count = MultiByteToWideChar (CP_UTF8, 0, str_utf8.data(), (int)str_utf8.size(), nullptr, 0);
		std::wstring wide (char_count, 0);
		MultiByteToWideChar (CP_UTF8, 0, str_utf8.data(), (int)str_utf8.size(), wide.data(), char_count);
		return wide;
	}

	std::string utf16_to_utf8 (std::wstring_view str_utf16)
	{
		if (str_utf16.empty())
			return { };

		int char_count = WideCharToMultiByte (CP_UTF8, 0, str_utf16.data(), (int) str_utf16.size(), nullptr, 0, nullptr, nullptr);
		std::string str (char_count, 0);
		WideCharToMultiByte (CP_UTF8, 0, str_utf16.data(), (int) str_utf16.size(), str.data(), char_count, nullptr, nullptr);
		return str;
	}

	std::string bstr_to_utf8 (BSTR bstr)
	{
		size_t char_count = SysStringLen(bstr);
		return utf16_to_utf8 ({ bstr, char_count });
	}

	D2D1_RECT_F make_positive (const D2D1_RECT_F& rect)
	{
		float l = std::min (rect.left, rect.right);
		float t = std::min (rect.top,  rect.bottom);
		float r = std::max (rect.left, rect.right);
		float b = std::max (rect.top,  rect.bottom);
		return { l, t, r, b };
	}

	D2D1_RECT_F union_rects (const D2D1_RECT_F& one, const D2D1_RECT_F& other)
	{
		auto aa = make_positive(one);
		auto bb = make_positive(other);
		float l = std::min (aa.left, bb.left);
		float t = std::min (aa.top, bb.top);
		float r = std::max (aa.right, bb.right);
		float b = std::max (aa.bottom, bb.bottom);
		return { l, t, r, b };
	}

	bool try_choose_file_path (open_or_save which, HWND fileDialogParentHWnd, const wchar_t* pathToInitializeDialogTo, std::wstring& sbOut)
	{
		com_ptr<IFileDialog> dialog;
		HRESULT hr = CoCreateInstance ((which == open_or_save::save) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, __uuidof(dialog), (void**) &dialog);
		if (FAILED(hr))
			throw com_exception(hr);

		static const COMDLG_FILTERSPEC ProjectFileDialogFileTypes[] =
		{
			{ L"Dash Files", L"*.dash" },
			{ L"All Files",     L"*.*" },
		};
		static const wchar_t ProjectFileExtensionWithoutDot[] = L"dash";

		//DWORD options;
		//hr = dialog->GetOptions (&options); rassert_hr(hr);
		//hr = dialog->SetOptions (options | FOS_FORCEFILESYSTEM); rassert_hr(hr);
		hr = dialog->SetFileTypes (std::size(ProjectFileDialogFileTypes), ProjectFileDialogFileTypes);
		if (FAILED(hr))
			throw com_exception(hr);

		hr = dialog->SetDefaultExtension (ProjectFileExtensionWithoutDot);
		if (FAILED(hr))
			throw com_exception(hr);

		if ((pathToInitializeDialogTo != nullptr) && (pathToInitializeDialogTo[0] != 0))
		{
			auto filePtr = PathFindFileName(pathToInitializeDialogTo);
			hr = dialog->SetFileName(filePtr);
			if (FAILED(hr))
				throw com_exception(hr);

			std::wstring dir (pathToInitializeDialogTo, filePtr - pathToInitializeDialogTo);
			com_ptr<IShellItem> si;
			hr = SHCreateItemFromParsingName (dir.c_str(), nullptr, IID_PPV_ARGS(&si));
			if (SUCCEEDED(hr))
				dialog->SetFolder(si);
		}

		hr = dialog->Show(fileDialogParentHWnd);
		if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
			return false;
		if (FAILED(hr))
			throw com_exception(hr);

		com_ptr<IShellItem> item;
		hr = dialog->GetResult (&item);
		if (FAILED(hr))
			throw com_exception(hr);

		co_task_mem_ptr<wchar_t> filePath;
		hr = item->GetDisplayName (SIGDN_FILESYSPATH, &filePath);
		if (FAILED(hr))
			throw com_exception(hr);
		sbOut = filePath.get();

		SHAddToRecentDocs(SHARD_PATHW, sbOut.c_str());
		return true;
	}
}

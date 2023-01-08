#pragma once

#include "Scintilla.h"
#include "resource.h"
#include "controls.h"

class SciEdit //: public SimpleControl
{
public:
	SciEdit(HWND hwndParent, int id, BOOL initialState = TRUE)//		:	SimpleControl (hwndParent, id, initialState)
	{
		UNREFERENCED_PARAMETER(id);
		Scintilla_RegisterClasses(GetModuleHandle(NULL));
		hwnd = ::CreateWindow(
			"Scintilla",
			"Source",
			WS_BORDER | WS_CHILD | WS_VSCROLL,
			25, 135, 320, 300,
			hwndParent,
			0,
			GetModuleHandle(NULL),
			0);
		::ShowWindow(hwnd, SW_SHOW);
		::SetFocus(hwnd);
		SendEditor(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
		SendEditor(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>("courier new"));
		Enable(initialState);
	}
	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
	{
		return ::SendMessage(hwnd, Msg, wParam, lParam);
	}
	void SetFocus()
	{
		::SetFocus(hwnd);
	}
	void Show(BOOL state)
	{
		int show = state ? SW_SHOW : SW_HIDE;
		::ShowWindow(hwnd, show);
	}
	void Enable(BOOL state)
	{
		::EnableWindow(hwnd, state);
	}
	BOOL IsVisible()
	{
		return (::IsWindowVisible(hwnd));
	}
	HWND Hwnd() const
	{
		return hwnd;
	}
protected:
	HWND hwnd;
};

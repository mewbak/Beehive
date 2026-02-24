#pragma once

#include <wx/window.h>
#include "Palette.h"

class PaletteViewCtrl : public wxWindow
{
public:

	static const int s_numColours = 16;

	PaletteViewCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);

	void SetPalette(const Palette& palette);

	void OnPaint(wxPaintEvent& evt);

private:

	std::vector<wxBrush> m_brushes;

	DECLARE_EVENT_TABLE()
};

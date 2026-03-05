#include "PaletteViewCtrl.h"
#include <wx/dc.h>
#include <wx/dcclient.h>

PaletteViewCtrl::PaletteViewCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	: wxWindow(parent, id, pos, size, style)
{
	m_brushes.resize(s_numColours);
}

void PaletteViewCtrl::SetPalette(const Palette& palette)
{
	for (int i = 0; i < s_numColours; i++)
	{
		if (palette.IsColourUsed(i))
			m_brushes[i] = wxBrush(wxColour(palette.GetColour(i).GetRed(), palette.GetColour(i).GetGreen(), palette.GetColour(i).GetBlue()));
		else
			m_brushes[i] = wxBrush(wxColour(255,0,0), wxBRUSHSTYLE_CROSSDIAG_HATCH);
	}

	Refresh();
}

void PaletteViewCtrl::OnPaint(wxPaintEvent& evt)
{
	int width;
	int height;
	GetSize(&width, &height);

	int size = width / s_numColours;

	wxPaintDC dc(this);

	for (int i = 0; i < s_numColours; i++)
	{
		dc.SetBrush(m_brushes[i]);
		dc.DrawRectangle(size * i, 0, size, size);
	}
}

BEGIN_EVENT_TABLE(PaletteViewCtrl, wxWindow)
EVT_PAINT(PaletteViewCtrl::OnPaint)
END_EVENT_TABLE()
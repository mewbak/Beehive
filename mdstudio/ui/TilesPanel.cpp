///////////////////////////////////////////////////////
// Beehive: A complete SEGA Mega Drive content tool
//
// (c) 2016 Matt Phillips, Big Evil Corporation
// http://www.bigevilcorporation.co.uk
// mattphillips@mail.com
// @big_evil_corp
//
// Licensed under GPLv3, see http://www.gnu.org/licenses/gpl-3.0.html
///////////////////////////////////////////////////////

#include "TilesPanel.h"
#include "MainWindow.h"

#include <wx/Menu.h>
#include <algorithm>

TilesPanel::TilesPanel(wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style)
	: ViewPanel(parent, winid, pos, size, style)
{
	//Custom zoom/pan handling
	EnableZoom(false);
	EnableScroll(true);
	EnablePan(false);
}


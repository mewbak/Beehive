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

#pragma once

#include "ViewPanel.h"

class StampsPanel : public ViewPanel
{
public:
	StampsPanel(wxWindow* parent, wxWindowID winid = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL | wxNO_BORDER);
	virtual ~StampsPanel();

	virtual void SetupRendering(MainWindow* mainWindow, Project* project, ion::render::Renderer* renderer, wxGLContext* glContext, RenderResources* renderResources);

	void SetStampSetId(StampSetId stampSetId);

	virtual void Refresh(bool eraseBackground = true, const wxRect *rect = NULL);

protected:

	//Mouse click or changed tile callback
	virtual void OnMouseTileEvent(ion::Vector2i mousePos, ion::Vector2i mouseDelta, ion::Vector2i tileDelta, int buttonBits, int x, int y);

	//Render callback
	virtual void OnRender(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float& z, float zOffset);

	//Right-click menu callback
	void OnContextMenuClick(wxCommandEvent& event);

private:

	enum MenuItems
	{
#if !BEEHIVE_PLUGIN_LUMINARY // Stamps come from directory scan only
		eMenuRenameStamp,
		eMenuDeleteStamp,
		eMenuUpdateStamp,
#endif

		eMenuEditCollision,
		eMenuSetBackground,

#if !BEEHIVE_PLUGIN_LUMINARY
		eMenuSetStampLowDrawPrio,
		eMenuSetStampHighDrawPrio,
#endif

#if !BEEHIVE_PLUGIN_LUMINARY
		eMenuUpdatePalette,
		eMenuSubstituteStamp,
		eMenuSortTilesSequentially,
		eMenuOpenInAnimEditor,
#endif
	};

	enum Mode
	{
		eModeSelect,
		eModeSubstitute
	};

	//Calc canvas size
	virtual ion::Vector2i CalcCanvasSize();

	//Paint all stamps using position map to canvas
	virtual void PaintContents();

	//Recalc all stamp positions and canvas size
	void ArrangeStamps(const ion::Vector2& panelSize);

	//Render stamp outlines
	void RenderStampOutlines(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float z);
	void RenderPaletteOverlays(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float z);

	StampSetId GetStampSetId() const;
	StampSet& GetStampSet();

	//Stamp position map
	std::vector< std::pair<StampId, ion::Vector2i> > m_stampPosMap;

	//Current stamp set
	StampSetId m_stampSetId;

	//Current/hover/substitute stamp
	StampId m_selectedStamp;
	StampId m_hoverStamp;
	StampId m_stampToSubstitute;

	//Current selection mode
	Mode m_mode;

	//Current/hover stamp pos
	ion::Vector2i m_selectedStampPos;
	ion::Vector2i m_hoverStampPos;
};
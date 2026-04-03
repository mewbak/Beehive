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

#include "StampsPanel.h"
#include "MainWindow.h"
#include "SpriteAnimEditorDialog.h"
#include "UpdateStampDialog.h"
#include "EditStampCollisionDialog.h"

#include <wx/menu.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <ion/core/utils/STL.h>

StampsPanel::StampsPanel(wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style)
	: ViewPanel(parent, winid, pos, size, style)
{
	m_selectedStamp = InvalidStampId;
	m_hoverStamp = InvalidStampId;
	m_stampToSubstitute = InvalidStampId;
	m_stampSetId = InvalidStampSetId;
	m_mode = eModeSelect;

	//Custom zoom/pan handling
	EnableZoom(false);
	EnableScroll(true);
	EnablePan(false);
}

StampsPanel::~StampsPanel()
{

}

void StampsPanel::SetupRendering(MainWindow* mainWindow, Project* project, ion::render::Renderer* renderer, wxGLContext* glContext, RenderResources* renderResources)
{
	m_selectableUnitSize.x = project->GetPlatformConfig().tileWidth * project->GetPlatformConfig().stampWidth;
	m_selectableUnitSize.y = project->GetPlatformConfig().tileHeight * project->GetPlatformConfig().stampHeight;

	ViewPanel::SetupRendering(mainWindow, project, renderer, glContext, renderResources);
}

void StampsPanel::SetStampSetId(StampSetId stampSetId)
{
	m_stampSetId = stampSetId;
	m_selectedStamp = InvalidStampId;
	m_hoverStamp = InvalidStampId;
	InitPanel();
	Refresh();
}

StampSetId StampsPanel::GetStampSetId() const
{
	if (m_stampSetId == InvalidStampSetId)
	{
		// TODO: Just get from editing map for now, but this should be cleaned up
		return m_project->GetEditingMap().GetStampSetId();
	}
	else
	{
		return m_stampSetId;
	}
}

StampSet& StampsPanel::GetStampSet()
{
	return m_project->GetStampSet(GetStampSetId());
}

void StampsPanel::OnMouseTileEvent(ion::Vector2i mousePos, ion::Vector2i mouseDelta, ion::Vector2i tileDelta, int buttonBits, int x, int y)
{
	StampId selectedStamp = InvalidStampId;
	ion::Vector2i stampTopLeft;

	//If in range, get stamp under mouse cursor
	if(x >= 0 && y >= 0 && x < m_canvasSize.x && y < m_canvasSize.y)
	{
		//TODO: Per-tile stamp map
		//Brute force search for stamp ID
		ion::Vector2i mousePos(x, y);

		for(int i = 0; i < m_stampPosMap.size() && selectedStamp == InvalidStampId; i++)
		{
			StampId stampId = m_stampPosMap[i].first;
			if (stampId != InvalidStampId)
			{
				const Stamp& stamp = GetStampSet().GetStamp(stampId);
				stampTopLeft = m_stampPosMap[i].second;
				const ion::Vector2i& stampBottomRight = stampTopLeft + ion::Vector2i(stamp.GetWidth(), stamp.GetHeight());
				if (mousePos.x >= stampTopLeft.x && mousePos.y >= stampTopLeft.y
					&& mousePos.x < stampBottomRight.x && mousePos.y < stampBottomRight.y)
				{
					selectedStamp = stampId;
				}
			}
		}
	}

	//Set mouse hover stamp
	m_hoverStamp = selectedStamp;
	m_hoverStampPos = stampTopLeft;

	if (buttonBits == 0)
	{
		const int tileWidth = m_project->GetPlatformConfig().tileWidth;
		const int tileHeight = m_project->GetPlatformConfig().tileHeight;

		if (selectedStamp != InvalidStampId)
		{
			const Stamp& stamp = GetStampSet().GetStamp(selectedStamp);
			std::stringstream tipStr;
			tipStr << "Stamp " << stamp.GetName() << std::endl;
			tipStr << "Index 0x" << SSTREAM_HEX4(selectedStamp) << " (" << selectedStamp << ")" << std::endl;
			tipStr << "Size: " << stamp.GetWidth() << ", " << stamp.GetHeight() << std::endl;
			tipStr << "Addr: 0x" << SSTREAM_HEX8(selectedStamp * tileWidth * tileHeight * 2) << std::endl;
			SetToolTip(tipStr.str().c_str());
		}
		else
		{
			UnsetToolTip();
		}
	}

	if((buttonBits & eMouseLeft) && !(m_prevMouseBits & eMouseLeft))
	{
		//Left click, set current stamp
		m_selectedStamp = selectedStamp;
		m_selectedStampPos = stampTopLeft;

		if(m_mode == eModeSelect)
		{
			//Set as current painting stamp
			m_project->SetPaintStamp(selectedStamp);

			//Set stamp paint tool
			m_mainWindow->SetMapTool(eToolPaintStamp);
		}
		else if(m_mode == eModeSubstitute)
		{
			if (m_stampToSubstitute != InvalidStampId && m_selectedStamp != InvalidStampId && m_stampToSubstitute != m_selectedStamp)
			{
				const Stamp& stampA = GetStampSet().GetStamp(m_stampToSubstitute);
				const Stamp& stampB = GetStampSet().GetStamp(m_selectedStamp);

				if(stampA.GetWidth() == stampB.GetWidth() && stampA.GetHeight() == stampB.GetHeight())
				{
					//Substitute stamp
					m_project->SubstituteStamp(GetStampSetId(), m_stampToSubstitute, m_selectedStamp);

					//Delete stamp
					m_project->DeleteStamp(GetStampSetId(), m_stampToSubstitute);

					//Redraw stamps and map panels
					m_project->InvalidateStamps(true);
					m_mainWindow->RefreshPanel(MainWindow::ePanelStamps);
					m_mainWindow->RefreshPanel(MainWindow::ePanelMap);
					m_project->InvalidateStamps(false);
				}
				else
				{
					wxMessageBox("Substitute stamp's width/height does not match the original", "Error", wxOK);
				}
			}

			m_stampToSubstitute = InvalidStampId;
			m_mode = eModeSelect;
		}
	}

	if(buttonBits & eMouseRight)
	{
		//if(m_hoverStamp != InvalidStampId)
		{
			//Right-click menu
			wxMenu contextMenu;

			contextMenu.Append(eMenuEditCollision, wxString("Edit stamp collision/animation/tiles"));
			contextMenu.Append(eMenuSetBackground, wxString("Set as background stamp"));

			contextMenu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&StampsPanel::OnContextMenuClick, NULL, this);
			PopupMenu(&contextMenu);
		}
	}

	//Redraw
	Refresh();
}

void StampsPanel::OnContextMenuClick(wxCommandEvent& event)
{
	if (event.GetId() == eMenuEditCollision)
	{
		//Show collision editor dialog
		if (m_hoverStamp != InvalidStampId)
		{
			Stamp& stamp = GetStampSet().GetStamp(m_hoverStamp);
			DialogEditStamp dialog(*m_mainWindow, GetStampSetId(), stamp, *m_project, *m_renderer, *m_glContext, *m_renderResources);
			dialog.ShowModal();
		}
	}
	else if (event.GetId() == eMenuSetBackground)
	{
		GetStampSet().SetBackgroundStamp(m_hoverStamp);
	}
}

void StampsPanel::OnRender(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float& z, float zOffset)
{
	//Render canvas
	const StampSet& stampSet = GetStampSet();
	RenderCanvas(renderer, cameraInverseMtx, projectionMtx, z, stampSet.GetTilesetId());

	z += zOffset;

	//Render selected stamp
	if(m_selectedStamp != InvalidStampId)
	{
		const Stamp& stamp = stampSet.GetStamp(m_selectedStamp);
		ion::Vector2 size(stamp.GetWidth(), stamp.GetHeight());
		const ion::Colour& colour = m_renderResources->GetColour(RenderResources::eColourSelected);
		RenderBox(m_selectedStampPos, size, colour, renderer, cameraInverseMtx, projectionMtx, z);
	}

	z += zOffset;

	//Render mouse hover stamp
	if(m_hoverStamp != InvalidStampId && m_hoverStamp != m_selectedStamp)
	{
		const Stamp& stamp = GetStampSet().GetStamp(m_hoverStamp);
		ion::Vector2 size(stamp.GetWidth(), stamp.GetHeight());
		const ion::Colour& colour = m_renderResources->GetColour(RenderResources::eColourHighlight);
		RenderBox(m_hoverStampPos, size, colour, renderer, cameraInverseMtx, projectionMtx, z);
	}

	z += zOffset;

	//Render grid
	if(m_project->GetShowGrid())
	{
		RenderGrid(renderer, cameraInverseMtx, projectionMtx, z);
	}

	z += zOffset;

	//Render outlines
	if(m_project->GetShowStampOutlines())
	{
		RenderStampOutlines(renderer, cameraInverseMtx, projectionMtx, z);
	}
}

void StampsPanel::Refresh(bool eraseBackground, const wxRect *rect)
{
	if(!m_mainWindow->IsRefreshLocked())
	{
		m_contentsInvalid |= m_project->StampsAreInvalidated();

		if(m_forceRefresh || m_contentsInvalid)
		{
			m_stampSetId = m_project->GetEditingMap().GetStampSetId();
			m_hoverStamp = InvalidStampId;
			m_selectedStamp = InvalidStampId;
		}

		ViewPanel::Refresh(eraseBackground, rect);
	}
}

ion::Vector2i StampsPanel::CalcCanvasSize()
{
	ArrangeStamps(ion::Vector2(m_panelSize.x, m_panelSize.y));
	return m_canvasSize;
}

void StampsPanel::ArrangeStamps(const ion::Vector2& panelSize)
{
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	//Fit canvas to panel
	m_canvasSize.x = ion::maths::Ceil(panelSize.x / tileWidth);
	m_canvasSize.y = ion::maths::Ceil(panelSize.y / tileHeight);

	//Clear stamp position map
	m_stampPosMap.clear();

	//Sort by size, and find widest stamp
	struct SortedStamp
	{
		int size;
		StampId id;
		const Stamp* stamp;
	};

	std::vector<SortedStamp> stampsSorted;
	stampsSorted.reserve(GetStampSet().GetStampCount());

	for(TStampMap::const_iterator it = GetStampSet().GetStamps().begin(), end = GetStampSet().GetStamps().end(); it != end; ++it)
	{
		const Stamp& stamp = it->second;
		int size = stamp.GetWidth() * stamp.GetHeight();
		stampsSorted.push_back(SortedStamp({ size, it->first, &stamp }));

		//If wider than current canvas width, grow canvas
		m_canvasSize.x = ion::maths::Max(m_canvasSize.x, it->second.GetWidth());
	}

	std::sort(stampsSorted.begin(), stampsSorted.end(), [](const SortedStamp& a, SortedStamp& b) { return a.size < b.size; });

	ion::Vector2i currPos;
	int rowHeight = 1;

	for(int i = 0; i < stampsSorted.size(); i++)
	{
		const Stamp& stamp = *stampsSorted[i].stamp;
		ion::Vector2i stampSize(stamp.GetWidth(), stamp.GetHeight());
		ion::Vector2i stampPos;

		if(currPos.x + stampSize.x > m_canvasSize.x)
		{
			//Can't fit on this line, advance Y by tallest stamp on current row
			currPos.y += rowHeight;

			//Reset column
			currPos.x = stampSize.x;

			//Reset current row height
			rowHeight = stampSize.y;

			//Set stamp pos
			stampPos.x = 0;
			stampPos.y = currPos.y;
		}
		else
		{
			//Set stamp pos
			stampPos = currPos;

			//Next column
			currPos.x += stampSize.x;
		}

		//Record tallest stamp on current row
		rowHeight = ion::maths::Max(rowHeight, stampSize.y);

		//If Y pos + height extends beyond canvas height, grow canvas
		m_canvasSize.y = ion::maths::Max(m_canvasSize.y, currPos.y + rowHeight);

		//Add stamp to position map
		m_stampPosMap.push_back(std::make_pair(stampsSorted[i].id, stampPos));

		//If this is the currently selected stamp, update position
		if(stampsSorted[i].id == m_selectedStamp)
		{
			m_selectedStampPos = stampPos;
		}
	}
}

void StampsPanel::PaintContents()
{
	const StampSet& stampSet = GetStampSet();
	TilesetId tilesetId = stampSet.GetTilesetId();

	for(int i = 0; i < m_stampPosMap.size(); i++)
	{
		if (m_stampPosMap[i].first != InvalidStampId)
		{
			const Stamp& stamp = stampSet.GetStamp(m_stampPosMap[i].first);
			PaintStamp(tilesetId, stamp, m_stampPosMap[i].second.x, m_stampPosMap[i].second.y, 0);
		}
	}
}

void StampsPanel::RenderStampOutlines(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float z)
{
	ion::Matrix4 worldViewProjMtx;
	ion::Matrix4 outlineMtx;
	ion::render::Shader* shader = m_renderResources->GetShader(RenderResources::eShaderFlatColour);
	ion::render::Shader::ParamHndl<ion::Matrix4> worldViewProjParamV = shader->CreateParamHndl<ion::Matrix4>("gWorldViewProjectionMatrix");

	ion::render::Primitive* primitive = m_renderResources->GetPrimitive(RenderResources::ePrimitiveTileLineQuad);
	ion::render::Material* material = m_renderResources->GetMaterial(RenderResources::eMaterialFlatColour);
	const ion::Colour& colour = m_renderResources->GetColour(RenderResources::eColourOutline);

	material->SetDiffuseColour(colour);
	renderer.BindMaterial(*material, outlineMtx, cameraInverseMtx, projectionMtx);

	for(int i = 0; i < m_stampPosMap.size(); i++)
	{
		if(m_stampPosMap[i].first != InvalidStampId)
		{
			const Stamp& stamp = GetStampSet().GetStamp(m_stampPosMap[i].first);
			outlineMtx = m_renderResources->CalcBoxMatrix(m_stampPosMap[i].second, ion::Vector2i(stamp.GetWidth(), stamp.GetHeight()), m_canvasSize, z);
			worldViewProjMtx = outlineMtx * cameraInverseMtx * projectionMtx;
			worldViewProjParamV.SetValue(worldViewProjMtx);

			renderer.DrawVertexBuffer(primitive->GetVertexBuffer());
		}
	}

	renderer.UnbindMaterial(*material);
}

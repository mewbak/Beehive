///////////////////////////////////////////////////////
// Beehive: A complete SEGA Mega Drive content tool
//
// (c) 2017 Matt Phillips, Big Evil Corporation
// http://www.bigevilcorporation.co.uk
// mattphillips@mail.com
// @big_evil_corp
//
// Licensed under GPLv3, see http://www.gnu.org/licenses/gpl-3.0.html
///////////////////////////////////////////////////////

#include "EditStampCollisionDialog.h"
#include "SpriteCanvas.h"
#include "Dialogs.h"
#include "MainWindow.h"
#include "RenderResources.h"

DialogEditStamp::DialogEditStamp(MainWindow& mainWindow, StampSetId stampSetId, StampId stampId, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources)
	: DialogEditStampBase(&mainWindow)
	, m_mainWindow(mainWindow)
	, m_stampSetId(stampSetId)
	, m_stampId(stampId)
	, m_project(project)
	, m_renderer(renderer)
	, m_renderResources(renderResources)
	, m_glContext(glContext)
{
	m_canvas->SetProject(&project);
	m_canvas->SetupRendering(&renderer, &glContext, &renderResources);

	Draw();
	SetPrevNextButtonState();
}

DialogEditStamp::~DialogEditStamp()
{
	m_mainWindow.RefreshPanel(MainWindow::Panel::ePanelMap);
	m_mainWindow.RefreshPanel(MainWindow::Panel::ePanelStamps);
}

void DialogEditStamp::OnToolPrevStamp(wxCommandEvent& event)
{
	StampSet& stampset = m_project.GetStampSet(m_stampSetId);
	auto& stamps = stampset.GetStamps();
	auto it = stamps.find(m_stampId);

	if(it != stamps.begin())
	{
		--it;
		m_stampId = it->first;
		m_canvas->SetStamp(m_stampSetId, m_stampId, ion::Vector2i());
	}

	SetPrevNextButtonState();
}

void DialogEditStamp::OnToolNextStamp(wxCommandEvent& event)
{
	StampSet& stampset = m_project.GetStampSet(m_stampSetId);
	auto& stamps = stampset.GetStamps();
	auto it = stamps.find(m_stampId);
	++it;

	if (it != stamps.end())
	{
		m_stampId = it->first;
		m_canvas->SetStamp(m_stampSetId, m_stampId, ion::Vector2i());
	}

	SetPrevNextButtonState();
}

void DialogEditStamp::SetPrevNextButtonState()
{
	StampSet& stampset = m_project.GetStampSet(m_stampSetId);
	auto& stamps = stampset.GetStamps();

	if (stamps.size() <= 1)
	{
		m_toolBar->EnableTool(m_toolPrevStamp->GetId(), false);
		m_toolBar->EnableTool(m_toolNextStamp->GetId(), false);
	}
	else
	{
		auto it = stamps.find(m_stampId);
		auto next = it;
		next++;

		if (it == stamps.begin())
		{
			m_toolBar->EnableTool(m_toolPrevStamp->GetId(), false);
			m_toolBar->EnableTool(m_toolNextStamp->GetId(), true);
		}
		else if (next == stamps.end())
		{
			m_toolBar->EnableTool(m_toolPrevStamp->GetId(), true);
			m_toolBar->EnableTool(m_toolNextStamp->GetId(), false);
		}
		else
		{
			m_toolBar->EnableTool(m_toolPrevStamp->GetId(), true);
			m_toolBar->EnableTool(m_toolNextStamp->GetId(), true);
		}
	}

	Refresh();
}

void DialogEditStamp::OnToolAddBezier(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolDrawTerrainBezier);
}

void DialogEditStamp::OnToolEditBezier(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolSelectTerrainBezier);
}

void DialogEditStamp::OnToolDeleteBezier(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolDeleteTerrainBezier);
}

void DialogEditStamp::OnToolPaintSolid(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolPaintCollisionSolid);
}

void DialogEditStamp::OnToolGenerateTerrain(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolNone);

	if (wxMessageBox("This will clear all terrain tiles and regenerate for all stamps, are you sure?", "Generate Terrain", wxOK | wxCANCEL) == wxOK)
	{
		if (!m_project.GenerateTerrainFromBeziers())
		{
			wxMessageBox("Error generating terrain - out of tile space", "Error", wxOK, this);
		}

		//Refresh all to redraw terrain tiles
		m_mainWindow.RefreshAll();

		//Invalid terrain tiles and refresh stamp canvas
		m_project.InvalidateTerrainTiles(true);
		m_canvas->Refresh();
		m_project.InvalidateTerrainTiles(false);
	}
}

void DialogEditStamp::OnToolSelectTiles(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolStampOverlay);
}

void DialogEditStamp::OnToolPlaceAnim(wxCommandEvent& event)
{
	m_canvas->SetTool(eToolStampAnimation);
}

void DialogEditStamp::Draw()
{
	const int tileWidth = m_project.GetPlatformConfig().tileWidth;
	const int tileHeight = m_project.GetPlatformConfig().tileHeight;
	const Stamp& stamp = m_project.GetStampSet(m_stampSetId).GetStamp(m_stampId);

	m_canvas->CreateGrid(stamp.GetWidth() * tileWidth, stamp.GetHeight() * tileHeight, stamp.GetWidth(), stamp.GetHeight());
	m_canvas->SetGridColour(ion::Colour(1.0f, 1.0f, 1.0f, 1.0f));
	m_canvas->SetDrawGrid(true);
	m_canvas->SetStamp(m_stampSetId, m_stampId, ion::Vector2i());
	m_canvas->Refresh();
}

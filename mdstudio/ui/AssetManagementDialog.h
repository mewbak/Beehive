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

#include "UIBase.h"
#include "RenderResources.h"

#include <ion/renderer/Renderer.h>

#include <wx/glcanvas.h>

class DialogAssetManagement : public DialogAssetsBase
{
public:
	DialogAssetManagement(MainWindow& mainWindow, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources);

	virtual void OnTabChanged(wxNotebookEvent& event);

	// Palettes tab
	virtual void OnListPalette(wxCommandEvent& event);
	virtual void OnBtnImportPalette(wxCommandEvent& event);
	virtual void OnBtnExportPalette(wxCommandEvent& event);
	virtual void OnBtnRenamePalette(wxCommandEvent& event);
	virtual void OnBtnDeletePalette(wxCommandEvent& event);
	virtual void OnListSlot0(wxCommandEvent& event);
	virtual void OnListSlot1(wxCommandEvent& event);
	virtual void OnListSlot2(wxCommandEvent& event);
	virtual void OnListSlot3(wxCommandEvent& event);

	// Tilesets tab
	virtual void OnListTilesets(wxCommandEvent& event);
	virtual void OnListTilesetPalette(wxCommandEvent& event);

	// Stamp sets tab
	virtual void OnListStampSet(wxCommandEvent& event);

private:
	void PopulatePalettes();
	void PopulateTilesets();
	void PopulateStampSets();

	void SelectPalette(int index);
	void SelectTileset(int index);
	void SelectStampSet(int index);

	void AssignPalette(int index, int slotIndex);

	int GetPaletteUsage(PaletteId paletteId, std::vector<TilesetId>& tilesets, std::vector<std::pair<ActorId,SpriteSheetId>>& spriteSheets, std::vector<MapId>& maps) const;
	int GetTilesetUsage(TilesetId tilesetId, std::vector<StampSetId>& stampSets) const;
	int GetStampSetUsage(StampSetId stampSetId, std::vector<MapId>& maps) const;

	Project& m_project;
	MainWindow& m_mainWindow;

	std::vector<std::pair<PaletteId, Palette>> m_populatedPalettes;
	std::vector<TilesetId> m_populatedTilesets;
	std::vector<StampSetId> m_populatedStampSets;
};
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
	enum class Tab
	{
		Palettes,
		Tilesets,
		Stampsets,
		Maps
	};

	DialogAssetManagement(MainWindow& mainWindow, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources, Tab tab = Tab::Palettes);

	virtual void OnTabChanged(wxNotebookEvent& event);

	// Palettes tab
	virtual void OnListPalette(wxCommandEvent& event);
	virtual void OnBtnImportPalette(wxCommandEvent& event);
	virtual void OnBtnExportPalette(wxCommandEvent& event);
	virtual void OnBtnRenamePalette(wxCommandEvent& event);
	virtual void OnBtnDeletePalette(wxCommandEvent& event);

	// Tilesets tab
	virtual void OnListTileset(wxCommandEvent& event);
	virtual void OnListTilesetPalette(wxCommandEvent& event);
	virtual void OnBtnNewTileset(wxCommandEvent& event);
	virtual void OnBtnDeleteTileset(wxCommandEvent& event);
	virtual void OnBtnRenameTileset(wxCommandEvent& event);
	virtual void OnBtnScanTileset(wxCommandEvent& event);
	virtual void OnBtnExportTileset(wxCommandEvent& event);
	virtual void OnBtnCleanupTileset(wxCommandEvent& event);
	virtual void OnBrowseTilesImg(wxFileDirPickerEvent& event);

	// Stamp sets tab
	virtual void OnListStampSet(wxCommandEvent& event);
	virtual void OnBtnNewStampSet(wxCommandEvent& event);
	virtual void OnBtnDeleteStampSet(wxCommandEvent& event);
	virtual void OnBtnScanStampSet(wxCommandEvent& event);
	virtual void OnBtnExportStampSet(wxCommandEvent& event);
	virtual void OnBtnRenameStampSet(wxCommandEvent& event);
	virtual void OnBtnCleanupStampSet(wxCommandEvent& event);
	virtual void OnStampPaletteSlot(wxCommandEvent& event);
	virtual void OnBrowseStampsImg(wxFileDirPickerEvent& event);

	// Maps tab
	virtual void OnListMap(wxCommandEvent& event);
	virtual void OnListSlot0(wxCommandEvent& event);
	virtual void OnListSlot1(wxCommandEvent& event);
	virtual void OnListSlot2(wxCommandEvent& event);
	virtual void OnListSlot3(wxCommandEvent& event);

private:
	void PopulatePalettes();
	void PopulateTilesets();
	void PopulateStampSets();
	void PopulateMaps();

	void SelectPalette(int index);
	void SelectTileset(int index);
	void SelectStampSet(int index);
	void SelectMap(int index);

	void SelectPaletteById(PaletteId paletteId);
	void SelectTilesetById(TilesetId tilesetId);
	void SelectStampSetById(StampSetId stampSetId);
	void SelectMapById(MapId mapId);

	void MergeTileset(const std::string filename, TilesetId tilesetId);
	void MergeStampset(const std::string filename, StampSetId stampSetId);

	void AssignPalette(int index, int slotIndex);

	int GetPaletteUsage(PaletteId paletteId, std::vector<TilesetId>& tilesets, std::vector<std::pair<ActorId,SpriteSheetId>>& spriteSheets, std::vector<MapId>& maps) const;
	int GetTilesetUsage(TilesetId tilesetId, std::vector<StampSetId>& stampSets) const;
	int GetStampSetUsage(StampSetId stampSetId, std::vector<MapId>& maps) const;

	void ShowImportError(Project::ImportResult result, const std::string& filename) const;

	Project& m_project;
	MainWindow& m_mainWindow;
	ion::render::Renderer& m_renderer;
	wxGLContext& m_glContext;
	RenderResources& m_renderResources;

	std::vector<PaletteId> m_populatedPalettes;
	std::vector<TilesetId> m_populatedTilesets;
	std::vector<StampSetId> m_populatedStampSets;
	std::vector<MapId> m_populatedMaps;
	std::vector<PaletteId> m_populatedMapPalettes;
};
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

class DialogNewProject : public DialogNewProjectBase
{
public:
	DialogNewProject(wxWindow* parent);
	virtual void OnChoicePreset(wxCommandEvent& event);
	virtual void OnBtnOk(wxCommandEvent& event) { EndModal(wxID_OK); }
	virtual void OnBtnCancel(wxCommandEvent& event) { EndModal(wxID_CANCEL); }
	void PopulatePreset(int index);
};

class DialogMapSize : public DialogMapSizeBase
{
public:
	DialogMapSize(wxWindow* parent) : DialogMapSizeBase(parent) {}
	virtual void OnBtnOk(wxCommandEvent& event) { EndModal(wxID_OK); }
	virtual void OnBtnCancel(wxCommandEvent& event) { EndModal(wxID_CANCEL); }
};

class DialogTerrainGen : public DialogTerrainGenBase
{
public:
	DialogTerrainGen(wxWindow* parent) : DialogTerrainGenBase(parent) {}
	virtual void OnBtnOk(wxCommandEvent& event) { EndModal(wxID_OK); }
	virtual void OnBtnCancel(wxCommandEvent& event) { EndModal(wxID_CANCEL); }
};

class ImportDialog : public ImportDialogBase
{
public:
	ImportDialog(wxWindow* parent);

	wxArrayString m_paths;

protected:
	virtual void OnBtnBrowse(wxCommandEvent& event);
};

class ImportStampsDialog : public ImportStampsDialogBase
{
public:
	ImportStampsDialog(wxWindow* parent);

	wxArrayString m_paths;

protected:
	virtual void OnBtnBrowse(wxCommandEvent& event);
	virtual void OnRadioImportFile(wxCommandEvent& event);
	virtual void OnRadioImportDir(wxCommandEvent& event);
};

class ImportDialogSpriteSheet : public ImportDialogSpriteSheetBase
{
public:
	ImportDialogSpriteSheet(wxWindow* parent, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources);
	virtual void OnFileOpened(wxFileDirPickerEvent& event);
	virtual void OnGridColourChanged(wxColourPickerEvent& event);
	virtual void OnSpinWidthCells(wxSpinEvent& event);
	virtual void OnSpinHeightCells(wxSpinEvent& event);
	virtual void OnSpinCellCount(wxSpinEvent& event);

private:
	Project& m_project;
	RenderResources& m_renderResources;
};

class MatchPaletteDialog : public DialogMatchPalette
{
public:
	MatchPaletteDialog(wxWindow* parent, Project& project, const Palette& paletteToMatch, PaletteId currentPaletteId, const std::string& defaultName);

	virtual void OnChoicePalette(wxCommandEvent& event);
	virtual void OnBtnMatch(wxCommandEvent& event);
	virtual void OnBtnNew(wxCommandEvent& event);

	void Populate(PaletteId selected);

	Project& m_project;
	Palette m_paletteToMatch;
	PaletteId m_selectedPaletteId;
	std::vector<std::pair<PaletteId, Palette>> m_matches;
	const std::string m_defaultName;
};

class MergePaletteDialog : public  DialogMergePaletteBase
{
public:
	MergePaletteDialog(wxWindow* parent, const Palette& original, const Palette& imported);

	virtual void OnRadioOriginal(wxCommandEvent& event);
	virtual void OnRadioImported(wxCommandEvent& event);

	const Palette& GetMergedPalette() { return m_merged; }

private:
	void MergeUp();
	void MergeDown();

	Palette m_original;
	Palette m_imported;
	Palette m_merged;

};

class MatchTilesetDialog : public MatchTilesetDialogBase
{
public:
	MatchTilesetDialog(MainWindow& mainWindow, Project& project, TilesetId tilesetToMatch, const std::vector<std::pair<TilesetId, int>>& matches, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources);

	virtual void OnChoiceTileset(wxCommandEvent& event);
	virtual void OnBtnMatch(wxCommandEvent& event);
	virtual void OnBtnNew(wxCommandEvent& event);

	TilesetId GetSelectedTileset() const;
	const std::string& GetNewName();

private:
	void PopulateView(TilesPanel* view, TilesetId tilesetId);
	std::string m_newName;
	const std::vector<std::pair<TilesetId, int>>& m_matches;
};

class NewMapDialog : public DialogNewMapBase
{
public:
	NewMapDialog(MainWindow& mainWindow, wxWindow* parent, Project& project);

	virtual void OnBtnNewStampSet(wxCommandEvent& event);

	std::string GetMapName() const;
	ion::Vector2i GetMapSize() const;
	StampSetId GetMapStampSetId() const;

private:
	MainWindow& m_mainWindow;
	Project& m_project;
	std::vector<StampSetId> m_populatedStampSets;
	StampSetId m_selectedStampSet;
};
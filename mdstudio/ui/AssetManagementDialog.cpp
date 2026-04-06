///////////////////////////////////////////////////////
// Beehive: A complete SEGA Mega Drive content tool
//
// (c) 2016 Matt Phillips, Big Evil Corporation
// http://www.bigevilcorporation.co.uk
// mattphillips@mail.com
// @big_evil_corp
//
// Licensed under GPLv3, see http://www.gnu.org/licenses/gpl-3.0.html
/////////////////////////////////////////////////////

#include "AssetManagementDialog.h"
#include "Dialogs.h"
#include "StampsPanel.h"
#include "TilesPanel.h"
#include "MainWindow.h"
#include "GameObjectUtils.h"

#include <ion/core/utils/STL.h>

DialogAssetManagement::DialogAssetManagement(MainWindow& mainWindow, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources, Tab tab)
	: DialogAssetsBase((wxWindow*)&mainWindow)
	, m_project(project)
	, m_mainWindow(mainWindow)
	, m_renderer(renderer)
	, m_glContext(glContext)
	, m_renderResources(renderResources)
{
	// Setup canvases
	m_canvasTiles->SetupRendering(&mainWindow, &project, &renderer, &glContext, &renderResources);
	m_canvasStamps->SetupRendering(&mainWindow, &project, &renderer, &glContext, &renderResources);

	// Populate all tabs
	PopulatePalettes();
	PopulateTilesets();
	PopulateStampSets();
	PopulateMaps();

	SelectPalette(0);
	SelectTileset(0);
	SelectStampSet(0);
	SelectMap(0);

	m_tabs->ChangeSelection((int)tab);

	Refresh();
}

void DialogAssetManagement::PopulatePalettes()
{
	int index = m_listPalettes->GetSelection();

	m_populatedPalettes.clear();
	m_listPalettes->Clear();
	m_choiceTilesetPalette->Clear();
	wxChoice* lists[4] = { m_choiceSlot0, m_choiceSlot1, m_choiceSlot2, m_choiceSlot3 };

	for (int i = 0; i < 4; i++)
	{
		lists[i]->Clear();
	}

	const auto& palettes = m_project.GetPalettes();

	for (const auto& palette : palettes)
	{
		m_listPalettes->Append(palette.second.GetName());
		m_choiceTilesetPalette->Append(palette.second.GetName());

		for (int i = 0; i < 4; i++)
		{
			lists[i]->Append(palette.second.GetName());
		}

		m_populatedPalettes.push_back(palette.first);
	}

	if (index >= 0 && index < m_populatedPalettes.size())
	{
		m_listPalettes->SetSelection(index);
	}
}

void DialogAssetManagement::PopulateTilesets()
{
	int index = m_listTilesets->GetSelection();

	m_populatedTilesets.clear();
	m_listTilesets->Clear();

	const auto& tilesets = m_project.GetTilesets();

	for (const auto& tileset : tilesets)
	{
		m_listTilesets->Append(tileset.second.GetName());
		m_populatedTilesets.push_back(tileset.first);
	}

	if (index >= 0 && index < m_populatedTilesets.size())
	{
		m_listTilesets->SetSelection(index);
	}
}

void DialogAssetManagement::PopulateStampSets()
{
	int index = m_listStampSets->GetSelection();

	m_populatedStampSets.clear();
	m_listStampSets->Clear();

	const auto& stampSets = m_project.GetStampSets();

	for (const auto& stampSet : stampSets)
	{
		m_listStampSets->Append(stampSet.second.GetName());
		m_populatedStampSets.push_back(stampSet.first);
	}

	if (index >= 0 && index < m_populatedStampSets.size())
	{
		m_listStampSets->SetSelection(index);
	}
}

void DialogAssetManagement::PopulateMaps()
{
	int index = m_listMaps->GetSelection();

	m_populatedMaps.clear();
	m_listMaps->Clear();
	m_choiceBgMap->Clear();

	const auto& maps = m_project.GetMaps();

	for (const auto& map : maps)
	{
		m_listMaps->Append(map.second.GetName());
		m_choiceBgMap->Append(map.second.GetName());
		m_populatedMaps.push_back(map.first);
	}

	m_choiceBgMap->Append("[None]");

	if (index >= 0 && index < m_populatedMaps.size())
	{
		m_listMaps->SetSelection(index);
	}
}

void DialogAssetManagement::SelectPalette(int index)
{
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		m_listPalettes->SetSelection(index);

		PaletteId paletteId = m_populatedPalettes[index];
		const Palette& palette = m_project.GetPalette(paletteId);
		m_paletteViewSelected->SetPalette(palette);
		m_txtPaletteName->SetLabelText(palette.GetName());
		m_txtPaletteId->SetLabelText(std::to_string(paletteId));

		m_txtPaletteActiveSlot->SetLabelText("Unassigned");

		for (int i = 0; i < m_project.GetEditingMap().GetNumPaletteSlots(); i++)
		{
			if (m_project.GetEditingMap().GetPaletteFromSlot(i) == paletteId)
			{
				m_txtPaletteActiveSlot->SetLabelText(std::to_string(i));
				break;
			}
		}

		std::vector<TilesetId> tilesets;
		std::vector<ActorId> actors;
		std::vector<MapId> maps;
		m_txtPaletteUsageCount->SetLabelText(std::to_string(GetPaletteUsage(paletteId, tilesets, actors, maps)));

		Refresh();
	}
}

void DialogAssetManagement::SelectTileset(int index)
{
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		m_listTilesets->SetSelection(index);

		TilesetId tilesetId = m_populatedTilesets[index];
		const Tileset& tileset = m_project.GetTileset(tilesetId);

		PaletteId paletteId = tileset.GetDefaultPaletteId();
		const Palette palette = m_project.GetPalette(paletteId);

		m_canvasTiles->SetTilesetId(tilesetId);
		m_paletteViewTiles->SetPalette(palette);
		m_choiceTilesetPalette->SetSelection(ion::utils::stl::IndexOf(m_populatedPalettes, paletteId));
		m_filePickerTilesImg->SetPath(m_project.m_settings.GetAbsolutePath(tilesetId));

		m_txtTilesetName->SetLabelText(tileset.GetName());
		m_txtTilesetId->SetLabelText(std::to_string(tilesetId));
		m_txtTilesetPalette->SetLabelText(palette.GetName());
		m_txtTilesetCount->SetLabelText(std::to_string(tileset.GetCount()));

		std::vector<StampSetId> stampSets;
		m_txtTilesetUsageCount->SetLabelText(std::to_string(GetTilesetUsage(tilesetId, stampSets)));

		if (tileset.GetOwningStampSet() == InvalidStampSetId)
		{
			m_txtTilesetOwner->SetLabelText("None");
			m_btnDeleteTileset->Enable();
			m_btnScanTileset->Enable();
			m_btnCleanupTileset->Enable();
			m_filePickerTilesImg->Enable();
		}
		else
		{
			const StampSet& owner = m_project.GetStampSet(tileset.GetOwningStampSet());
			m_txtTilesetOwner->SetLabelText(owner.GetName());
			m_btnDeleteTileset->Disable();
			m_btnScanTileset->Disable();
			m_btnCleanupTileset->Disable();
			m_filePickerTilesImg->Disable();
		}

		Refresh();
	}
}

void DialogAssetManagement::SelectStampSet(int index)
{
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		m_listStampSets->SetSelection(index);

		StampSetId stampSetId = m_populatedStampSets[index];
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);

		TilesetId tilesetId = stampSet.GetTilesetId();
		const Tileset& tileset = m_project.GetTileset(tilesetId);

		PaletteId paletteId = tileset.GetDefaultPaletteId();
		const Palette& palette = m_project.GetPalette(paletteId);

		m_canvasStamps->SetStampSetId(stampSetId);
		m_paletteViewStamps->SetPalette(palette);

		m_txtStampSetName->SetLabelText(stampSet.GetName());
		m_txtStampSetId->SetLabelText(std::to_string(stampSetId));
		m_txtStampCount->SetLabelText(std::to_string(stampSet.GetStampCount()));
		m_txtStampSetPalette->SetLabelText(palette.GetName());
		m_txtStampSetTileset->SetLabelText(tileset.GetName());

		std::vector<MapId> maps;
		m_txtStampSetUsageCount->SetLabelText(std::to_string(GetStampSetUsage(stampSetId, maps)));

		m_filePickerStampsImg->SetPath(m_project.m_settings.GetAbsolutePath(stampSetId));

		Refresh();
	}
}

void DialogAssetManagement::SelectMap(int index)
{
	if (index >= 0 && index < m_populatedMaps.size())
	{
		m_listMaps->SetSelection(index);

		MapId mapId = m_populatedMaps[index];
		const Map& map = m_project.GetMap(mapId);
		StampSetId stampSetId = map.GetStampSetId();
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);
		TilesetId tilesetId = stampSet.GetTilesetId();
		const Tileset& tileset = m_project.GetTileset(tilesetId);

		bool isBackgroundMap = false;

		for (const auto& it : m_project.GetMaps())
		{
			if (it.second.GetBackgroundMapId() == mapId)
			{
				isBackgroundMap = true;
				break;
			}
		}

		if (isBackgroundMap)
		{
			m_choiceBgMap->Disable();
			m_btnAutoAssignPalettes->Disable();
		}
		else
		{
			m_choiceBgMap->Enable();
			m_btnAutoAssignPalettes->Enable();
		}

		PaletteViewCtrl* views[4] = { m_paletteViewSlot0, m_paletteViewSlot1, m_paletteViewSlot2, m_paletteViewSlot3 };
		wxChoice* lists[4] = { m_choiceSlot0, m_choiceSlot1, m_choiceSlot2, m_choiceSlot3 };

		for (int i = 0; i < 4; i++)
		{
			if (isBackgroundMap)
			{
				lists[i]->Disable();
			}
			else
			{
				lists[i]->Enable();

				PaletteId id = map.GetPaletteFromSlot(i);
				if (id == InvalidPaletteId)
				{
					views[i]->SetPalette(Palette());
					lists[i]->SetSelection(-1);
				}
				else
				{
					const Palette& palette = m_project.GetPalette(id);
					views[i]->SetPalette(palette);

					int index = ion::utils::stl::IndexOf(m_populatedPalettes, id);
					lists[i]->SetSelection(index);
				}
			}
		}

		MapId bgMapId = map.GetBackgroundMapId();
		if (bgMapId == InvalidMapId)
			m_choiceBgMap->SetSelection(m_populatedMaps.size());
		else
			m_choiceBgMap->SetSelection(ion::utils::stl::IndexOf(m_populatedMaps, bgMapId));

		m_txtMapName->SetLabelText(map.GetName());
		m_txtMapId->SetLabelText(std::to_string(mapId));
		m_txtMapStampSet->SetLabelText(stampSet.GetName());
		m_txtMapTileset->SetLabelText(tileset.GetName());
		m_txtMapType->SetLabelText(isBackgroundMap ? "Background" : "Foreground");
	}
}

void DialogAssetManagement::SelectPaletteById(PaletteId paletteId)
{
	SelectPalette(ion::utils::stl::IndexOf(m_populatedPalettes, paletteId));
}

void DialogAssetManagement::SelectTilesetById(TilesetId tilesetId)
{
	SelectTileset(ion::utils::stl::IndexOf(m_populatedTilesets, tilesetId));
}

void DialogAssetManagement::SelectStampSetById(StampSetId stampSetId)
{
	SelectStampSet(ion::utils::stl::IndexOf(m_populatedStampSets, stampSetId));
}

void DialogAssetManagement::SelectMapById(MapId mapId)
{
	SelectMap(ion::utils::stl::IndexOf(m_populatedMaps, mapId));
}

void DialogAssetManagement::AssignPalette(int index, int slotIndex)
{
	int mapIdx = m_listMaps->GetSelection();
	if (mapIdx >= 0 && mapIdx < m_populatedMaps.size())
	{
		MapId mapId = m_populatedMaps[mapIdx];
		PaletteId paletteId = m_populatedPalettes[index];

		Map& map = m_project.GetMap(mapId);
		const Palette& palette = m_project.GetPalette(paletteId);

		map.AssignPaletteToSlot(paletteId, slotIndex);

		PaletteViewCtrl* views[4] = { m_paletteViewSlot0, m_paletteViewSlot1, m_paletteViewSlot2, m_paletteViewSlot3 };
		views[slotIndex]->SetPalette(palette);
		Refresh();
	}
}

void DialogAssetManagement::OnTabChanged(wxNotebookEvent& event)
{

}

void DialogAssetManagement::OnListPalette(wxCommandEvent& event)
{
	SelectPalette(m_listPalettes->GetSelection());
}

void DialogAssetManagement::OnBtnImportPalette(wxCommandEvent& event)
{
	//Open BMP
	wxFileDialog fileDlg(this, _("Open image file"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		//Import palette
		std::string filename = fileDlg.GetPath().c_str().AsChar();
		std::string name = ion::string::GetFilename(filename);
		Palette palette;
		Project::ImportResult result = m_project.ImportPaletteFromImage(filename, palette);

		if (result != Project::ImportResult::Success)
		{
			ShowImportError(result, filename, m_project);
			return;
		}

		//Match with existing or create new
		MatchPaletteDialog matchDlg(this, m_project, palette, InvalidPaletteId, name);
		matchDlg.ShowModal();

		//Remember source path
		m_project.m_settings.SetAbsolutePath(matchDlg.m_selectedPaletteId, filename);

		PopulatePalettes();
		SelectPalette(m_populatedPalettes.size() - 1);
	}
}

void DialogAssetManagement::OnBtnExportPalette(wxCommandEvent& event)
{
	int index = m_listPalettes->GetSelection();
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		PaletteId paletteId = m_populatedPalettes[index];
		const Palette& palette = m_project.GetPalette(paletteId);

		std::string defaultFilename = palette.GetName() + ".bmp";
		wxFileDialog fileDlg(this, _("Save BMP file"), "", defaultFilename, "BMP files (*.bmp)|*.bmp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fileDlg.ShowModal() == wxID_OK)
		{
			ion::ImageFormatBMP writer(ion::ImageFormat::BitFormat::eIndexed16Colour);

			for (int i = 0; i < Palette::coloursPerPalette; i++)
			{
				Colour colour(0, 0, 0);
				if (palette.IsColourUsed(i))
					colour = palette.GetColour(i);
				writer.SetPaletteEntry(i, ion::ImageFormat::Colour(colour.GetRed(), colour.GetGreen(), colour.GetBlue()));
			}

			const int blockWidth = 32;
			const int blockHeight = 32;

			writer.SetDimensions(blockWidth * Palette::coloursPerPalette, blockHeight);

			for (int x = 0; x < blockWidth * Palette::coloursPerPalette; x++)
			{
				for (int y = 0; y < blockHeight; y++)
				{
					u8 index = x / blockWidth;
					writer.SetColourIndex(x, y, index);
				}
			}

			std::string filename = fileDlg.GetPath().c_str().AsChar();
			if (!writer.Write(filename))
				wxMessageBox("Error exporting palette to image '" + filename + "'", "Error");
		}
	}
}

void DialogAssetManagement::OnBtnRenamePalette(wxCommandEvent& event)
{
	int index = m_listPalettes->GetSelection();
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		PaletteId paletteId = m_populatedPalettes[index];
		Palette& palette = m_project.GetPalette(paletteId);

		wxTextEntryDialog dlg(this, "Rename palette", "Rename palette", palette.GetName());
		if (dlg.ShowModal() == wxID_OK)
		{
			palette.SetName(dlg.GetValue().c_str().AsChar());
			PopulatePalettes();
			SelectPaletteById(paletteId);
		}
	}
}

void DialogAssetManagement::OnBtnDeletePalette(wxCommandEvent& event)
{
	int index = m_listPalettes->GetSelection();
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		PaletteId paletteId = m_populatedPalettes[index];
		const Palette& palette = m_project.GetPalette(paletteId);

		std::vector<TilesetId> tilesets;
		std::vector<ActorId> actors;
		std::vector<MapId> maps;
		int usageCount = GetPaletteUsage(paletteId, tilesets, actors, maps);

		if (usageCount > 0)
		{
			std::string msg = "Cannot delete in-use palette " + palette.GetName() + "\n";
			
			if (tilesets.size() > 0)
			{
				msg += "\nUsed by tileset(s):\n\n";

				for (TilesetId tilesetId : tilesets)
				{
					const Tileset& tileset = m_project.GetTileset(tilesetId);
					msg += " " + tileset.GetName() + "\n";
				}
			}

			if (actors.size() > 0)
			{
				msg += "\nUsed by sprite actors(s):\n\n";

				for (const auto& it : actors)
				{
					const Actor* actor = m_project.GetActor(it);
					msg += " " + actor->GetName() + "\n";
				}
			}

			if (maps.size() > 0)
			{
				msg += "\nAssigned to slot(s) in map(s):\n\n";

				for (const auto& mapId : maps)
				{
					const Map& map = m_project.GetMap(mapId);
					msg += " " + map.GetName() + "\n";
				}
			}

			wxMessageBox(msg, "Error");
			return;
		}

		m_project.DeletePalette(paletteId);
		PopulatePalettes();
		SelectPalette(0);
	}
}

void DialogAssetManagement::OnListTileset(wxCommandEvent& event)
{
	SelectTileset(m_listTilesets->GetSelection());
}

void DialogAssetManagement::OnListTilesetPalette(wxCommandEvent& event)
{
	int paletteIdx = m_choiceTilesetPalette->GetSelection();
	int tilesetIdx = m_listTilesets->GetSelection();
	if (paletteIdx >= 0 && paletteIdx < m_populatedPalettes.size()
		&& tilesetIdx >= 0 && tilesetIdx < m_populatedTilesets.size())
	{
		PaletteId paletteId = m_populatedPalettes[paletteIdx];
		TilesetId tilesetId = m_populatedTilesets[tilesetIdx];
		const Palette& palette = m_project.GetPalette(paletteId);
		Tileset& tileset = m_project.GetTileset(tilesetId);

		if (!m_project.CheckCompatibilityPaletteTileset(paletteId, tilesetId))
		{
			std::string msg = "Palette " + palette.GetName() + " is incompatible with tileset " + tileset.GetName();
			wxMessageBox(msg, "Error");
			m_choiceTilesetPalette->SetSelection(ion::utils::stl::IndexOf(m_populatedPalettes, tileset.GetDefaultPaletteId()));
			return;
		}

		m_paletteViewTiles->SetPalette(palette);
		m_txtTilesetPalette->SetLabelText(palette.GetName());
		tileset.SetDefaultPaletteId(paletteId);

		m_mainWindow.RefreshTileset();
		m_mainWindow.RefreshAll();

		SelectTileset(m_listTilesets->GetSelection());
		SelectStampSet(m_listStampSets->GetSelection());
	}
}

void DialogAssetManagement::OnBtnNewTileset(wxCommandEvent& event)
{
	//Open BMP
	wxFileDialog fileDlg(this, _("Open image file"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		//Import tileset
		std::string filename = fileDlg.GetPath().c_str().AsChar();
		std::string name = ion::string::GetFilename(filename);
		Palette palette;
		Tileset tileset;
		Project::ImportResult result = m_project.ImportTilesetFromImage(filename, palette, tileset);

		if (result != Project::ImportResult::Success)
		{
			ShowImportError(result, filename, m_project);
			return;
		}

		//Match palette with existing or create new
		MatchPaletteDialog matchDlg(this, m_project, palette, InvalidPaletteId, name);
		matchDlg.ShowModal();
		PaletteId paletteId = matchDlg.m_selectedPaletteId;

		if (paletteId == InvalidPaletteId)
		{
			wxMessageBox("Cannot import tileset, no matching palette selected", "Error");
			return;
		}

		tileset.SetDefaultPaletteId(paletteId);

		//Add new tilset
		TilesetId tilesetId = m_project.AddTileset(tileset);

		//Remember source path
		m_project.m_settings.SetAbsolutePath(tilesetId, filename);

		//Recreate render resources
		m_mainWindow.RefreshTileset();
		m_mainWindow.RefreshAll();

		// Refresh
		PopulatePalettes();
		PopulateTilesets();
		SelectPaletteById(paletteId);
		SelectTilesetById(tilesetId);
	}
}

void DialogAssetManagement::OnBtnDeleteTileset(wxCommandEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		const Tileset& tileset = m_project.GetTileset(tilesetId);

		std::vector<StampSetId> stampsets;
		int usageCount = GetTilesetUsage(tilesetId, stampsets);

		if (usageCount > 0)
		{
			std::string msg = "Cannot delete in-use tilset " + tileset.GetName() + "\n";

			msg += "\nUsed by stamp sets(s):\n\n";

			for (StampSetId stampSetId : stampsets)
			{
				const StampSet& stampset = m_project.GetStampSet(stampSetId);
				msg += " " + stampset.GetName() + "\n";
			}

			wxMessageBox(msg, "Error");
			return;
		}

		m_project.DeleteTileset(tilesetId);
		PopulateTilesets();
		SelectTileset(0);
	}
}

void DialogAssetManagement::OnBtnRenameTileset(wxCommandEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		Tileset& tileset = m_project.GetTileset(tilesetId);

		wxTextEntryDialog dlg(this, "Rename tileset", "Rename tileset", tileset.GetName());
		if (dlg.ShowModal() == wxID_OK)
		{
			tileset.SetName(dlg.GetValue().c_str().AsChar());
			PopulateTilesets();
			SelectTilesetById(tilesetId);
		}
	}
}

void DialogAssetManagement::OnBtnScanTileset(wxCommandEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		std::string filename = m_project.m_settings.GetAbsolutePath(tilesetId);
		MergeTileset(filename, tilesetId);
	}
}

void DialogAssetManagement::ExportTileset(const std::string filename, TilesetId tilesetId)
{
	const Tileset& tileset = m_project.GetTileset(tilesetId);
	PaletteId paletteId = tileset.GetDefaultPaletteId();
	const Palette& palette = m_project.GetPalette(paletteId);

	ion::ImageFormatBMP writer(ion::ImageFormat::BitFormat::eIndexed16Colour);

	for (int i = 0; i < Palette::coloursPerPalette; i++)
	{
		Colour colour(0, 0, 0);
		if (palette.IsColourUsed(i))
			colour = palette.GetColour(i);
		writer.SetPaletteEntry(i, ion::ImageFormat::Colour(colour.GetRed(), colour.GetGreen(), colour.GetBlue()));
	}

	const int numTiles = tileset.GetCount();
	const int numTilesSq = (int)ion::maths::Ceil(ion::maths::Sqrt(numTiles));
	const int tilesPerRow = numTilesSq;
	const int tileWidth = m_project.GetPlatformConfig().tileWidth;
	const int tileHeight = m_project.GetPlatformConfig().tileHeight;

	writer.SetDimensions(numTilesSq * tileWidth, numTilesSq * tileHeight);

	int i = 0;
	for (const auto& it : tileset.GetTiles())
	{
		const Tile& tile = it.second;
		int tileX = i % numTilesSq;
		int tileY = i / numTilesSq;
		i++;

		for (int pixelX = 0; pixelX < tileWidth; pixelX++)
		{
			for (int pixelY = 0; pixelY < tileHeight; pixelY++)
			{
				int imageX = (tileX * tileWidth) + pixelX;
				int imageY = (tileY * tileHeight) + pixelY;
				writer.SetColourIndex(imageX, imageY, tile.GetPixelColour(pixelX, pixelY));
			}
		}
	}

	if (!writer.Write(filename))
		wxMessageBox("Error exporting tileset to image '" + filename + "'", "Error");
}

void DialogAssetManagement::OnBtnExportTileset(wxCommandEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		const Tileset& tileset = m_project.GetTileset(tilesetId);

		std::string storedFilename = m_project.m_settings.GetAbsolutePath(tilesetId);
		std::string defaultFilename = storedFilename.empty() ? tileset.GetName() + ".bmp" : storedFilename;
		wxFileDialog fileDlg(this, _("Save BMP file"), "", defaultFilename, "BMP files (*.bmp)|*.bmp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fileDlg.ShowModal() == wxID_OK)
		{
			ExportTileset(fileDlg.GetPath().c_str().AsChar(), tilesetId);
		}
	}
}

void DialogAssetManagement::OnBtnCleanupTileset(wxCommandEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		const Tileset& tileset = m_project.GetTileset(tilesetId);
		int cleaned = m_project.CleanupTileset(tilesetId);
		if (wxMessageBox("Removed " + std::to_string(cleaned) + " unused tiles from tileset '" + tileset.GetName() + "'.\n\nTileset should be re-exported to source image before editing, export now?", "Cleanup", wxYES | wxNO | wxICON_WARNING) == wxID_YES)
		{
			ExportTileset(m_project.m_settings.GetAbsolutePath(tilesetId), tilesetId);
		}
		SelectTileset(index);
	}
}

void DialogAssetManagement::OnBrowseTilesImg(wxFileDirPickerEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		const Tileset& tileset = m_project.GetTileset(tilesetId);

		if (tileset.GetCount() > 0)
		{
			if (wxMessageBox("Tileset " + tileset.GetName() + " already contains tiles, ensure the imported image is based on a previous export as a template. Continue?", "Warning", wxOK | wxCANCEL | wxICON_WARNING) == wxID_CANCEL)
			{
				return;
			}
		}

		std::string filename = m_filePickerTilesImg->GetPath();
		MergeTileset(filename, tilesetId);
	}
}

void DialogAssetManagement::MergeTileset(const std::string filename, TilesetId tilesetId)
{
	Tileset& tileset = m_project.GetTileset(tilesetId);
	PaletteId paletteId = tileset.GetDefaultPaletteId();
	Palette& palette = m_project.GetPalette(paletteId);

	// Check palette is compatible
	Palette importedPalette;
	Project::ImportResult paletteResult = m_project.ImportPaletteFromImage(filename, importedPalette);
	if (paletteResult != Project::ImportResult::Success)
	{
		ShowImportError(paletteResult, filename, m_project);
		return;
	}

	// Check if imported palette has changed any colours
	bool paletteMatch = true;
	for (int i = 0; i < Palette::coloursPerPalette; i++)
	{
		if (   (palette.IsColourUsed(i) && importedPalette.IsColourUsed(i))
			&& (palette.GetColour(i) != importedPalette.GetColour(i)))
		{
			paletteMatch = false;
			break;
		}
	}

	if (!paletteMatch)
	{
		MergePaletteDialog dlg(this, palette, importedPalette);
		if (dlg.ShowModal() != wxID_OK)
			return;

		palette = dlg.GetMergedPalette();
	}

	// Keep original name
	std::string name = tileset.GetName();

	// Merge tileset
	Project::ImportResult tilesetResult = m_project.ImportTilesetFromImage(filename, importedPalette, tileset);
	if (tilesetResult != Project::ImportResult::Success)
	{
		ShowImportError(tilesetResult, filename, m_project);
		return;
	}

	tileset.SetName(name);

	//Remember source path
	m_project.m_settings.SetAbsolutePath(tilesetId, filename);

	//Recreate render resources
	m_mainWindow.RefreshTileset();
	m_mainWindow.RefreshAll();

	// Refresh
	PopulatePalettes();
	PopulateTilesets();
	SelectPaletteById(paletteId);
	SelectTilesetById(tilesetId);
}

void DialogAssetManagement::OnListStampSet(wxCommandEvent& event)
{
	SelectStampSet(m_listStampSets->GetSelection());
}

void DialogAssetManagement::OnBtnNewStampSet(wxCommandEvent& event)
{
	//Open BMP
	wxFileDialog fileDlg(this, _("Open image file"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		//Import stampset
		std::string filename = fileDlg.GetPath().c_str().AsChar();
		std::string name = ion::string::GetFilename(filename);
		Palette palette;
		Tileset tileset;
		StampSet stampSet;
		Project::ImportResult result = m_project.ImportStampsetFromImage(filename, palette, tileset, stampSet);

		if (result != Project::ImportResult::Success)
		{
			ShowImportError(result, filename, m_project);
			return;
		}
		
		// Match palette with existing or create new
		MatchPaletteDialog matchDlg(this, m_project, palette, InvalidPaletteId, name);
		matchDlg.ShowModal();
		PaletteId paletteId = matchDlg.m_selectedPaletteId;

		if (paletteId == InvalidPaletteId)
		{
			wxMessageBox("Cannot import tileset, no matching palette selected", "Error");
			return;
		}

		// Add tileset
		tileset.SetDefaultPaletteId(paletteId);
		TilesetId tilesetId = m_project.AddTileset(tileset);
		Tileset& newTileset = m_project.GetTileset(tilesetId);

		// Add stampset
		stampSet.SetTilesetId(tilesetId);
		StampSetId stampsetId = m_project.AddStampSet(stampSet);
		newTileset.SetOwningStampSet(stampsetId);

		//Remember source path
		m_project.m_settings.SetAbsolutePath(stampsetId, filename);

		// Recreate render resources
		m_mainWindow.RefreshTileset();
		m_mainWindow.RefreshAll();

		// Refresh
		PopulatePalettes();
		PopulateTilesets();
		PopulateStampSets();
		SelectPaletteById(paletteId);
		SelectTilesetById(tilesetId);
		SelectStampSetById(stampsetId);
	}
}

void DialogAssetManagement::OnBtnDeleteStampSet(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);
		TilesetId tilesetId = stampSet.GetTilesetId();

		std::vector<MapId> maps;
		int usageCount = GetStampSetUsage(stampSetId, maps);

		if (usageCount > 0)
		{
			std::string msg = "Cannot delete in-use stamp set " + stampSet.GetName() + "\n";

			msg += "\nUsed by map(s):\n\n";

			for (MapId mapId : maps)
			{
				const Map& map = m_project.GetMap(mapId);
				msg += " " + map.GetName() + "\n";
			}

			wxMessageBox(msg, "Error");
			return;
		}

		m_project.DeleteStampSet(stampSetId);
		m_project.DeleteTileset(tilesetId);
		PopulateStampSets();
		PopulateTilesets();
		SelectStampSet(0);
		SelectTileset(0);
	}
}

void DialogAssetManagement::OnBtnScanStampSet(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		std::string filename = m_project.m_settings.GetAbsolutePath(stampSetId);
		MergeStampset(filename, stampSetId);
	}
}

void DialogAssetManagement::ExportStampset(const std::string filename, StampSetId stampSetId)
{
	const StampSet& stampSet = m_project.GetStampSet(stampSetId);
	TilesetId tilesetId = stampSet.GetTilesetId();
	const Tileset& tileset = m_project.GetTileset(tilesetId);
	PaletteId paletteId = tileset.GetDefaultPaletteId();
	const Palette& palette = m_project.GetPalette(paletteId);

	ion::ImageFormatBMP writer(ion::ImageFormat::BitFormat::eIndexed16Colour);

	for (int i = 0; i < Palette::coloursPerPalette; i++)
	{
		Colour colour(0, 0, 0);
		if (palette.IsColourUsed(i))
			colour = palette.GetColour(i);
		writer.SetPaletteEntry(i, ion::ImageFormat::Colour(colour.GetRed(), colour.GetGreen(), colour.GetBlue()));
	}

	const int numStamps = stampSet.GetStampCount();
	const int numStampsSq = (int)ion::maths::Ceil(ion::maths::Sqrt(numStamps));
	const int stampsPerRow = numStampsSq;
	const int tileWidth = m_project.GetPlatformConfig().tileWidth;
	const int tileHeight = m_project.GetPlatformConfig().tileHeight;
	const int stampWidth = m_project.GetPlatformConfig().stampWidth;
	const int stampHeight = m_project.GetPlatformConfig().stampHeight;

	writer.SetDimensions(numStampsSq * stampWidth * tileWidth, numStampsSq * stampHeight * tileHeight);

	int i = 0;
	for (const auto& it : stampSet.GetStamps())
	{
		const Stamp& stamp = it.second;
		int stampX = i % numStampsSq;
		int stampY = i / numStampsSq;
		i++;

		for (int tileX = 0; tileX < stampWidth; tileX++)
		{
			for (int tileY = 0; tileY < stampHeight; tileY++)
			{
				TileId tileId = stamp.GetTile(tileX, tileY);
				u32 tileFlags = stamp.GetTileFlags(tileX, tileY);
				const Tile& tile = tileset.GetTile(tileId);
				for (int pixelX = 0; pixelX < tileWidth; pixelX++)
				{
					for (int pixelY = 0; pixelY < tileHeight; pixelY++)
					{
						int flipPixelX = (tileFlags & Map::eFlipX) ? (tileWidth - 1 - pixelX) : pixelX;
						int flipPixelY = (tileFlags & Map::eFlipY) ? (tileHeight - 1 - pixelY) : pixelY;

						int imageX = (((stampX * stampWidth) + tileX) * tileWidth) + flipPixelX;
						int imageY = (((stampY * stampHeight) + tileY) * tileHeight) + flipPixelY;

						writer.SetColourIndex(imageX, imageY, tile.GetPixelColour(pixelX, pixelY));
					}
				}
			}
		}
	}

	if (!writer.Write(filename))
		wxMessageBox("Error exporting stampset to image '" + filename + "'", "Error");
}

void DialogAssetManagement::OnBtnExportStampSet(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);

		std::string storedFilename = m_project.m_settings.GetAbsolutePath(stampSetId);
		std::string defaultFilename = storedFilename.empty() ? stampSet.GetName() + ".bmp" : storedFilename;
		wxFileDialog fileDlg(this, _("Save BMP file"), "", defaultFilename, "BMP files (*.bmp)|*.bmp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fileDlg.ShowModal() == wxID_OK)
		{
			ExportStampset(fileDlg.GetPath().c_str().AsChar(), stampSetId);
		}
	}
}

void DialogAssetManagement::OnBtnRenameStampSet(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		StampSet& stampSet = m_project.GetStampSet(stampSetId);

		wxTextEntryDialog dlg(this, "Rename stamp set", "Rename stamp set", stampSet.GetName());
		if (dlg.ShowModal() == wxID_OK)
		{
			stampSet.SetName(dlg.GetValue().c_str().AsChar());
			PopulateStampSets();
			SelectStampSetById(stampSetId);
		}
	}
}

void DialogAssetManagement::OnBtnCleanupStampSet(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);
		int cleanedStamps = m_project.CleanupStamps(stampSetId);

		TilesetId tilesetId = stampSet.GetTilesetId();
		const Tileset& tileset = m_project.GetTileset(tilesetId);
		int cleanedTiles = m_project.CleanupTileset(tilesetId);

		std::string msg = "Removed " + std::to_string(cleanedStamps) + " unused stamps from stampset '" + stampSet.GetName() + "'\n";
		msg += "Removed " + std::to_string(cleanedTiles) + " unused tiles from tileset '" + tileset.GetName() + "'\n\nStamp set should be re-exported to source image before editing, export now?";
		wxMessageBox(msg, "Cleanup");

		if (wxMessageBox(msg, "Cleanup", wxYES | wxNO | wxICON_WARNING) == wxID_YES)
		{
			ExportStampset(m_project.m_settings.GetAbsolutePath(stampSetId), stampSetId);
		}

		m_mainWindow.RefreshTileset();
		m_mainWindow.RefreshAll();

		SelectStampSetById(stampSetId);
		SelectTilesetById(tilesetId);
	}
}

void DialogAssetManagement::OnBrowseStampsImg(wxFileDirPickerEvent& event)
{
	int stampSetIdx = m_listStampSets->GetSelection();
	if (stampSetIdx >= 0 && stampSetIdx < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[stampSetIdx];
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);
		
		if (stampSet.GetStampCount() > 0)
		{
			if (wxMessageBox("Stamp set " + stampSet.GetName() + " already contains stamps, ensure the imported image is based on a previous export as a template. Continue?", "Warning", wxOK | wxCANCEL | wxICON_WARNING) == wxID_CANCEL)
			{
				return;
			}
		}

		std::string filename = m_filePickerStampsImg->GetPath();
		MergeStampset(filename, stampSetId);
	}
}

void DialogAssetManagement::MergeStampset(const std::string filename, StampSetId stampSetId)
{
	const int tileWidth = m_project.GetPlatformConfig().tileWidth;
	const int tileHeight = m_project.GetPlatformConfig().tileHeight;

	// Import stampset
	Palette importedPalette;
	Tileset importedTileset;
	StampSet importedStampSet;

	Project::ImportResult result = m_project.ImportStampsetFromImage(filename, importedPalette, importedTileset, importedStampSet);
	if (result != Project::ImportResult::Success)
	{
		ShowImportError(result, filename, m_project);
		return;
	}

	// Check if any stamps have been removed
	StampSet& stampSet = m_project.GetStampSet(stampSetId);
	TilesetId tilesetId = stampSet.GetTilesetId();
	Tileset& tileset = m_project.GetTileset(tilesetId);
	std::vector<StampId> removed;
	for (const auto& stamp : stampSet.GetStamps())
	{
		if (!importedStampSet.StampExists(stamp.first))
			removed.push_back(stamp.first);
	}

	if (removed.size() > 0)
	{
		if (wxMessageBox(std::to_string(removed.size()) + " stamps removed, this will reset any placements to default. Are you sure?", "Warning", wxOK | wxCANCEL | wxICON_WARNING) == wxID_CANCEL)
			return;

		// Clear any used from all maps
		for (auto& map : m_project.GetMaps())
		{
			for (auto& placement : map.second.GetStamps())
			{
				if (ion::utils::stl::Find(removed, placement.m_id))
					placement.m_id = InvalidStampId;
			}
		}
	}

	// Copy collision, animation, palette overlays
	for (auto& lhs: importedStampSet.GetStamps())
	{
		auto& rhs = stampSet.GetStamps().find(lhs.first);
		if (rhs != stampSet.GetStamps().end())
		{
			lhs.second.GetTerrainBeziers() = rhs->second.GetTerrainBeziers();
			lhs.second.GetCollisionTiles() = rhs->second.GetCollisionTiles();
			lhs.second.GetStampAnims() = rhs->second.GetStampAnims();
			lhs.second.GetOverlays() = rhs->second.GetOverlays();

			//Re-allocate animation regions
			for (const auto& stampAnim : rhs->second.GetStampAnims())
			{
				// Add anim to stamp
				StampAnimId stampAnimId = lhs.second.AddStampAnim(stampAnim.second.actorId, stampAnim.second.spriteSheetId, stampAnim.second.spriteAnimId, stampAnim.second.position);

				// If not already allocated in tileset
				Tileset::ReservedBlock reservedBlock = importedTileset.GetReservedBlock(stampAnim.second.spriteAnimId);

				if (reservedBlock.firstTile == InvalidTileId)
				{
					// Allocate tiles
					const Actor& actor = *m_project.GetActor(stampAnim.second.actorId);
					const SpriteSheet& spriteSheet = *actor.GetSpriteSheet(stampAnim.second.spriteSheetId);
					const SpriteAnimation& spriteAnim = *spriteSheet.GetAnimation(stampAnim.second.spriteAnimId);
					reservedBlock = importedTileset.AllocateReservedBlock(stampAnim.second.spriteAnimId, spriteSheet.GetWidthTiles() * spriteSheet.GetHeightTiles());

					// Copy first frame
					std::vector<Tile*> tiles;
					for (int i = 0; i < reservedBlock.numTiles; i++)
					{
						tiles.push_back(&importedTileset.GetTile(reservedBlock.firstTile + i));
					}

					spriteSheet.CopyFrameToTilesetRowMajor(spriteAnim.m_trackSpriteFrame.GetValue(0.0f), tiles);
				}
			}
		}
	}

	// Set new
	tileset.GetTiles() = importedTileset.GetTiles();
	tileset.GetReservedBlocks() = importedTileset.GetReservedBlocks();
	stampSet.GetStamps() = importedStampSet.GetStamps();
	tileset.RebuildHashMap();

	// Match palette with existing or create new
	MatchPaletteDialog matchDlg(this, m_project, importedPalette, tileset.GetDefaultPaletteId(), stampSet.GetName());
	matchDlg.ShowModal();
	PaletteId paletteId = matchDlg.m_selectedPaletteId;

	if (paletteId == InvalidPaletteId)
	{
		wxMessageBox("Cannot import tileset, no matching palette selected", "Error");
		return;
	}

	tileset.SetDefaultPaletteId(paletteId);

	//Remember source path
	m_project.m_settings.SetAbsolutePath(stampSetId, filename);

	//Recreate render resources
	m_mainWindow.RefreshTileset();
	m_mainWindow.RefreshTerrainTileset();
	m_mainWindow.RefreshAll();

	// Refresh
	PopulatePalettes();
	PopulateTilesets();
	PopulateStampSets();
	SelectPaletteById(paletteId);
	SelectTilesetById(tilesetId);
	SelectStampSetById(stampSetId);
}

void DialogAssetManagement::OnListMap(wxCommandEvent& event)
{
	SelectMap(m_listMaps->GetSelection());
}

void DialogAssetManagement::OnListMapDClick(wxCommandEvent& event)
{
	int index = m_listMaps->GetSelection();
	if (index >= 0 && index < m_populatedMaps.size())
	{
		m_listMaps->SetSelection(index);

		MapId mapId = m_populatedMaps[index];

		if (mapId != m_project.GetEditingMapId())
		{
			m_project.SetEditingMap(mapId);
			m_project.SetEditingCollisionMap(mapId);

			m_project.InvalidateMap(true);
			m_project.InvalidateStamps(true);
			m_project.InvalidateCamera(true);
		}

		SelectMap(index);
		m_mainWindow.RefreshAll();
	}
}

void DialogAssetManagement::OnBtnNewMap(wxCommandEvent& event)
{
	NewMapDialog dlg(m_mainWindow, this, m_project);
	if (dlg.ShowModal() == wxID_OK)
	{
		std::string name = dlg.GetMapName();
		StampSetId stampSetId = dlg.GetMapStampSetId();
		ion::Vector2i size = dlg.GetMapSize();

		if (!name.empty() && stampSetId != InvalidStampSetId && size.x > 0 && size.y > 0)
		{
			MapId mapId = m_project.CreateMap(name, stampSetId, size.x, size.y);
			const Map& map = m_project.GetMap(mapId);

			m_project.CreateCollisionMap(mapId, map.GetWidth(), map.GetHeight());

			const StampSet& stampSet = m_project.GetStampSet(stampSetId);
			TilesetId tilesetId = stampSet.GetTilesetId();
			const Tileset& tileset = m_project.GetTileset(tilesetId);
			PaletteId paletteId = tileset.GetDefaultPaletteId();

			// Refresh
			PopulatePalettes();
			PopulateTilesets();
			PopulateStampSets();
			PopulateMaps();
			SelectPaletteById(paletteId);
			SelectTilesetById(tilesetId);
			SelectStampSetById(stampSetId);
			SelectMapById(mapId);

			m_mainWindow.RefreshAll();
		}
	}
}

void DialogAssetManagement::OnBtnDeleteMap(wxCommandEvent& event)
{
	int index = m_listMaps->GetSelection();
	if (index >= 0 && index < m_populatedMaps.size())
	{
		MapId mapId = m_populatedMaps[index];
		if (mapId == m_project.GetEditingMapId())
		{
			wxMessageBox("Cannot delete currently editing map", "Error");
			return;
		}

		m_project.DeleteMap(mapId);

		for (auto& map : m_project.GetMaps())
		{
			if (map.second.GetBackgroundMapId() == mapId)
				map.second.SetBackgroundMap(InvalidMapId);
		}

		PopulateMaps();
		m_mainWindow.RefreshAll();
	}
}

void DialogAssetManagement::OnBtnImportMap(wxCommandEvent& event)
{
	int index = m_listMaps->GetSelection();
	if (index >= 0 && index < m_populatedMaps.size())
	{
		MapId mapId = m_populatedMaps[index];
		Map& map = m_project.GetMap(mapId);

		std::string defaultFilename = map.GetName() + ".bmp";
		wxFileDialog fileDlg(this, _("Open image file"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fileDlg.ShowModal() == wxID_OK)
		{
			StampSetId stampsetId = map.GetStampSetId();
			const StampSet& stampset = m_project.GetStampSet(stampsetId);
			TilesetId tilesetId = stampset.GetTilesetId();
			const Tileset& tileset = m_project.GetTileset(tilesetId);
			std::string filename = fileDlg.GetPath().c_str().AsChar();

			Map importedMap;
			Project::ImportResult result = m_project.ImportMapFromImage(filename, tileset, stampset, importedMap);
			if (result != Project::ImportResult::Success)
			{
				ShowImportError(result, filename, m_project);
				return;
			}

			CollisionMap& collisionMap = m_project.GetCollisionMap(mapId);
			collisionMap.Resize(importedMap.GetWidth(), importedMap.GetHeight(), false, false);

			map.Clear();
			map.Resize(importedMap.GetWidth(), importedMap.GetHeight(), false, false);
			TStampPosMap& stamps = map.GetStamps();
			stamps = importedMap.GetStamps();

			m_project.InvalidateMap(true);
			m_mainWindow.RefreshPanel(MainWindow::Panel::ePanelMap);
		}
	}
}

void DialogAssetManagement::ExportMap(const std::string filename, MapId mapId)
{
	const Map& map = m_project.GetMap(mapId);
	StampSetId stampSetId = map.GetStampSetId();
	const StampSet& stampSet = m_project.GetStampSet(stampSetId);
	TilesetId tilesetId = stampSet.GetTilesetId();
	const Tileset& tileset = m_project.GetTileset(tilesetId);
	PaletteId paletteId = tileset.GetDefaultPaletteId();
	const Palette& palette = m_project.GetPalette(paletteId);

	ion::ImageFormatBMP writer(ion::ImageFormat::BitFormat::eIndexed16Colour);

	for (int i = 0; i < Palette::coloursPerPalette; i++)
	{
		Colour colour(0, 0, 0);
		if (palette.IsColourUsed(i))
			colour = palette.GetColour(i);
		writer.SetPaletteEntry(i, ion::ImageFormat::Colour(colour.GetRed(), colour.GetGreen(), colour.GetBlue()));
	}

	const int tileWidth = m_project.GetPlatformConfig().tileWidth;
	const int tileHeight = m_project.GetPlatformConfig().tileHeight;
	const int stampWidth = m_project.GetPlatformConfig().stampWidth;
	const int stampHeight = m_project.GetPlatformConfig().stampHeight;
	const int mapWidthTiles = map.GetWidth();
	const int mapHeightTiles = map.GetHeight();
	const int mapWidthStamps = mapWidthTiles / stampWidth;
	const int mapHeightStamps = mapHeightTiles / stampHeight;

	writer.SetDimensions(mapWidthTiles * tileWidth, mapHeightTiles * tileHeight);

	for(const auto& stampPlacement : map.GetStamps())
	{
		const Stamp& stamp = stampSet.GetStamp(stampPlacement.m_id);
		int stampX = stampPlacement.m_position.x / stampWidth;
		int stampY = stampPlacement.m_position.y / stampHeight;

		for (int tileX = 0; tileX < stampWidth; tileX++)
		{
			for (int tileY = 0; tileY < stampHeight; tileY++)
			{
				TileId tileId = stamp.GetTile(tileX, tileY);
				u32 tileFlags = stamp.GetTileFlags(tileX, tileY);
				const Tile& tile = tileset.GetTile(tileId);
				for (int pixelX = 0; pixelX < tileWidth; pixelX++)
				{
					for (int pixelY = 0; pixelY < tileHeight; pixelY++)
					{
						int flipPixelX = (tileFlags & Map::eFlipX) ? (tileWidth - 1 - pixelX) : pixelX;
						int flipPixelY = (tileFlags & Map::eFlipY) ? (tileHeight - 1 - pixelY) : pixelY;

						int imageX = (((stampX * stampWidth) + tileX) * tileWidth) + flipPixelX;
						int imageY = (((stampY * stampHeight) + tileY) * tileHeight) + flipPixelY;

						writer.SetColourIndex(imageX, imageY, tile.GetPixelColour(pixelX, pixelY));
					}
				}
			}
		}
	}

	if (!writer.Write(filename))
		wxMessageBox("Error exporting map to image '" + filename + "'", "Error");
}

void DialogAssetManagement::OnBtnExportMap(wxCommandEvent& event)
{
	int index = m_listMaps->GetSelection();
	if (index >= 0 && index < m_populatedMaps.size())
	{
		MapId mapId = m_populatedMaps[index];
		Map& map = m_project.GetMap(mapId);

		std::string defaultFilename = map.GetName() + ".bmp";
		wxFileDialog fileDlg(this, _("Save BMP file"), "", defaultFilename, "BMP files (*.bmp)|*.bmp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fileDlg.ShowModal() == wxID_OK)
		{
			ExportMap(fileDlg.GetPath().c_str().AsChar(), mapId);
		}
	}
}

void DialogAssetManagement::OnBtnRenameMap(wxCommandEvent& event)
{
	int index = m_listMaps->GetSelection();
	if (index >= 0 && index < m_populatedMaps.size())
	{
		MapId mapId = m_populatedMaps[index];
		Map& map = m_project.GetMap(mapId);

		wxTextEntryDialog dlg(this, "Rename map", "Rename map", map.GetName());
		if (dlg.ShowModal() == wxID_OK)
		{
			map.SetName(dlg.GetValue().c_str().AsChar());
			PopulateMaps();
			SelectMapById(mapId);
		}
	}
}

void DialogAssetManagement::OnListBackgroundMap(wxCommandEvent& event)
{
	int fgIndex = m_listMaps->GetSelection();
	int bgIndex = m_choiceBgMap->GetSelection();
	if (   fgIndex >= 0 && fgIndex < m_populatedMaps.size())
	{
		MapId fgMapId = m_populatedMaps[fgIndex];
		MapId bgMapId = InvalidMapId;

		if (bgIndex >= 0 && bgIndex < m_populatedMaps.size())
			bgMapId = m_populatedMaps[bgIndex];

		if (fgMapId == bgMapId)
		{
			wxMessageBox("Map cannot use itself as a background map", "Error");
			SelectMap(fgIndex);
			return;
		}

		Map& fgMap = m_project.GetMap(fgMapId);
		fgMap.SetBackgroundMap(bgMapId);
	}
}

void DialogAssetManagement::OnListSlot0(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot0->GetSelection(), 0);
}

void DialogAssetManagement::OnListSlot1(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot1->GetSelection(), 1);
}

void DialogAssetManagement::OnListSlot2(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot2->GetSelection(), 2);
}

void DialogAssetManagement::OnListSlot3(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot3->GetSelection(), 3);
}

void DialogAssetManagement::OnBtnAutoAssignPalettes(wxCommandEvent& event)
{
	int index = m_listMaps->GetSelection();
	if (index >= 0 && index < m_populatedMaps.size())
	{
		// Collect all used palettes, their usage types, and reference counts
		static const int maxPaletteSlots = 4;
		MapId mapId = m_populatedMaps[index];
		Map& map = m_project.GetMap(mapId);

		std::vector<std::pair<PaletteId, Project::PaletteUsage>> paletteSlots;
		m_project.SortPaletteSlotsForExport(mapId, paletteSlots);

		if (paletteSlots.size() > maxPaletteSlots)
		{
			std::string errorMsg = "Used palettes (" + std::to_string(paletteSlots.size()) + ") exceeds slot count (" + std::to_string(maxPaletteSlots) + "). Usage:\n\n";

			for (const auto& it : paletteSlots)
			{
				const Palette& palette = m_project.GetPalette(it.first);
				errorMsg += " Palette " + palette.GetName() + ":\n";

				if (it.second.usageFlags & Project::PaletteUser::eUsageMap)
				{
					for (const MapId id : it.second.maps)
					{
						errorMsg += "  Map: " + m_project.GetMap(id).GetName() + "\n";
					}
				}

				if (it.second.usageFlags & Project::PaletteUser::eUsageOverlay)
				{
					for (OverlayId id : it.second.overlays)
					{
						char name[1024] = { 0 };

						for (const auto& stampSet : m_project.GetStampSets())
						{
							for (const auto& stamp : stampSet.second.GetStamps())
							{
								for (const auto& region : stamp.second.GetOverlays())
								{
									if (region.first == id)
									{
										sprintf_s(name, 1024, "%s region %i,%i - %i,%i", stampSet.second.GetName().c_str(), region.second.topLeft.x, region.second.topLeft.y, region.second.bottomRight.x, region.second.bottomRight.y);
									}
								}
							}
						}

						errorMsg += "  Overlay: " + std::string(name) + "\n";
					}
				}

				if (it.second.usageFlags & Project::PaletteUser::eUsageActor)
				{
					for (ActorId id : it.second.actors)
					{
						errorMsg += "  Actor: " + m_project.GetActor(id)->GetName() + "\n";
					}
				}

				errorMsg += "\n";
			}

			wxMessageBox(errorMsg.c_str(), "Error");
		}

		wxChoice* lists[4] = { m_choiceSlot0, m_choiceSlot1, m_choiceSlot2, m_choiceSlot3 };

		for (int i = 0; i < maxPaletteSlots; i++)
		{
			PaletteId paletteId = i < paletteSlots.size() ? paletteSlots[i].first : InvalidPaletteId;
			map.AssignPaletteToSlot(paletteId, i);
			lists[i]->SetSelection(paletteId == InvalidPaletteId ? -1 : ion::utils::stl::IndexOf(m_populatedPalettes, paletteId));
		}
	}
}

int DialogAssetManagement::GetPaletteUsage(PaletteId paletteId, std::vector<TilesetId>& tilesets, std::vector<ActorId>& actors, std::vector<MapId>& maps) const
{
	for (const auto& tileset : m_project.GetTilesets())
	{
		if (tileset.second.GetDefaultPaletteId() == paletteId)
			tilesets.push_back(tileset.first);
	}

	for (const auto& actor : m_project.GetActors())
	{
		if (actor.second.GetPaletteId() == paletteId)
		{
			actors.push_back(actor.first);
		}
	}

	for (const auto& map : m_project.GetMaps())
	{
		for (int i = 0; i < map.second.GetNumPaletteSlots(); i++)
		{
			if (map.second.GetPaletteFromSlot(i) == paletteId)
				maps.push_back(map.first);
		}
	}

	return tilesets.size() + actors.size() + maps.size();
}

int DialogAssetManagement::GetTilesetUsage(TilesetId tilesetId, std::vector<StampSetId>& stampSets) const
{
	for (const auto& stampset : m_project.GetStampSets())
	{
		if (stampset.second.GetTilesetId() == tilesetId)
			stampSets.push_back(stampset.first);
	}

	return stampSets.size();
}

int DialogAssetManagement::GetStampSetUsage(StampSetId stampSetId, std::vector<MapId>& maps) const
{
	for (const auto& map : m_project.GetMaps())
	{
		if (map.second.GetStampSetId() == stampSetId)
			maps.push_back(map.first);
	}

	return maps.size();
}

void DialogAssetManagement::ShowImportError(Project::ImportResult result, const std::string& filename, const Project& project)
{
	std::string msg = "Error importing file '" + filename + "'\n\n";

	switch (result)
	{
	case Project::ImportResult::Success:
		return;
	case Project::ImportResult::UnknownError:
		msg += "An unknown error occured";
		break;
	case Project::ImportResult::FileNotFound:
		msg += "File not found";
		break;
	case Project::ImportResult::InvalidFileFormat:
		msg += "File format not supported";
		break;
	case Project::ImportResult::BadHeader:
		msg += "Bad file header or unsupported format";
		break;
	case Project::ImportResult::ImageNotIndexed16:
		msg += "Image is not 16 colour indexed";
		break;
	case Project::ImportResult::PaletteMismatch:
		msg += "Palette does not match image to be replaced";
		break;
	case Project::ImportResult::InvalidDimensions:
		msg += "Image size is not compatible with project settings:\n";
		msg += "\n  Tile width: " + std::to_string(project.GetPlatformConfig().tileWidth);
		msg += "\n  Tile height: " + std::to_string(project.GetPlatformConfig().tileHeight);
		msg += "\n  Stamp width: " + std::to_string(project.GetPlatformConfig().stampWidth);
		msg += "\n  Stamp height: " + std::to_string(project.GetPlatformConfig().stampHeight);
		break;
	case Project::ImportResult::MissingTile:
		msg += "Imported map contains tiles not present in the tileset, import the correct stampset first";
		break;
	case Project::ImportResult::MissingStamp:
		msg += "Imported map contains stamps not present in the stampset, import the correct stampset first";
		break;
	}

	wxMessageBox(msg, "Error");
}
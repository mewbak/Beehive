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
	m_canvasTiles->SetProject(&project);
	m_canvasTiles->SetMainWindow(&mainWindow);
	m_canvasTiles->SetupRendering(&renderer, &glContext, &renderResources);

	m_canvasStamps->SetProject(&project);
	m_canvasStamps->SetMainWindow(&mainWindow);
	m_canvasStamps->SetupRendering(&renderer, &glContext, &renderResources);

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

	const auto& palettes = m_project.GetPalettes();

	for (const auto& palette : palettes)
	{
		m_listPalettes->Append(palette.second.GetName());
		m_choiceTilesetPalette->Append(palette.second.GetName());
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

	const auto& maps = m_project.GetMaps();

	for (const auto& map : maps)
	{
		m_listMaps->Append(map.second.GetName());
		m_populatedMaps.push_back(map.first);
	}

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
		std::vector<std::pair<ActorId, SpriteSheetId>> spriteSheets;
		std::vector<MapId> maps;
		m_txtPaletteUsageCount->SetLabelText(std::to_string(GetPaletteUsage(paletteId, tilesets, spriteSheets, maps)));

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
		m_txtStampSetPalette->SetLabelText(palette.GetName());
		m_txtStampSetTileset->SetLabelText(tileset.GetName());

		std::vector<MapId> maps;
		m_txtStampSetUsageCount->SetLabelText(std::to_string(GetStampSetUsage(stampSetId, maps)));

		m_choicePaletteSlot->Clear();
		const Map& editingMap = m_project.GetEditingMap();
		for (int i = 0; i < editingMap.GetNumPaletteSlots(); i++)
		{
			PaletteId paletteId = editingMap.GetPaletteFromSlot(i);
			std::string paletteName = "[Unassigned]";
			if (paletteId != InvalidPaletteId)
			{
				const Palette& palette = m_project.GetPalette(paletteId);
				paletteName = palette.GetName();
			}

			std::string name = std::to_string(i) + ": " + paletteName;

			if (paletteId == tileset.GetDefaultPaletteId())
				name += "  * default for tileset *";

			m_choicePaletteSlot->Append(name);
		}

		m_choicePaletteSlot->SetSelection(stampSet.GetPaletteSlot());

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

		PaletteViewCtrl* views[4] = { m_paletteViewSlot0, m_paletteViewSlot1, m_paletteViewSlot2, m_paletteViewSlot3 };
		wxChoice* lists[4] = { m_choiceSlot0, m_choiceSlot1, m_choiceSlot2, m_choiceSlot3 };

		for (int i = 0; i < 4; i++)
		{
			lists[i]->Clear();

			if (map.IsBackgroundMap())
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
				}
				else
				{
					const Palette& palette = m_project.GetPalette(id);
					views[i]->SetPalette(palette);
				}

				for (PaletteId paletteId : m_populatedPalettes)
				{
					const Palette& palette = m_project.GetPalette(paletteId);
					lists[i]->Append(palette.GetName());
					if (paletteId == id)
						lists[i]->SetSelection(i);
				}
			}
		}
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
			ShowImportError(result, filename);
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
		std::vector<std::pair<ActorId, SpriteSheetId>> spriteSheets;
		std::vector<MapId> maps;
		int usageCount = GetPaletteUsage(paletteId, tilesets, spriteSheets, maps);

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

			if (spriteSheets.size() > 0)
			{
				msg += "\nUsed by sprite sheet(s):\n\n";

				for (const auto& it : spriteSheets)
				{
					const Actor* actor = m_project.GetActor(it.first);
					const SpriteSheet* spriteSheet = actor->GetSpriteSheet(it.second);
					msg += " " + spriteSheet->GetName() + "\n";
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
			ShowImportError(result, filename);
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

void DialogAssetManagement::OnBtnExportTileset(wxCommandEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
		const Tileset& tileset = m_project.GetTileset(tilesetId);
		PaletteId paletteId = tileset.GetDefaultPaletteId();
		const Palette& palette = m_project.GetPalette(paletteId);

		std::string defaultFilename = tileset.GetName() + ".bmp";
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

			std::string filename = fileDlg.GetPath().c_str().AsChar();
			if (!writer.Write(filename))
				wxMessageBox("Error exporting tileset to image '" + filename + "'", "Error");
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
		wxMessageBox("Removed " + std::to_string(cleaned) + " unused tiles from tileset '" + tileset.GetName() + "'", "Cleanup");
		SelectTileset(index);
	}
}

void DialogAssetManagement::OnBrowseTilesImg(wxFileDirPickerEvent& event)
{
	int index = m_listTilesets->GetSelection();
	if (index >= 0 && index < m_populatedTilesets.size())
	{
		TilesetId tilesetId = m_populatedTilesets[index];
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
		ShowImportError(paletteResult, filename);
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
		ShowImportError(tilesetResult, filename);
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
			ShowImportError(result, filename);
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

		// Add stampset
		stampSet.SetTilesetId(tilesetId);
		StampSetId stampsetId = m_project.AddStampSet(stampSet);
		tileset.SetOwningStampSet(stampsetId);

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

void DialogAssetManagement::OnBtnExportStampSet(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		const StampSet& stampSet = m_project.GetStampSet(stampSetId);
		TilesetId tilesetId = stampSet.GetTilesetId();
		const Tileset& tileset = m_project.GetTileset(tilesetId);
		PaletteId paletteId = tileset.GetDefaultPaletteId();
		const Palette& palette = m_project.GetPalette(paletteId);

		std::string defaultFilename = stampSet.GetName() + ".bmp";
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

			std::string filename = fileDlg.GetPath().c_str().AsChar();
			if (!writer.Write(filename))
				wxMessageBox("Error exporting stampset to image '" + filename + "'", "Error");
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
		int cleaned = m_project.CleanupStamps(stampSetId);
		wxMessageBox("Removed " + std::to_string(cleaned) + " unused stamps from stampset '" + stampSet.GetName() + "'", "Cleanup");
		SelectStampSet(index);
	}
}

void DialogAssetManagement::OnStampPaletteSlot(wxCommandEvent& event)
{
	int index = m_listStampSets->GetSelection();
	if (index >= 0 && index < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[index];
		StampSet& stampSet = m_project.GetStampSet(stampSetId);

		stampSet.SetPaletteSlot(m_choicePaletteSlot->GetSelection());

		m_mainWindow.RefreshTileset();
		m_mainWindow.RefreshAll();
	}
}

void DialogAssetManagement::OnBrowseStampsImg(wxFileDirPickerEvent& event)
{
	int stampSetIdx = m_listStampSets->GetSelection();
	if (stampSetIdx >= 0 && stampSetIdx < m_populatedStampSets.size())
	{
		StampSetId stampSetId = m_populatedStampSets[stampSetIdx];
		std::string filename = m_filePickerStampsImg->GetPath();
		MergeStampset(filename, stampSetId);
	}
}

void DialogAssetManagement::MergeStampset(const std::string filename, StampSetId stampSetId)
{
	// Import stampset
	Palette importedPalette;
	Tileset importedTileset;
	StampSet importedStampSet;
	Project::ImportResult result = m_project.ImportStampsetFromImage(filename, importedPalette, importedTileset, importedStampSet);
	if (result != Project::ImportResult::Success)
	{
		ShowImportError(result, filename);
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

	// Set new
	tileset.GetTiles() = importedTileset.GetTiles();
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

int DialogAssetManagement::GetPaletteUsage(PaletteId paletteId, std::vector<TilesetId>& tilesets, std::vector<std::pair<ActorId, SpriteSheetId>>& spriteSheets, std::vector<MapId>& maps) const
{
	for (const auto& tileset : m_project.GetTilesets())
	{
		if (tileset.second.GetDefaultPaletteId() == paletteId)
			tilesets.push_back(tileset.first);
	}

	for (const auto& actor : m_project.GetActors())
	{
		for (const auto& spriteSheet : actor.second.GetSpriteSheets())
		{
			if (spriteSheet.second.GetPaletteId() == paletteId)
				spriteSheets.push_back(std::make_pair(actor.first, spriteSheet.first));
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

	return tilesets.size() + spriteSheets.size() + maps.size();
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

void DialogAssetManagement::ShowImportError(Project::ImportResult result, const std::string& filename) const
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
		msg += "\n  Tile width: " + std::to_string(m_project.GetPlatformConfig().tileWidth);
		msg += "\n  Tile height: " + std::to_string(m_project.GetPlatformConfig().tileHeight);
		msg += "\n  Stamp width: " + std::to_string(m_project.GetPlatformConfig().stampWidth);
		msg += "\n  Stamp height: " + std::to_string(m_project.GetPlatformConfig().stampHeight);
		break;
	}

	wxMessageBox(msg, "Error");
}
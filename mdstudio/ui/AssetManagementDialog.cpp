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

DialogAssetManagement::DialogAssetManagement(MainWindow& mainWindow, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources)
	: DialogAssetsBase((wxWindow*)&mainWindow)
	, m_project(project)
	, m_mainWindow(mainWindow)
{
	// Setup canvases
	m_canvasTiles->SetProject(&project);
	m_canvasTiles->SetMainWindow(&mainWindow);
	m_canvasTiles->SetupRendering(&renderer, &glContext, &renderResources);

	m_canvasStamps->SetProject(&project);
	m_canvasStamps->SetMainWindow(&mainWindow);
	m_canvasStamps->SetupRendering(&renderer, &glContext, &renderResources);

	const Map& editingMap = project.GetEditingMap();
	m_sizerSlots->GetStaticBox()->SetLabel("Active Slots (Map \'" + editingMap.GetName() + "\')");

	// Populate all tabs
	PopulatePalettes();
	PopulateTilesets();
	PopulateStampSets();

	SelectPalette(0);
	SelectTileset(0);
	SelectStampSet(0);

	Refresh();
}

void DialogAssetManagement::PopulatePalettes()
{
	int index = m_listPalettes->GetSelection();

	m_populatedPalettes.clear();
	m_listPalettes->Clear();

	const auto& palettes = m_project.GetPalettes();

	for (const auto palette : palettes)
	{
		m_listPalettes->Append(palette.second.GetName());
		m_choiceTilesetPalette->Append(palette.second.GetName());
		m_populatedPalettes.push_back(std::make_pair(palette.first, palette.second));
	}

	if (index >= 0 && index < m_populatedPalettes.size())
	{
		m_listPalettes->SetSelection(index);
	}

	PaletteViewCtrl* views[4] = { m_paletteViewSlot0, m_paletteViewSlot1, m_paletteViewSlot2, m_paletteViewSlot3 };
	wxChoice* lists[4] = { m_choiceSlot0, m_choiceSlot1, m_choiceSlot2, m_choiceSlot3 };

	for (int i = 0; i < 4; i++)
	{
		lists[i]->Clear();

		PaletteId id = m_project.GetEditingMap().GetPaletteFromSlot(i);
		if (id == InvalidPaletteId)
		{
			views[i]->SetPalette(Palette());
		}
		else
		{
			const Palette* palette = m_project.GetPalette(id);
			views[i]->SetPalette(*palette);
		}

		for (const auto palette : palettes)
		{
			lists[i]->Append(palette.second.GetName());
			if (palette.first == id)
				lists[i]->SetSelection(i);
		}
	}
}

void DialogAssetManagement::PopulateTilesets()
{
	int index = m_listTilesets->GetSelection();

	m_populatedTilesets.clear();
	m_listTilesets->Clear();

	const auto& tilesets = m_project.GetTilesets();

	for (const auto tileset : tilesets)
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

	for (const auto stampSet : stampSets)
	{
		m_listStampSets->Append(stampSet.second.GetName());
		m_populatedStampSets.push_back(stampSet.first);
	}

	if (index >= 0 && index < m_populatedStampSets.size())
	{
		m_listStampSets->SetSelection(index);
	}
}

void DialogAssetManagement::SelectPalette(int index)
{
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		m_listPalettes->SetSelection(index);

		PaletteId paletteId = m_populatedPalettes[index].first;
		const Palette& palette = m_populatedPalettes[index].second;
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

		PaletteId paletteId = tileset.GetPaletteId();
		const Palette* palette = m_project.GetPalette(paletteId);

		m_canvasTiles->SetTilesetId(tilesetId);
		m_paletteViewTiles->SetPalette(*palette);

		m_txtTilesetName->SetLabelText(tileset.GetName());
		m_txtTilesetId->SetLabelText(std::to_string(tilesetId));
		m_txtTilesetPalette->SetLabelText(palette->GetName());
		m_txtTilesetCount->SetLabelText(std::to_string(tileset.GetCount()));

		std::vector<StampSetId> stampSets;
		m_txtTilesetUsageCount->SetLabelText(std::to_string(GetTilesetUsage(tilesetId, stampSets)));
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

		PaletteId paletteId = tileset.GetPaletteId();
		const Palette* palette = m_project.GetPalette(paletteId);

		m_canvasStamps->SetStampSetId(stampSetId);
		m_paletteViewStamps->SetPalette(*palette);

		m_txtStampSetName->SetLabelText(stampSet.GetName());
		m_txtStampSetId->SetLabelText(std::to_string(stampSetId));
		m_txtStampSetPalette->SetLabelText(palette->GetName());
		m_txtStampSetTileset->SetLabelText(tileset.GetName());

		std::vector<MapId> maps;
		m_txtStampSetUsageCount->SetLabelText(std::to_string(GetStampSetUsage(stampSetId, maps)));

		Refresh();
	}
}

void DialogAssetManagement::AssignPalette(int index, int slotIndex)
{
	PaletteViewCtrl* views[4] = { m_paletteViewSlot0, m_paletteViewSlot1, m_paletteViewSlot2, m_paletteViewSlot3 };
	PaletteId paletteId = m_populatedPalettes[index].first;
	const Palette& palette = m_populatedPalettes[index].second;
	views[slotIndex]->SetPalette(palette);
	m_project.GetEditingMap().AssignPaletteToSlot(paletteId, slotIndex);
	Refresh();
}

void DialogAssetManagement::OnTabChanged(wxNotebookEvent& event)
{

}

void DialogAssetManagement::OnListPalette(wxCommandEvent& event)
{
	SelectPalette(m_listPalettes->GetSelection());
}

void DialogAssetManagement::OnListTilesets(wxCommandEvent& event)
{
	SelectTileset(m_listTilesets->GetSelection());
}

void DialogAssetManagement::OnListStampSet(wxCommandEvent& event)
{
	SelectStampSet(m_listStampSets->GetSelection());
}

void DialogAssetManagement::OnListTilesetPalette(wxCommandEvent& event)
{
	int paletteIdx = m_choiceTilesetPalette->GetSelection();
	int tilesetIdx = m_listTilesets->GetSelection();
	if (   paletteIdx >= 0 && paletteIdx < m_populatedPalettes.size()
		&& tilesetIdx >= 0 && tilesetIdx < m_populatedTilesets.size())
	{
		PaletteId paletteId = m_populatedPalettes[paletteIdx].first;
		TilesetId tilesetId = m_populatedTilesets[tilesetIdx];
		const Palette& palette = m_populatedPalettes[paletteIdx].second;
		Tileset& tileset = m_project.GetTileset(tilesetId);

		if (!m_project.CheckCompatibilityPaletteTileset(paletteId, tilesetId))
		{
			std::string msg = "Palette " + palette.GetName() + " is incompatible with tileset " + tileset.GetName();
			wxMessageBox(msg, "Error");
			return;
		}

		m_paletteViewTiles->SetPalette(palette);
		m_txtTilesetPalette->SetLabelText(palette.GetName());
		tileset.SetPaletteId(paletteId);

		m_mainWindow.RefreshTileset();

		SelectTileset(m_listTilesets->GetSelection());
		SelectStampSet(m_listStampSets->GetSelection());
	}
}

void DialogAssetManagement::OnBtnImportPalette(wxCommandEvent& event)
{
	//Open BMP
	wxFileDialog fileDlg(this, _("Open image files"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		//Import palette
		std::string filename = fileDlg.GetPath().c_str().AsChar();
		Palette palette;
		Project::ImportResult result = m_project.ImportPaletteFromImage(filename, palette);

		if (result != Project::ImportResult::Success)
		{
			ShowImportError(result, filename);
			return;
		}

		//Match with existing or create new
		MatchPaletteDialog matchDlg(this, m_project, palette, InvalidPaletteId);
		matchDlg.ShowModal();

		PopulatePalettes();
		SelectPalette(m_populatedPalettes.size() - 1);
	}
}

void DialogAssetManagement::OnBtnExportPalette(wxCommandEvent& event)
{
	int index = m_listPalettes->GetSelection();
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		const Palette& palette = m_populatedPalettes[index].second;

		wxFileDialog fileDlg(this, _("Save BMP file"), "", "", "BMP files (*.bmp)|*.bmp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
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
		wxTextEntryDialog dlg(this, "Rename palette", "Rename palette", m_populatedPalettes[index].second.GetName());
		if (dlg.ShowModal() == wxID_OK)
		{
			if (Palette* palette = m_project.GetPalette(m_populatedPalettes[index].first))
			{
				palette->SetName(dlg.GetValue().c_str().AsChar());
				PopulatePalettes();
			}
		}
	}
}

void DialogAssetManagement::OnBtnDeletePalette(wxCommandEvent& event)
{
	int index = m_listPalettes->GetSelection();
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		PaletteId paletteId = m_populatedPalettes[index].first;
		const Palette* palette = m_project.GetPalette(paletteId);

		std::vector<TilesetId> tilesets;
		std::vector<std::pair<ActorId, SpriteSheetId>> spriteSheets;
		std::vector<MapId> maps;
		int usageCount = GetPaletteUsage(paletteId, tilesets, spriteSheets, maps);

		if (usageCount > 0)
		{
			std::string msg = "Cannot delete in-use palette " + palette->GetName() + "\n";
			
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

				for (const auto it : spriteSheets)
				{
					const Actor* actor = m_project.GetActor(it.first);
					const SpriteSheet* spriteSheet = actor->GetSpriteSheet(it.second);
					msg += " " + spriteSheet->GetName() + "\n";
				}
			}

			if (maps.size() > 0)
			{
				msg += "\nAssigned to slot(s) in map(s):\n\n";

				for (const auto mapId : maps)
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

int DialogAssetManagement::GetPaletteUsage(PaletteId paletteId, std::vector<TilesetId>& tilesets, std::vector<std::pair<ActorId, SpriteSheetId>>& spriteSheets, std::vector<MapId>& maps) const
{
	for (const auto& tileset : m_project.GetTilesets())
	{
		if (tileset.second.GetPaletteId() == paletteId)
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
	}

	wxMessageBox(msg, "Error");
}
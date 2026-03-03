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

DialogAssetManagement::DialogAssetManagement(MainWindow& mainWindow, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources)
	: DialogAssetsBase((wxWindow*)&mainWindow)
	, m_project(project)
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
	m_populatedPalettes.clear();
	m_listPalettes->Clear();

	const auto& palettes = m_project.GetPalettes();

	for (const auto palette : palettes)
	{
		m_listPalettes->Append(palette.second.GetName());
		m_populatedPalettes.push_back(std::make_pair(palette.first, palette.second));
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
	m_populatedTilesets.clear();
	m_listTilesets->Clear();

	const auto& tilesets = m_project.GetTilesets();

	for (const auto tileset : tilesets)
	{
		m_listTilesets->Append(tileset.second.GetName());
		m_populatedTilesets.push_back(tileset.first);
	}
}

void DialogAssetManagement::PopulateStampSets()
{
	m_populatedStampSets.clear();
	m_listStampSets->Clear();

	const auto& stampSets = m_project.GetStampSets();

	for (const auto stampSet : stampSets)
	{
		m_listStampSets->Append(stampSet.second.GetName());
		m_populatedStampSets.push_back(stampSet.first);
	}
}

void DialogAssetManagement::SelectPalette(int index)
{
	if (index >= 0 && index < m_populatedPalettes.size())
	{
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

		m_txtPaletteNumSprites->SetLabelText("");
		m_txtPaletteNumStamps->SetLabelText("");

		Refresh();
	}
}

void DialogAssetManagement::SelectTileset(int index)
{
	if (index >= 0 && index < m_populatedTilesets.size())
	{
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
	}
}

void DialogAssetManagement::SelectStampSet(int index)
{
	if (index >= 0 && index < m_populatedStampSets.size())
	{
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

void DialogAssetManagement::OnBtnImportPalette(wxCommandEvent& event)
{
	//Open BMP
	wxFileDialog fileDlg(this, _("Open image files"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileDlg.ShowModal() == wxID_OK)
	{
		//Read BMP
		std::string filename = fileDlg.GetPath().c_str().AsChar();
		std::string name = filename.substr(filename.find_last_of("/\\") + 1);
		ion::ImageFormat* reader = ion::ImageFormat::CreateReader(ion::string::GetFileExtension(filename));
		if (reader && reader->Read(filename))
		{
			//Import palette
			Palette palette(name);

			for (int i = 0; i < reader->GetPaletteSize(); i++)
			{
				ion::ImageFormat::Colour colour = reader->GetPaletteEntry(i);
				palette.AddColour(Colour(colour.r, colour.g, colour.b));
			}

			//Match with existing or create new
			MatchPaletteDialog matchDlg(this, m_project, palette, InvalidPaletteId);
			matchDlg.ShowModal();

			PopulatePalettes();
			SelectPalette(m_populatedPalettes.size() - 1);
		}
	}
}

void DialogAssetManagement::OnBtnExportPalette(wxCommandEvent& event)
{
	wxMessageBox("Unimplemented, sorry", "Error");
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
		m_project.DeletePalette(m_populatedPalettes[index].first);
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

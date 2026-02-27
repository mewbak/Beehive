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

#include <ion/renderer/imageformats/ImageFormatBMP.h>

#include "Dialogs.h"
#include "SpriteCanvas.h"

#include <wx/msgdlg.h>

DialogNewProject::DialogNewProject(wxWindow* parent)
	: DialogNewProjectBase(parent) 
{
	for(int i = 0; i < ePlatformNum; i++)
	{
		m_choicePreset->AppendString(PlatformPresets::s_configs[i].name);
	}

	PopulatePreset(PlatformPresets::ePresetMegaDrive);
	m_choicePreset->SetSelection(PlatformPresets::ePresetMegaDrive);
}

void DialogNewProject::OnChoicePreset(wxCommandEvent& event)
{
	PopulatePreset(event.GetInt());
}

void DialogNewProject::PopulatePreset(int index)
{
	PlatformConfig& config = PlatformPresets::s_configs[index];
	m_spinCtrlTileWidth->SetValue(config.tileWidth);
	m_spinCtrlTileHeight->SetValue(config.tileHeight);
	m_spinCtrlStampWidth->SetValue(config.stampWidth);
	m_spinCtrlStampHeight->SetValue(config.stampHeight);

#if BEEHIVE_FIXED_STAMP_MODE
	m_spinCtrlMapWidth->SetValue(8);
	m_spinCtrlMapHeight->SetValue(4);
#else
	m_spinCtrlMapWidth->SetValue(config.scrollPlaneWidthTiles);
	m_spinCtrlMapHeight->SetValue(config.scrollPlaneHeightTiles);
#endif
}

ImportDialog::ImportDialog(wxWindow* parent) : ImportDialogBase(parent)
{

}

void ImportDialog::OnBtnBrowse(wxCommandEvent& event)
{
	wxFileDialog dialog(this, _("Open image files"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if(dialog.ShowModal() == wxID_OK)
	{
		dialog.GetPaths(m_paths);

		if(m_paths.size() == 0)
		{
			m_filenames->Clear();
		}
		else if(m_paths.size() == 1)
		{
			m_filenames->SetValue(m_paths[0]);
		}
		else
		{
			char text[128] = { 0 };
			sprintf(text, "(%u) image files", m_paths.size());
			m_filenames->SetValue(wxString(text));
		}
	}
}

ImportStampsDialog::ImportStampsDialog(wxWindow* parent) : ImportStampsDialogBase(parent)
{

}

void ImportStampsDialog::OnBtnBrowse(wxCommandEvent& event)
{
	wxFileDialog dialog(this, _("Open image files"), "", "", "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (dialog.ShowModal() == wxID_OK)
	{
		dialog.GetPaths(m_paths);

		if (m_paths.size() == 0)
		{
			m_filenames->Clear();
		}
		else if (m_paths.size() == 1)
		{
			m_filenames->SetValue(m_paths[0]);
		}
		else
		{
			char text[128] = { 0 };
			sprintf(text, "(%u) image files", m_paths.size());
			m_filenames->SetValue(wxString(text));
		}
	}
}

void ImportStampsDialog::OnRadioImportFile(wxCommandEvent& event)
{
	m_chkImportPalette->Enable(true);
	m_chkClearPalettes->Enable(true);
}

void ImportStampsDialog::OnRadioImportDir(wxCommandEvent& event)
{
	m_chkImportPalette->Enable(false);
	m_chkClearPalettes->Enable(false);
}

ImportDialogSpriteSheet::ImportDialogSpriteSheet(wxWindow* parent, Project& project, ion::render::Renderer& renderer, wxGLContext& glContext, RenderResources& renderResources)
	: ImportDialogSpriteSheetBase(parent)
	, m_renderResources(renderResources)
	, m_project(project)
{
	m_canvas->SetProject(&project);
	m_canvas->SetupRendering(&renderer, &glContext, &renderResources);
	m_canvas->SetSpriteSheetDimentionsCells(ion::Vector2i(m_spinWidthCells->GetValue(), m_spinHeightCells->GetValue()));
	m_canvas->SetDrawGrid(true);
	m_canvas->SetDrawPreview(true, 0);

	m_spinCellCount->SetValue(m_spinWidthCells->GetValue() * m_spinHeightCells->GetValue());
	m_spinCellCount->SetMax(m_spinWidthCells->GetValue() * m_spinHeightCells->GetValue());
	m_canvas->SetDrawPreview(true, m_spinCellCount->GetValue());
}

void ImportDialogSpriteSheet::OnFileOpened(wxFileDirPickerEvent& event)
{
	const int tileWidth = m_project.GetPlatformConfig().tileWidth;
	const int tileHeight = m_project.GetPlatformConfig().tileHeight;

	ion::ImageFormat* reader = ion::ImageFormat::CreateReader(ion::string::GetFileExtension(event.GetPath().GetData().AsChar()));
	if(reader && reader->Read(event.GetPath().GetData().AsChar()))
	{
		if(reader->GetWidth() % tileWidth != 0 || reader->GetHeight() % tileHeight != 0)
		{
			if(wxMessageBox("Bitmap width/height is not multiple of target platform tile width/height, image will be truncated", "Warning", wxOK | wxCANCEL | wxICON_WARNING) == wxCANCEL)
			{
				return;
			}
		}

		//Create texture from bitmap
		m_renderResources.CreateSpriteSheetPreviewTexture(*reader);

		m_canvas->SetSpriteSheetDimentionsPixels(ion::Vector2i(reader->GetWidth(), reader->GetHeight()));

		m_textName->SetValue(m_filePicker->GetFileName().GetName());

		delete reader;
	}
}

void ImportDialogSpriteSheet::OnGridColourChanged(wxColourPickerEvent& event)
{
	ion::Colour colour((float)event.GetColour().Red() / 255.0f, (float)event.GetColour().Green() / 255.0f, (float)event.GetColour().Blue() / 255.0f);
	m_canvas->SetGridColour(colour);
}

void ImportDialogSpriteSheet::OnSpinWidthCells(wxSpinEvent& event)
{
	m_canvas->SetSpriteSheetDimentionsCells(ion::Vector2i(m_spinWidthCells->GetValue(), m_spinHeightCells->GetValue()));
	m_spinCellCount->SetValue(m_spinWidthCells->GetValue() * m_spinHeightCells->GetValue());
	m_spinCellCount->SetMax(m_spinWidthCells->GetValue() * m_spinHeightCells->GetValue());
	m_canvas->SetDrawPreview(true, m_spinCellCount->GetValue());
}

void ImportDialogSpriteSheet::OnSpinHeightCells(wxSpinEvent& event)
{
	m_canvas->SetSpriteSheetDimentionsCells(ion::Vector2i(m_spinWidthCells->GetValue(), m_spinHeightCells->GetValue()));
	m_spinCellCount->SetValue(m_spinWidthCells->GetValue() * m_spinHeightCells->GetValue());
	m_spinCellCount->SetMax(m_spinWidthCells->GetValue() * m_spinHeightCells->GetValue());
	m_canvas->SetDrawPreview(true, m_spinCellCount->GetValue());
}

void ImportDialogSpriteSheet::OnSpinCellCount(wxSpinEvent& event)
{
	m_canvas->SetDrawPreview(true, m_spinCellCount->GetValue());
}

MatchPaletteDialog::MatchPaletteDialog(wxWindow* parent, Project& project, const Palette& paletteToMatch, PaletteId currentPaletteId)
	: DialogMatchPalette(parent)
	, m_project(project)
{
	m_paletteToMatch = paletteToMatch;

	Populate(currentPaletteId);

	// Set original
	m_paletteViewOld->SetPalette(paletteToMatch);
}

void MatchPaletteDialog::Populate(PaletteId selected)
{
	m_choicePalette->Clear();

	// Build list of all matching palettes
	m_matches = m_project.FindMatchingPalettes(m_paletteToMatch);
	int selectedIdx = 0;

	for(int i = 0; i < m_matches.size(); i++)
	{
		m_choicePalette->AppendString(m_matches[i].second.GetName());
		if (m_matches[i].first == selected)
			selectedIdx = i;
	}

	// Select
	if (m_matches.size() > 0 && m_matches.size() > selectedIdx)
	{
		m_choicePalette->SetSelection(selectedIdx);
		m_paletteViewNew->SetPalette(m_matches[selectedIdx].second);
	}

	m_selectedPaletteId = selected;

	Refresh();
}

void MatchPaletteDialog::OnChoicePalette(wxCommandEvent& event)
{
	int selection = m_choicePalette->GetSelection();

	if (selection >= 0 && m_choicePalette->GetSelection() < m_matches.size())
	{
		m_paletteViewNew->SetPalette(m_matches[selection].second);
	}
}

void MatchPaletteDialog::OnBtnMatch(wxCommandEvent& event)
{
	int selection = m_choicePalette->GetSelection();

	if(selection >= 0 && m_choicePalette->GetSelection() < m_matches.size())
		m_selectedPaletteId = m_matches[m_choicePalette->GetSelection()].first;

	EndModal(wxID_MATCH);
}

void MatchPaletteDialog::OnBtnNew(wxCommandEvent& event)
{
	DialogNewPalette dlg(this);

	dlg.m_paletteView->SetPalette(m_paletteToMatch);
	
	if (dlg.ShowModal() == wxID_OK)
	{
		m_paletteToMatch.SetName(dlg.m_textName->GetValue().c_str().AsChar());
		m_selectedPaletteId = m_project.AddPalette(m_paletteToMatch);
		Populate(m_selectedPaletteId);
	}
}

DialogPaletteManagement::DialogPaletteManagement(wxWindow* parent, Project& project)
	: DialogPaletteManagementBase(parent)
	, m_project(project)
{
	const Map& editingMap = project.GetEditingMap();
	m_sizerSlots->GetStaticBox()->SetLabel("Active Slots (Map \'" + editingMap.GetName() + "\')");
	Populate();
	SelectPalette(0);
}

void DialogPaletteManagement::Populate()
{
	m_populatedPalettes.clear();
	m_listPalettes->Clear();

	const auto palettes = m_project.GetPalettes();

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

	Refresh();
}

void DialogPaletteManagement::SelectPalette(int index)
{
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		PaletteId paletteId = m_populatedPalettes[index].first;
		const Palette& palette = m_populatedPalettes[index].second;
		m_paletteViewSelected->SetPalette(palette);
		m_txtName->SetLabelText(palette.GetName());
		m_txtId->SetLabelText(std::to_string(paletteId));

		m_txtActiveSlot->SetLabelText("Unassigned");

		for (int i = 0; i < m_project.GetEditingMap().GetNumPaletteSlots(); i++)
		{
			if (m_project.GetEditingMap().GetPaletteFromSlot(i) == paletteId)
			{
				m_txtActiveSlot->SetLabelText(std::to_string(i));
				break;
			}
		}

		m_txtNumSprites->SetLabelText("");
		m_txtNumStamps->SetLabelText("");

		Refresh();
	}
}

void DialogPaletteManagement::AssignPalette(int index, int slotIndex)
{
	PaletteViewCtrl* views[4] = { m_paletteViewSlot0, m_paletteViewSlot1, m_paletteViewSlot2, m_paletteViewSlot3 };
	PaletteId paletteId = m_populatedPalettes[index].first;
	const Palette& palette = m_populatedPalettes[index].second;
	views[slotIndex]->SetPalette(palette);
	m_project.GetEditingMap().AssignPaletteToSlot(paletteId, slotIndex);
	Refresh();
}

void DialogPaletteManagement::OnListPalette(wxCommandEvent& event)
{
	SelectPalette(m_listPalettes->GetSelection());
}

void DialogPaletteManagement::OnBtnImport(wxCommandEvent& event)
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

			Populate();
			SelectPalette(m_populatedPalettes.size() - 1);
		}
	}
}

void DialogPaletteManagement::OnBtnExport(wxCommandEvent& event)
{
	wxMessageBox("Unimplemented, sorry", "Error");
}

void DialogPaletteManagement::OnBtnRename(wxCommandEvent& event)
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
				Populate();
			}
		}
	}
}

void DialogPaletteManagement::OnBtnDelete(wxCommandEvent& event)
{
	int index = m_listPalettes->GetSelection();
	if (index >= 0 && index < m_populatedPalettes.size())
	{
		m_project.DeletePalette(m_populatedPalettes[index].first);
		Populate();
	}
}

void DialogPaletteManagement::OnListSlot0(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot0->GetSelection(), 0);
}

void DialogPaletteManagement::OnListSlot1(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot1->GetSelection(), 1);
}

void DialogPaletteManagement::OnListSlot2(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot2->GetSelection(), 2);
}

void DialogPaletteManagement::OnListSlot3(wxCommandEvent& event)
{
	AssignPalette(m_choiceSlot3->GetSelection(), 3);
}

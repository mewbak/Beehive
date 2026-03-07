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

#if BEEHIVE_PLUGIN_LUMINARY
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

MatchPaletteDialog::MatchPaletteDialog(wxWindow* parent, Project& project, const Palette& paletteToMatch, PaletteId currentPaletteId, const std::string& defaultName)
	: DialogMatchPalette(parent)
	, m_project(project)
	, m_defaultName(defaultName)
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

	dlg.m_textName->SetLabelText(m_defaultName);
	dlg.m_paletteView->SetPalette(m_paletteToMatch);
	
	if (dlg.ShowModal() == wxID_OK)
	{
		m_paletteToMatch.SetName(dlg.m_textName->GetValue().c_str().AsChar());
		m_selectedPaletteId = m_project.AddPalette(m_paletteToMatch);
		Populate(m_selectedPaletteId);
	}
}

MergePaleteDialog::MergePaleteDialog(wxWindow* parent, const Palette& original, const Palette& imported)
	: DialogMergePaletteBase(parent)
	, m_original(original)
	, m_imported(imported)
{
	m_paletteViewOriginal->SetPalette(original);
	m_paletteViewImported->SetPalette(imported);
	MergeUp();
}

void MergePaleteDialog::MergeUp()
{
	// Take clashing colours from imported
	m_merged = m_imported;

	for (int i = 0; i < Palette::coloursPerPalette; i++)
	{
		if (!m_imported.IsColourUsed(i) && m_original.IsColourUsed(i))
			m_merged.SetColour(i, m_original.GetColour(i));
	}

	m_paletteViewMerged->SetPalette(m_merged);
}

void MergePaleteDialog::MergeDown()
{
	// Take clashing colours from original
	m_merged = m_original;

	for (int i = 0; i < Palette::coloursPerPalette; i++)
	{
		if (m_imported.IsColourUsed(i) && !m_original.IsColourUsed(i))
			m_merged.SetColour(i, m_imported.GetColour(i));
	}

	m_paletteViewMerged->SetPalette(m_merged);
}

void MergePaleteDialog::OnRadioOriginal(wxCommandEvent& event)
{
	m_radioImported->SetValue(false);
	MergeDown();
}

void MergePaleteDialog::OnRadioImported(wxCommandEvent& event)
{

	m_radioOriginal->SetValue(false);
	MergeUp();
}

void MergePaleteDialog::OnBtnMerge(wxCommandEvent& event)
{

}
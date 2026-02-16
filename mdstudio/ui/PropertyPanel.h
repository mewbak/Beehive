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

#include <ion/beehive/Project.h>
#include <ion/beehive/Map.h>

#include <map>
#include <vector>

#include "UIBase.h"

class MainWindow;

class PropertyPanel : public PropertyPanelBase
{
public:
	PropertyPanel(MainWindow* mainWindow, Project& project, wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL | wxNO_BORDER, const wxString& name = wxPanelNameStr);

	virtual void Refresh(bool eraseBackground = true, const wxRect *rect = NULL);
	virtual void OnPropertyChanged(wxPropertyGridEvent& event);
	virtual void OnRightClick(wxMouseEvent& event);

	void SetGameObject(GameObjectTypeId gameObjectTypeId, GameObjectId gameObjectId);
	void SetGameObjectType(GameObjectTypeId gameObjectTypeId);
	void SetArchetype(GameObjectTypeId gameObjectTypeId, GameObjectArchetypeId archetypeId);
	void SetPrefabChild(GameObjectTypeId rootTypeId, GameObjectId rootObjectId, GameObjectTypeId childTypeId, GameObjectId childInstanceId);

protected:

private:
	enum ContextMenu
	{
		Default,
		EditScript,
		CompileScript
	};

	enum class BuiltInProperties
	{
		Name,
		SizeFromSprite,
		Width,
		Height
	};

	GameObjectBase* GetEditingObject();
	GameObjectType* GetEditingType();
	std::string GetEditingName();
	GameObjectVariable* PrepareVarForEditing(const char* variableName, int componentIdx);

	void SetObjectProperty(GameObjectVariable* variable, const std::string& value, GameObjectBase* gameObject);

	void OnContextMenuClick(wxCommandEvent& event);

	void AddProperty(const GameObjectBase* gameObject, const GameObjectType* gameObjectType, const GameObjectVariable& variable, int componentIdx, bool enabled = true);
	int PopulateSpriteActorList(wxArrayString& list, const std::string& selectedValue);
	int PopulateSpriteSheetList(wxArrayString& list, const Actor& actor, const std::string& selectedValue);
	int PopulateSpriteAnimList(wxArrayString& list, const Actor& actor, SpriteSheetId spriteSheetId, const std::string& selectedValue);
	int PopulateGameObjectTypeList(wxArrayString& list, const std::string& selectedValue);
	int PopulateGameObjectList(wxArrayString& list, const std::string& selectedValue);
	int PopulateArchetypeList(wxArrayString& list, GameObjectTypeId gameObjectTypeId, const std::string& selectedValue);

	Project& m_project;
	MainWindow* m_mainWindow;
	GameObjectId m_gameObjectId;
	GameObjectId m_prefabChildId;
	GameObjectTypeId m_gameObjectTypeId;
	GameObjectTypeId m_prefabChildTypeId;
	GameObjectArchetypeId m_archetypeId;

	wxPGProperty* m_contextProperty;
};
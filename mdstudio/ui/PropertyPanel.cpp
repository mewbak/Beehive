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

#include "PropertyPanel.h"
#include "MainWindow.h"
#include "Dialogs.h"

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <ion/core/io/File.h>
#include <ion/core/io/FileDevice.h>
#include <ion/beehive/GameObjectUtils.h>

#if defined BEEHIVE_PLUGIN_LUMINARY
#include <luminary/Types.h>
#include <luminary/ScriptCompiler.h>
#include <luminary/BeehiveToLuminary.h>
#endif

static const std::string s_defaultVarName = "[ DEFAULT_VALUE ]";

PropertyPanel::PropertyPanel(MainWindow* mainWindow, Project& project, wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: PropertyPanelBase(parent, id, pos, size, style)
	, m_project(project)
	, m_mainWindow(mainWindow)
{
	m_gameObjectId = InvalidGameObjectId;
	m_prefabChildId = InvalidGameObjectId;
	m_gameObjectTypeId = InvalidGameObjectTypeId;
	m_prefabChildTypeId = InvalidGameObjectTypeId;
	m_archetypeId = InvalidGameObjectArchetypeId;
	m_contextProperty = nullptr;
}

GameObjectBase* PropertyPanel::GetEditingObject()
{
	GameObjectBase* editingObject = nullptr;

	// Are we editing a prefab child?
	if (!editingObject)
		editingObject = m_project.GetGameObjectType(m_prefabChildTypeId) ? m_project.GetGameObjectType(m_prefabChildTypeId)->FindPrefabChild(m_prefabChildId) : nullptr;

	// Are we editing an archetype?
	if(!editingObject)
		editingObject = m_project.GetGameObjectType(m_gameObjectTypeId) ? m_project.GetGameObjectType(m_gameObjectTypeId)->GetArchetype(m_archetypeId) : nullptr;

	// Are we editing a game object?
	if(!editingObject)
		editingObject = m_project.GetEditingMap().GetGameObject(m_gameObjectId);

	// Are we editing a game object type?
	if(!editingObject)
		editingObject = m_project.GetGameObjectType(m_gameObjectTypeId);

	// Are we editing a prefab child type?
	if (!editingObject)
		editingObject = m_project.GetGameObjectType(m_prefabChildTypeId);

	return editingObject;
}

GameObjectType* PropertyPanel::GetEditingType()
{
	if (GameObjectBase* object = GetEditingObject())
	{
		return GetGameObjectType(m_project, object);
	}
	
	return nullptr;
}

std::string PropertyPanel::GetEditingName()
{
	GameObjectBase* object = GetEditingObject();
	GameObjectType* objectType = GetEditingType();
	if (object->GetName().size() > 0)
		return object->GetName();
	else
		return objectType->GetName();
}

void PropertyPanel::Refresh(bool eraseBackground, const wxRect *rect)
{
	if(!m_mainWindow->IsRefreshLocked())
	{
		m_propertyGrid->Clear();

		GameObjectBase* editingObject = GetEditingObject();
		GameObjectType* editingObjectType = GetEditingType();
		ActorId actorId;
		SpriteSheetId spriteSheetId;
		Actor* actor = FindGameObjectActor(m_project, editingObject, actorId);
		SpriteSheet* spriteSheet = FindGameObjectSpriteSheet(m_project, actor, editingObject, spriteSheetId, false);
		std::string name = GetEditingName();
		std::vector<GameObjectVariable> variables;
		GameObjectDimensionsSource dimensionsSource;
		ion::Vector2i dimensions = FindGameObjectDimensions(m_project, editingObject, dimensionsSource);
		GameObjectGetMergedVariables(m_project, editingObject, variables);

		if (editingObject)
		{
			std::string editingName = name;
			if(dynamic_cast<GameObjectType::PrefabChild*>(editingObject))
				editingName += " (prefab child : " + editingObjectType->GetName() + ")";
			else if (dynamic_cast<GameObject*>(editingObject))
				editingName += " (entity : " + editingObjectType->GetName() + ")";
			else if(dynamic_cast<GameObjectArchetype*>(editingObject))
				editingName += " (archetype : " + editingObjectType->GetName() + ")";
			else
				editingName += " (entity type)";

			m_labelName->SetLabel(editingName);

			//Built-in properties
			wxStringProperty* nameProp = new wxStringProperty("Name");
			nameProp->SetAttribute("isScript", false);
			nameProp->SetValue(name);
			nameProp->SetAttribute("builtInProp", (long)BuiltInProperties::Name);

			wxBoolProperty* sizeFromSpriteProp = new wxBoolProperty("Size from Sprite");
			sizeFromSpriteProp->SetEditor(new wxPGCheckBoxEditor);
			sizeFromSpriteProp->SetAttribute("isScript", false);
			sizeFromSpriteProp->SetValue(dimensionsSource == GameObjectDimensionsSource::SpriteSheet);
			sizeFromSpriteProp->SetAttribute("builtInProp", (long)BuiltInProperties::SizeFromSprite);

			wxStringProperty* widthProp = new wxStringProperty("Width");
			widthProp->SetAttribute("isScript", false);
			widthProp->SetValue(dimensions.x);
			widthProp->SetAttribute("builtInProp", (long)BuiltInProperties::Width);

			wxStringProperty* heightProp = new wxStringProperty("Height");
			heightProp->SetAttribute("isScript", false);
			heightProp->SetValue(dimensions.y);
			heightProp->SetAttribute("builtInProp", (long)BuiltInProperties::Height);

#if defined BEEHIVE_PLUGIN_LUMINARY
			// Can't edit type name, it's scanned from project source
			if (editingObject == editingObjectType || dynamic_cast<GameObjectArchetype*>(editingObject))
				nameProp->Enable(false);
#endif

			m_propertyGrid->Append(nameProp);
			wxPGProperty* addedsizeFromSpriteProp = m_propertyGrid->Append(sizeFromSpriteProp);
			wxPGProperty* addedWidthProp = m_propertyGrid->Append(widthProp);
			wxPGProperty* addedHeightProp = m_propertyGrid->Append(heightProp);

			// If we have no sprite sheet, disable the Size from Sprite checkbox
			if (!spriteSheet)
				addedsizeFromSpriteProp->SetFlagRecursively(wxPG_PROP_DISABLED, true);

			switch (dimensionsSource)
			{
			case GameObjectDimensionsSource::Object:
				widthProp->SetBackgroundColour(*wxYELLOW);
				heightProp->SetBackgroundColour(*wxYELLOW);
				addedWidthProp->SetHelpString("From entity instance");
				addedHeightProp->SetHelpString("From entity instance");
				break;
			case GameObjectDimensionsSource::Type:
				addedWidthProp->SetHelpString("From entity type");
				addedHeightProp->SetHelpString("From entity type");
				break;
			case GameObjectDimensionsSource::SpriteSheet:
				addedWidthProp->SetHelpString("From sprite");
				addedHeightProp->SetHelpString("From sprite");
				break;
			}

			//Add all properties from all components
			int componentIdx = -1;

#if 0
			if (gameObjectType->GetScriptVariables().size() > 0)
			{
				//Populate all variables, mark unmodifiable ones as read only
				for (auto variable : gameObjectType->GetScriptVariables())
				{
					if (componentIdx != variable.m_componentIdx)
					{
						m_propertyGrid->Append(new wxPropertyCategory(variable.m_componentName));
						componentIdx = variable.m_componentIdx;
					}

					if (gameObjectType->FindVariable(variable.m_name, variable.m_componentIdx))
					{
						//Variable is modifiable
						if (const GameObjectVariable* overriddenVar = gameObject->FindVariable(variable.m_name, variable.m_componentIdx))
						{
							AddProperty(*gameObject, *overriddenVar, variable.m_componentIdx);
						}
						else
						{
							AddProperty(*gameObject, variable, variable.m_componentIdx);
						}
					}
					else
					{
						//Script variable only, show but mark read only
						AddProperty(*gameObject, variable, variable.m_componentIdx, false);
					}
				}
			}
			else
#endif
			{
				for (auto variable : variables)
				{
					if (componentIdx != variable.m_componentIdx)
					{
						m_propertyGrid->Append(new wxPropertyCategory(variable.m_componentName));
						componentIdx = variable.m_componentIdx;
					}

					AddProperty(editingObject, editingObjectType, variable, variable.m_componentIdx);
				}
			}
		}

		m_mainWindow->GetMapPanel()->Refresh();
	}
}

GameObjectVariable* PropertyPanel::PrepareVarForEditing(const char* variableName, int componentIdx)
{
	GameObjectBase* editingObject = GetEditingObject();
	GameObjectType* editingObjectType = GetEditingType();

	// Find on object
	GameObjectVariable* instanceVariable = editingObject->FindVariable(variableName, componentIdx);

	// Find on object type
	GameObjectVariable* typeVariable = editingObjectType->FindVariable(variableName, componentIdx);
	ion::debug::Assert(typeVariable, "PropertyPanel::OnPropertyChanged() - Variable \'%s\' does not exist on type \'%s\'", variableName, editingObjectType->GetName().c_str());

	// Prefer instance variable
	GameObjectVariable* variable = instanceVariable ? instanceVariable : typeVariable;

	if (!variable)
	{
		if (editingObject != editingObjectType)
		{
			// Copy type variable to a new instance variable
			variable = &editingObject->CopyVariable(*typeVariable);
			variable->m_overriddenOnInstance = true;
		}
		else
		{
			// Editing a type, allow editing the original
			variable = typeVariable;
		}
	}

	return variable;
}

void PropertyPanel::OnPropertyChanged(wxPropertyGridEvent& event)
{
	if (wxPGProperty* property = event.GetProperty())
	{
		GameObjectBase* editingObject = GetEditingObject();
		GameObjectType* editingObjectType = GetEditingType();

		wxString variableName = property->GetAttribute("variableName", "");
		wxVariant componentIdx = property->GetAttribute("componentIdx");
		wxString value = property->GetValueAsString();

		int builtInType = property->GetAttributeAsLong("builtInProp", -1);

		switch (builtInType)
		{
			case (int)BuiltInProperties::Name:
			{
				// Can't set type name, it comes from codebase scan
				if (editingObject != editingObjectType)
					editingObject->SetName(value.c_str().AsChar());
				break;
			}

			case (int)BuiltInProperties::SizeFromSprite:
			{
				// Backup size
				GameObjectDimensionsSource dimensionsSource;
				ion::Vector2i originalDimensions = FindGameObjectDimensions(m_project, editingObject, dimensionsSource);

				editingObject->SetDimensionsFromSprite(property->GetValue().GetBool());

				// If set to get size from object but it defaults back to sprite, copy over the previous size
				if (!editingObject->GetDimensionsFromSprite())
				{
					FindGameObjectDimensions(m_project, editingObject, dimensionsSource);
					if (dimensionsSource == GameObjectDimensionsSource::SpriteSheet)
					{
						editingObject->SetDimensions(originalDimensions);
					}
				}
				break;
			}

			case (int)BuiltInProperties::Width:
			{
				ion::Vector2i dimensions = editingObject->GetDimensions();
				dimensions.x = property->GetValue().GetLong();

				if (dimensions.x == 0)
				{
					// If x is 0, reset both, so it uses default behaviour
					dimensions.y = 0;
				}
				else if(dimensions.y == 0)
				{
					// If y is 0, set both, to pull it out of default behaviour
					dimensions.y = dimensions.x;
				}

				editingObject->SetDimensions(dimensions);
				break;
			}

			case (int)BuiltInProperties::Height:
			{
				ion::Vector2i dimensions = editingObject->GetDimensions();
				dimensions.y = property->GetValue().GetLong();

				if (dimensions.y == 0)
				{
					// If y is 0, reset both, so it uses default behaviour
					dimensions.x = 0;
				}
				else if (dimensions.x == 0)
				{
					// If x is 0, set both, to pull it out of default behaviour
					dimensions.x = dimensions.y;
				}

				editingObject->SetDimensions(dimensions);
				break;
			}

			default:
			{
				if (value != s_defaultVarName)
				{
					GameObjectVariable* variable = PrepareVarForEditing(variableName.c_str().AsChar(), componentIdx.GetInteger());

					if (variable)
					{
						variable->m_value = value.c_str().AsChar();
						SetObjectProperty(variable, editingObject);
					}
				}

				break;
			}
		}


		Refresh();
	}
}

void PropertyPanel::SetObjectProperty(GameObjectVariable* variable, GameObjectBase* gameObject)
{
	if (gameObject)
	{
		if (variable->HasTag("SPRITE_ACTOR"))
		{
			ActorId actorId = m_project.FindActorId(variable->m_value);
			if (actorId != InvalidActorId)
				gameObject->SetSpriteActorId(actorId);
		}
	}
}

void PropertyPanel::OnRightClick(wxMouseEvent& event)
{
	m_contextProperty = m_propertyGrid->GetItemAtY(event.GetPosition().y);

	if (m_contextProperty)
	{
		if (GameObject* gameObject = m_project.GetEditingMap().GetGameObject(m_gameObjectId))
		{
			wxMenu contextMenu;

			wxMenuItem* defaultItem = contextMenu.Append(ContextMenu::Default, "Revert to Default");
			defaultItem->Enable(gameObject->GetOriginalArchetype() != InvalidGameObjectArchetypeId);

			wxMenuItem* editScriptItem = contextMenu.Append(ContextMenu::EditScript, "Edit Script");
			editScriptItem->Enable(m_contextProperty->GetAttribute("isScript"));

			wxMenuItem* compileScriptItem = contextMenu.Append(ContextMenu::CompileScript, "Compile Script");
			compileScriptItem->Enable(m_contextProperty->GetAttribute("isScript"));

			contextMenu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&PropertyPanel::OnContextMenuClick, NULL, this);
			PopupMenu(&contextMenu);
		}
	}
}

void PropertyPanel::OnContextMenuClick(wxCommandEvent& event)
{
	if (m_contextProperty)
	{
		GameObjectBase* editingObject = GetEditingObject();
		GameObjectType* editingObjectType = GetEditingType();

		std::string variableName = m_contextProperty->GetAttribute("variableName").GetString();
		int componentIdx = m_contextProperty->GetAttribute("componentIdx").GetInteger();

		switch (event.GetId())
		{
			case ContextMenu::Default:
			{
				break;
			}

			case ContextMenu::EditScript:
			{
#if defined BEEHIVE_PLUGIN_LUMINARY
				//Find or add overridden var on game object
				GameObjectVariable* variable = PrepareVarForEditing(variableName.c_str(), componentIdx);
				if (variable)
				{
					//TODO: Do this in Beehive-side script compiler
					const std::string engineRootDir = m_project.m_settings.Get("engineootDir");
					const std::string projectRootDir = m_project.m_settings.Get("projectRootDir");

					const std::string scriptsSourceDir = projectRootDir + "\\SCRIPTS\\";
					const std::string scriptsEngineIncludes = engineRootDir + "\\INCLUDE\\";

					const std::string scriptSourceFilename = editingObjectType->GetName() + ".cpp";
					const std::string scriptSourceFullPath = scriptsSourceDir + "\\" + scriptSourceFilename;

					//Generate header
					luminary::ScriptTranspiler scriptTranspiler;
					luminary::Entity entity;
					luminary::beehive::ConvertEntityType(m_project, *editingObjectType, entity);
					scriptTranspiler.GenerateEntityCppHeader(entity, scriptsSourceDir);

					//Generate boilerplate
					if (ion::io::FileDevice* device = ion::io::FileDevice::GetDefault())
					{
						if (!device->GetFileExists(scriptSourceFullPath))
						{
							scriptTranspiler.GenerateEntityCppBoilerplate(entity, scriptsSourceDir);
							variable->m_value = scriptSourceFilename;
							Refresh();
						}
					}

					//Open in default shell program
					std::string shellCmd = "rundll32 SHELL32.DLL,ShellExec_RunDLL \"" + scriptSourceFullPath + "\"";
					wxExecute(shellCmd);
				}
#endif
				break;
			}

			case ContextMenu::CompileScript:
			{
#if defined BEEHIVE_PLUGIN_LUMINARY
				//Find or add overridden var on game object
				GameObjectVariable* variable = PrepareVarForEditing(variableName.c_str(), componentIdx);
				if (variable)
				{
					//TODO: Do this in Beehive-side script compiler
					m_mainWindow->ShowPanelScriptCompile();
					if (ScriptCompilePanel* panel = m_mainWindow->GetScriptCompilePanel())
					{
						const std::string engineRootDir = m_project.m_settings.Get("engineRootDir");
						const std::string projectRootDir = m_project.m_settings.Get("projectRootDir");

						const std::string scriptsSourceDir = projectRootDir + "\\SCRIPTS\\";
						const std::string scriptsEngineIncludes = engineRootDir + "\\INCLUDE\\";
						const std::string scriptsExportDir = projectRootDir + "\\DATA\\SCRIPTS\\";

						const std::string scriptSourceFilename = editingObjectType->GetName() + ".cpp";
						const std::string scriptSourceFullPath = scriptsSourceDir + "\\" + scriptSourceFilename;

						//Generate header
						luminary::ScriptTranspiler scriptTranspiler;
						luminary::Entity entity;
						luminary::beehive::ConvertEntityType(m_project, *editingObjectType, entity);
						scriptTranspiler.GenerateEntityCppHeader(entity, scriptsSourceDir);

						//Generate boilerplate
						if (ion::io::FileDevice* device = ion::io::FileDevice::GetDefault())
						{
							if (!device->GetFileExists(scriptSourceFullPath))
							{
								scriptTranspiler.GenerateEntityCppBoilerplate(entity, scriptsSourceDir);
								variable->m_value = scriptSourceFilename;
								Refresh();
							}
						}

						//Compile
						const std::string scriptOutNameRelease = ion::string::RemoveSubstring(scriptSourceFilename, ".cpp");
						const std::string scriptOutFullPathRelease = scriptsExportDir + scriptOutNameRelease;
						std::vector<std::string> includes;
						std::vector<std::string> defines;
						includes.push_back(scriptsEngineIncludes);
						includes.push_back(scriptsSourceDir);
						defines.push_back("_RELEASE");
						panel->BeginCompileAsync(scriptSourceFullPath, scriptOutFullPathRelease, includes, defines, nullptr);
					}
				}
#endif
				break;
			}
		}

		m_contextProperty = nullptr;
	}
}

void PropertyPanel::SetGameObject(GameObjectTypeId gameObjectTypeId, GameObjectId gameObjectId)
{
	m_gameObjectId = gameObjectId;
	m_prefabChildId = InvalidGameObjectId;
	m_gameObjectTypeId = gameObjectTypeId;
	m_prefabChildTypeId = InvalidGameObjectTypeId;
	m_archetypeId = InvalidGameObjectArchetypeId;
	Refresh();
}

void PropertyPanel::SetGameObjectType(GameObjectTypeId gameObjectTypeId)
{
	m_gameObjectId = InvalidGameObjectId;
	m_prefabChildId = InvalidGameObjectId;
	m_gameObjectTypeId = gameObjectTypeId;
	m_prefabChildTypeId = InvalidGameObjectTypeId;
	m_archetypeId = InvalidGameObjectArchetypeId;
	Refresh();
}

void PropertyPanel::SetArchetype(GameObjectTypeId gameObjectTypeId, GameObjectArchetypeId archetypeId)
{
	m_gameObjectId = InvalidGameObjectId;
	m_prefabChildId = InvalidGameObjectId;
	m_gameObjectTypeId = gameObjectTypeId;
	m_prefabChildTypeId = InvalidGameObjectTypeId;
	m_archetypeId = archetypeId;
	Refresh();
}

void PropertyPanel::SetPrefabChild(GameObjectTypeId rootTypeId, GameObjectId rootObjectId, GameObjectTypeId childTypeId, GameObjectId childInstanceId)
{
	m_gameObjectId = rootObjectId;
	m_prefabChildId = childInstanceId;
	m_gameObjectTypeId = rootTypeId;
	m_prefabChildTypeId = childTypeId;
	m_archetypeId = InvalidGameObjectArchetypeId;
	Refresh();
}

void PropertyPanel::AddProperty(const GameObjectBase* gameObject, const GameObjectType* gameObjectType, const GameObjectVariable& variable, int componentIdx, bool enabled)
{
	wxPGProperty* property = nullptr;
	bool selectionValid = true;

	std::string propName = componentIdx == -1 ? gameObject->GetName() : variable.m_componentName;
	propName += "_" + variable.m_name + "_" + std::to_string(componentIdx);

#if defined BEEHIVE_PLUGIN_LUMINARY
	if (variable.HasTag("SPRITE_ACTOR"))
	{
		wxArrayString* list = new wxArrayString();

		int selection = PopulateSpriteActorList(*list, variable.m_value);

		wxEnumProperty* choiceProp = new wxEnumProperty(variable.m_name, propName, *list);
		property = choiceProp;

		if (selection >= 0)
		{
			choiceProp->SetChoiceSelection(selection);
		}
		else
		{
			//Add a blank entry
			choiceProp->AddChoice("[none]");
			choiceProp->SetChoiceSelection(choiceProp->GetChoices().GetCount() - 1);
			selectionValid = false;
		}
	}
	else if (variable.HasTag("SPRITE_SHEET"))
	{
		wxArrayString* list = new wxArrayString();
		int selection = -1;

		// Find the actor var in this component to fetch the sprite sheet from
		const GameObjectVariable* spriteActorVar = FindGameObjectVariableByTag(m_project, gameObject, "SPRITE_ACTOR", componentIdx);
		if (spriteActorVar)
		{
			const Actor* actor = m_project.FindActor(spriteActorVar->m_value);
			if (actor)
			{
				selection = PopulateSpriteSheetList(*list, *actor, variable.m_value);
			}
		}
		
		wxEnumProperty* choiceProp = new wxEnumProperty(variable.m_name, propName, *list);
		property = choiceProp;

		if (selection >= 0)
		{
			choiceProp->SetChoiceSelection(selection);
		}
		else
		{
			//Add a blank entry
			choiceProp->AddChoice("[none]");
			choiceProp->SetChoiceSelection(choiceProp->GetChoices().GetCount() - 1);
			selectionValid = false;
		}
	}
	else if (variable.HasTag("SPRITE_ANIM"))
	{
		wxArrayString* list = new wxArrayString();
		int selection = -1;

		// Find the actor and sprite sheet var in this component to fetch the anim from
		const GameObjectVariable* spriteActorVar = FindGameObjectVariableByTag(m_project, gameObject, "SPRITE_ACTOR", componentIdx);
		const GameObjectVariable* spriteSheetVar = FindGameObjectVariableByTag(m_project, gameObject, "SPRITE_SHEET", componentIdx);
		if (spriteActorVar && spriteSheetVar)
		{
			const Actor* actor = m_project.FindActor(spriteActorVar->m_value);
			if (actor)
			{
				SpriteSheetId spriteSheetId = actor->FindSpriteSheetId(spriteSheetVar->m_value);
				selection = PopulateSpriteAnimList(*list, *actor, spriteSheetId, variable.m_value);
			}
		}

		wxEnumProperty* choiceProp = new wxEnumProperty(variable.m_name, propName, *list);
		property = choiceProp;

		if (selection >= 0)
		{
			choiceProp->SetChoiceSelection(selection);
		}
		else
		{
			//Add a blank entry
			choiceProp->AddChoice("[none]");
			choiceProp->SetChoiceSelection(choiceProp->GetChoices().GetCount() - 1);
			selectionValid = false;
		}
	}
	else if (variable.HasTag("ENTITY_DESC"))
	{
		wxArrayString* list = new wxArrayString();
		int selection = PopulateGameObjectTypeList(*list, variable.m_value);
		wxEnumProperty* choiceProp = new wxEnumProperty(variable.m_name, propName, *list);
		property = choiceProp;

		if (selection >= 0)
		{
			choiceProp->SetChoiceSelection(selection);
		}
		else
		{
			//Add a blank entry
			choiceProp->AddChoice("[none]");
			choiceProp->SetChoiceSelection(choiceProp->GetChoices().GetCount() - 1);
			selectionValid = false;
		}
	}
	else if (variable.HasTag("ENTITY_ARCHETYPE"))
	{
		//Find entity type in this component first
		GameObjectTypeId gameObjTypeId = InvalidGameObjectTypeId;
		const GameObjectVariable* gameObjTypeVar = FindGameObjectVariableByTag(m_project, gameObject, "ENTITY_DESC", componentIdx);
		if (gameObjTypeVar)
		{
			if (const GameObjectType* gameObjType = m_project.FindGameObjectType(gameObjTypeVar->m_value))
			{
				gameObjTypeId = gameObjType->GetId();
			}
		}

		wxArrayString* list = new wxArrayString();
		int selection = PopulateArchetypeList(*list, gameObjTypeId, variable.m_value);
		wxEnumProperty* choiceProp = new wxEnumProperty(variable.m_name, propName, *list);
		property = choiceProp;

		if (selection >= 0)
		{
			choiceProp->SetChoiceSelection(selection);
		}
		else
		{
			//Add a blank entry
			choiceProp->AddChoice("[none]");
			choiceProp->SetChoiceSelection(choiceProp->GetChoices().GetCount() - 1);
			selectionValid = false;
		}
	}
	else
#endif
	{
		property = new wxStringProperty(variable.m_name, propName, variable.m_value);
	}

	if (property)
	{
		property->Enable(enabled);
		property->SetAttribute("variableName", variable.m_name);
		property->SetAttribute("componentIdx", componentIdx);
		property->SetAttribute("variableSize", variable.m_size);
		property->SetAttribute("variable", wxVariant((void*)&variable));
		property->SetAttribute("isScript", variable.HasTag("SCRIPT_DATA"));

		m_propertyGrid->Append(property);

		if (!selectionValid)
			property->SetBackgroundColour(*wxRED);
		else if (variable.m_overriddenOnInstance)
			property->SetBackgroundColour(*wxYELLOW);
	}
}

int PropertyPanel::PopulateSpriteActorList(wxArrayString& list, const std::string& selectedValue)
{
	int selection = -1;
	int index = 0;

	for (TActorMap::const_iterator it = m_project.ActorsBegin(), end = m_project.ActorsEnd(); it != end; ++it, ++index)
	{
		list.Add(it->second.GetName());

		if (it->second.GetName() == selectedValue)
			selection = index;
	}

	return selection;
}

int PropertyPanel::PopulateSpriteSheetList(wxArrayString& list, const Actor& actor, const std::string& selectedValue)
{
	int selection = -1;
	int index = 0;

	for (TSpriteSheetMap::const_iterator it = actor.SpriteSheetsBegin(), end = actor.SpriteSheetsEnd(); it != end; ++it, ++index)
	{
		list.Add(it->second.GetName());

		if (it->second.GetName() == selectedValue)
			selection = index;
	}

	return selection;
}

int PropertyPanel::PopulateSpriteAnimList(wxArrayString& list, const Actor& actor, SpriteSheetId spriteSheetId, const std::string& selectedValue)
{
	int selection = -1;
	int index = 0;

	if (const SpriteSheet* spriteSheet = actor.GetSpriteSheet(spriteSheetId))
	{
		for (TSpriteAnimMap::const_iterator it = spriteSheet->AnimationsBegin(), end = spriteSheet->AnimationsEnd(); it != end; ++it, ++index)
		{
			list.Add(it->second.GetName());

			if (it->second.GetName() == selectedValue)
				selection = index;
		}
	}

	return selection;
}

int PropertyPanel::PopulateGameObjectTypeList(wxArrayString& list, const std::string& selectedValue)
{
	int selection = -1;
	int index = 0;

	for (TGameObjectTypeMap::const_iterator it = m_project.GetGameObjectTypes().begin(), end = m_project.GetGameObjectTypes().end(); it != end; ++it, ++index)
	{
		list.Add(it->second.GetName());

		if (it->second.GetName() == selectedValue)
			selection = index;
	}

	return selection;
}

int PropertyPanel::PopulateArchetypeList(wxArrayString& list, GameObjectTypeId gameObjectTypeId, const std::string& selectedValue)
{
	int selection = -1;
	int index = 0;

	if (const GameObjectType* gameObjectType = m_project.GetGameObjectType(gameObjectTypeId))
	{
		for (TGameObjectArchetypeMap::const_iterator it = gameObjectType->GetArchetypes().begin(), end = gameObjectType->GetArchetypes().end(); it != end; ++it, ++index)
		{
			list.Add(it->second.GetName());

			if (it->second.GetName() == selectedValue)
				selection = index;
		}
	}

	return selection;
}
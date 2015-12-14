///////////////////////////////////////////////////////
// Beehive: A complete SEGA Mega Drive content tool
//
// (c) 2015 Matt Phillips, Big Evil Corporation
///////////////////////////////////////////////////////

#pragma once

#include <ion/maths/Vector.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>
#include <ion/renderer/Renderer.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Texture.h>

#include "../Project.h"

class RenderResources
{
public:
	enum ShaderType
	{
		eShaderFlatColour,
		eShaderFlatTextured,
		eShaderMax
	};

	enum MaterialType
	{
		eMaterialFlatColour,
		eMaterialTileset,
		eMaterialCollisionTileset,
		eMaterialMax
	};

	enum TextureType
	{
		eTextureTileset,
		eTextureCollisionTileset,
		eTextureMax
	};

	enum ColourType
	{
		eColourHighlight,
		eColourSelected,
		eColourPreview,
		eColourOutline,
		eColourGrid,
		eColourMax
	};

	enum PrimitiveType
	{
		ePrimitiveUnitQuad,
		ePrimitiveUnitLineQuad,
		ePrimitiveMax
	};

	RenderResources();
	~RenderResources();

	//Set project
	void SetProject(Project* project) { m_project = project; }

	//Create and redraw tileset texture
	void CreateTilesetTexture();

	//Create and redraw collision tileset texture
	void CreateCollisionTilesTexture();

	//Get tileset UV coords for tile
	void GetTileTexCoords(TileId tileId, ion::render::TexCoord texCoords[4], u32 flipFlags) const;

	//Get collision tileset UV coords for collision tile
	void GetCollisionTileTexCoords(CollisionTileId tileId, ion::render::TexCoord texCoords[4]) const;

	//Edit tileset texture pixel
	void SetTilesetTexPixel(TileId tileId, const ion::Vector2i& pixel, u8 colourIdx);

	//Edit collision tileset texture height
	void SetCollisionTileHeight(CollisionTileId tileId, int x, s8 height);

	//Get resources
	ion::render::Material* GetMaterial(MaterialType type) { return m_materials[type]; }
	ion::render::Shader* GetVertexShader(ShaderType type) { return m_vertexShaders[type]; }
	ion::render::Shader* GetPixelhader(ShaderType type) { return m_pixelShaders[type]; }
	ion::render::Primitive* GetPrimitive(PrimitiveType type) const { return m_primitives[type]; }
	const ion::Colour& GetColour(ColourType type) const { return m_colours[type]; }

	//Rendering util functions
	ion::Matrix4 CalcBoxMatrix(const ion::Vector2i& position, const ion::Vector2i& size, const ion::Vector2i& mapSize, float z);

private:
	
	//Map tile IDs to indices
	std::map<TileId, u32> m_tileIndexMap;

	//Tileset size sq
	u32 m_tilesetSizeSq;

	//Collision tileset size sq
	u32 m_collisionTilesetSizeSq;

	//Tileset texture cell size sq
	float m_cellSizeTexSpaceSq;

	//Collision tileset texture cell size sq
	float m_cellSizeCollisionTilesetTexSpaceSq;

	//Current project
	Project* m_project;

	//Resources
	ion::render::Shader* m_vertexShaders[eShaderMax];
	ion::render::Shader* m_pixelShaders[eShaderMax];
	ion::render::Texture* m_textures[eTextureMax];
	ion::render::Material* m_materials[eMaterialMax];
	ion::render::Primitive* m_primitives[ePrimitiveMax];
	ion::Colour m_colours[eColourMax];
};
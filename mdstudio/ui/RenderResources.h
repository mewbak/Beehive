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

#include <ion/maths/Vector.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>
#include <ion/renderer/Renderer.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Texture.h>
#include <ion/renderer/imageformats/ImageFormatBMP.h>
#include <ion/beehive/Project.h>
#include <ion/beehive/SpriteSheet.h>
#include <ion/resource/ResourceManager.h>
#include <ion/resource/ResourceHandle.h>

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
		eMaterialCollisionTypes,
		eMaterialTerrainTilesetHeight,
		eMaterialTerrainTilesetWidth,
		eMaterialSpriteSheet,
		eMaterialReferenceImage,
		eMaterialMax
	};

	enum TextureType
	{
		eTextureCollisionTypes,
		eTextureTerrainTilesetWidth,
		eTextureTerrainTilesetHeight,
		eTextureSpriteSheetPreview,
		eTextureReferenceImage,
		eTextureMax
	};

	enum ColourType
	{
		eColourHighlight,
		eColourSelected,
		eColourUnselected,
		eColourPreview,
		eColourOutline,
		eColourGridTile,
		eColourGridStamp,
		eColourDisplayFrame,
		eColourMax
	};

	enum PrimitiveType
	{
		ePrimitiveUnitQuad,
		ePrimitiveTileQuad,
		ePrimitiveTileLineQuad,
		ePrimitiveUnitLineQuad,
		ePrimitiveScreenLineQuad,
		ePrimitiveMax
	};

	RenderResources(Project& project, ion::io::ResourceManager& resourceManager);
	~RenderResources();

	//Create and redraw tileset textures
	void CreateTilesetTextures();

	//Create and redraw palette region textures
	void CreatePaletteOverlays();
	void CreatePaletteOverlay(StampSetId stampSetId, StampId stampId, OverlayId overlayId);

	//Create and redraw terrain widthmap/heightmap tileset textures
	void CreateTerrainTilesTextures();

	//Create and redraw collision texture
	void CreateCollisionTypesTexture();

	//Create spriteSheet preview texture
	void CreateSpriteSheetPreviewTexture(const ion::ImageFormat& reader);
	ion::render::Texture* CreateSpriteSheetPreviewTexture(const SpriteSheet& spriteSheet);

	//Create reference image texture
	void CreateReferenceImageTexture(const ion::ImageFormat& reader);

	//Get tileset UV coords for tile
	void GetTileTexCoords(TilesetId tilesetId, TileId tileId, ion::render::TexCoord texCoords[4], u32 flipFlags) const;
	void GetTileTexCoords(const Stamp::Overlay& overlay, int x, int y, ion::render::TexCoord texCoords[4], u32 flipFlags) const;

	//Get terrain tileset UV coords for collision type
	void GetCollisionTypeTexCoords(u16 collisionFlags, ion::render::TexCoord texCoords[4]) const;

	//Get terrain tileset UV coords for terrain tile
	void GetTerrainTileTexCoords(TerrainTileId tileId, ion::render::TexCoord texCoords[4]) const;

	//Edit tileset texture pixel
#if !BEEHIVE_PLUGIN_LUMINARY
	void SetTilesetTexPixel(TilesetId tilesetId, TileId tileId, const ion::Vector2i& pixel, u8 colourIdx);
#endif

	//Edit terrain tileset texture height
	void SetTerrainTileHeight(TerrainTileId terrainTileId, int x, s8 height);

	//Get resources
	ion::render::Material* GetMaterial(MaterialType type) { return m_materials[type]; }
	ion::render::Material* GetMaterial(TilesetId tilesetId) { return m_tilesetResources[tilesetId].material; }
	ion::render::Material* GetMaterial(StampId stampId, OverlayId overlayId) const;
	ion::render::Shader* GetShader(ShaderType type) { return m_shaders[type].Get(); }
	ion::render::Primitive* GetPrimitive(PrimitiveType type) const { return m_primitives[type]; }
	const ion::Colour& GetColour(ColourType type) const { return m_colours[type]; }

	//Rendering util functions
	ion::Matrix4 CalcBoxMatrix(const ion::Vector2i& position, const ion::Vector2i& size, const ion::Vector2i& mapSize, float z);
	ion::render::Primitive* CreateBezierPrimitive(const ion::gamekit::BezierPath& bezier);
	ion::render::Primitive* CreateBezierPointsPrimitive(const ion::gamekit::BezierPath& bezier, float handleBoxHalfExtents);
	ion::render::Primitive* CreateBezierHandlesPrimitive(const ion::gamekit::BezierPath& bezier, float handleBoxHalfExtents);
	ion::render::Primitive* CreateBezierNormalsPrimitive(const ion::gamekit::BezierPath& bezier, float lineLength, float distPerNormal);

	//SpriteSheet rendering
	class SpriteSheetRenderResources
	{
	public:
		SpriteSheetRenderResources();
		~SpriteSheetRenderResources();

		void Load(const Palette& palette, const SpriteSheet& spriteSheet, ion::io::ResourceHandle<ion::render::Shader>& shader, Project* project, ion::io::ResourceManager& resourceManager);

		struct Frame
		{
			ion::io::ResourceHandle<ion::render::Texture> texture;
			ion::io::ResourceHandle<ion::render::Material> material;
		};

		ion::render::Chessboard* m_primitive;
		std::vector<Frame> m_frames;
	};

	void CreateSpriteSheetResources(const Project& project);
	void CreateSpriteSheetResources(const Palette& palette, SpriteSheetId spriteSheetId, const SpriteSheet& spriteSheet);
	void DeleteSpriteSheetRenderResources(SpriteSheetId spriteSheetId);
	SpriteSheetRenderResources* GetSpriteSheetResources(SpriteSheetId spriteSheetId);

private:

	struct TilesetResources
	{
		std::map<TileId, u32> tileIndexMap;	//Map tile IDs to indices
		u32 tilesetSizeSq;					//Tileset size sq
		float cellSizeTexSpaceSq;			//Tileset texture cell size sq
		ion::io::ResourceHandle<ion::render::Texture> texture;
		ion::io::ResourceHandle<ion::render::Material> material;
	};

	const TilesetResources& GetTilesetResources(TilesetId tilesetId) const;
	TilesetResources& GetTilesetResources(TilesetId tilesetId);

	//terrain tileset size sq
	u32 m_terrainTilesetSizeSq;

	//terrain tileset texture cell size sq
	float m_cellSizeTerrainTilesetTexSpaceSq;

	//Current project
	Project& m_project;

	//Resource manager
	ion::io::ResourceManager& m_resourceManager;

	//Resources
	ion::io::ResourceHandle<ion::render::Shader> m_shaders[eShaderMax];
	ion::io::ResourceHandle<ion::render::Texture> m_textures[eTextureMax];
	ion::io::ResourceHandle<ion::render::Material> m_materials[eMaterialMax];
	std::map<TilesetId, TilesetResources> m_tilesetResources;
	std::map<ion::UUID64, TilesetResources> m_paletteOverlayResources;
	ion::render::Primitive* m_primitives[ePrimitiveMax];
	ion::Colour m_colours[eColourMax];

	//SpriteSheets
	std::map<SpriteSheetId, SpriteSheetRenderResources> m_spriteSheetRenderResources;
};
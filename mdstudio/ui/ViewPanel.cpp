#include "ViewPanel.h"
#include "MainWindow.h"
#include "EditStampCollisionDialog.h"

//All tools
#include "MapToolManipulatorStamp.h"

ViewPanel::ViewPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: wxGLCanvas(parent, MainWindow::GetGLAttributes(), id, pos, size, style, name)
	, m_viewport(128, 128, ion::render::Viewport::PerspectiveMode::Ortho2DAbsolute)
{
	m_mainWindow = NULL;
	m_canvasPrimitive = NULL;
	m_terrainCanvasPrimitive = NULL;
	m_collisionCanvasPrimitive = NULL;
	m_gridPrimitive = NULL;
	m_cameraZoom = 1.0f;
	m_cameraPanSpeed = 1.0f;
	m_prevMouseBits = 0;
	m_enableZoom = true;
	m_enableScroll = false;
	m_enablePan = true;
	m_canvasPrimitiveDirty = true;
	m_terrainCanvasDirty = false;
	m_collisionCanvasDirty = false;
	m_forceRefresh = false;
	m_tilesetId = InvalidTilesetId;
	m_selectedTile = InvalidTileId;
	m_hoverTile = InvalidTileId;
	m_selectableUnitSize.x = 0;
	m_selectableUnitSize.y = 0;
	m_currentTool = nullptr;
}

ViewPanel::~ViewPanel()
{
	ResetToolData();
}

void ViewPanel::SetupRendering(MainWindow* mainWindow, Project* project, ion::render::Renderer* renderer, wxGLContext* glContext, RenderResources* renderResources)
{
	m_mainWindow = mainWindow;
	m_project = project;
	m_renderer = renderer;
	m_renderResources = renderResources;
	m_glContext = glContext;
	m_forceRefresh = true;
	m_contentsInvalid = true;

	SetCurrent(*glContext);

	//Bind input events
	Bind(wxEVT_LEFT_DOWN, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_LEFT_UP, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_LEFT_DCLICK, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_MIDDLE_DOWN, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_MIDDLE_UP, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_RIGHT_DOWN, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_RIGHT_UP, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_MOTION, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_MOUSEWHEEL, &ViewPanel::EventHandlerMouse, this, GetId());
	Bind(wxEVT_KEY_DOWN, &ViewPanel::EventHandlerKeyboard, this, GetId());
	Bind(wxEVT_KEY_UP, &ViewPanel::EventHandlerKeyboard, this, GetId());
	Bind(wxEVT_PAINT, &ViewPanel::EventHandlerPaint, this, GetId());
	Bind(wxEVT_ERASE_BACKGROUND, &ViewPanel::EventHandlerErase, this, GetId());
	Bind(wxEVT_SIZE, &ViewPanel::EventHandlerResize, this, GetId());

	SetBackgroundStyle(wxBG_STYLE_PAINT);

	//Set viewport clear colour
	m_viewport.SetClearColour(ion::Colour(0.3f, 0.3f, 0.3f));

	//Centre camera on canvas
	CentreCamera();

	//Reset zoom
	SetCameraZoom(1.0f);

	wxSize clientSize = GetSize();
	m_panelSize.x = clientSize.x;
	m_panelSize.y = clientSize.y;
	m_prevPanelSize = m_panelSize;
	m_viewport.Resize(m_panelSize.x, m_panelSize.y);

	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	//Create selection quad
	m_selectionPrimitive = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2(tileWidth / 2.0f, tileHeight / 2.0f));

	Refresh();

	//Create all tools
	m_toolFactory.RegisterTool<MapToolManipulatorStamp>(*project, *this, m_undoStack);

	ResetToolData();
}

void ViewPanel::SetTilesetId(TilesetId tilesetId)
{
	m_tilesetId = tilesetId;
	m_selectedTile = InvalidTileId;
	m_hoverTile = InvalidTileId;
	InitPanel();
	ResetZoomPan();
	Refresh();
}

StampSetId ViewPanel::GetTilesetId() const
{
	if (m_tilesetId == InvalidTilesetId)
	{
		// TODO: Just get from editing map for now, but this should be cleaned up
		StampSetId stampSetId = m_project->GetEditingMap().GetStampSetId();
		const StampSet& stampSet = m_project->GetStampSet(stampSetId);
		return stampSet.GetTilesetId();
	}
	else
	{
		return m_tilesetId;
	}
}

Tileset& ViewPanel::GetTileset()
{
	return m_project->GetTileset(GetTilesetId());
}

void ViewPanel::EventHandlerMouse(wxMouseEvent& event)
{
	//Get mouse delta
	ion::Vector2i mousePosScreenSpace(event.GetX(), event.GetY());
	ion::Vector2i mouseDelta = mousePosScreenSpace - m_mousePrevPos;
	m_mousePrevPos = mousePosScreenSpace;

	OnMouse(event, mouseDelta);
	event.Skip();
}

void ViewPanel::EventHandlerKeyboard(wxKeyEvent& event)
{
	OnKeyboard(event);
	event.Skip();
}

void ViewPanel::EventHandlerPaint(wxPaintEvent& event)
{
	if(!m_mainWindow->IsRefreshLocked())
	{
		//Set GL context
		SetCurrent(*m_glContext);

		//Begin rendering to current viewport
		m_renderer->BeginFrame(m_viewport, GetHDC());
		m_renderer->ClearColour();
		m_renderer->ClearDepth();

		ion::Matrix4 cameraInverseMtx = m_camera.GetTransform().GetInverse();
		ion::Matrix4 projectionMtx = m_renderer->GetProjectionMatrix();

		//Z order
		const float zOffset = 0.0001f;
		float z = 0.0f;

		//Render callback
		OnRender(*m_renderer, cameraInverseMtx, projectionMtx, z, zOffset);

		//End rendering
		m_renderer->SwapBuffers();
		m_renderer->EndFrame();

		event.Skip();
	}
}

void ViewPanel::EventHandlerErase(wxEraseEvent& event)
{
	//Ignore event
}

void ViewPanel::EventHandlerResize(wxSizeEvent& event)
{
	OnResize(event);
	event.Skip();
}

void ViewPanel::ForceRefresh()
{
	m_forceRefresh = true;
	Refresh();
}

void ViewPanel::InitPanel()
{
	m_canvasSize = CalcCanvasSize();

	//Recreate canvas
	CreateCanvas(m_canvasSize.x, m_canvasSize.y);

	//Fill with invalid tile
	FillTiles(m_tilesetId, InvalidTileId, ion::Vector2i(0, 0), ion::Vector2i(m_canvasSize.x - 1, m_canvasSize.y - 1));

	//Recreate grid
	CreateGrid(m_canvasSize.x, m_canvasSize.y, m_canvasSize.x / m_project->GetGridSize(), m_canvasSize.y / m_project->GetGridSize());

	//Redraw contents
	PaintContents();
}

void ViewPanel::Refresh(bool eraseBackground, const wxRect *rect)
{
	if(!m_mainWindow->IsRefreshLocked())
	{
		if (m_contentsInvalid || m_project->TilesAreInvalidated())
		{
			const int tileWidth = m_project->GetPlatformConfig().tileWidth;
			const int tileHeight = m_project->GetPlatformConfig().tileHeight;

			if (m_panelSize.x > tileWidth && m_panelSize.y > tileHeight)
			{
				InitPanel();
			}

			m_contentsInvalid = false;
		}

		wxGLCanvas::Refresh(eraseBackground, rect);

		if (m_canvasPrimitive && m_canvasPrimitiveDirty)
		{
			m_canvasPrimitive->GetVertexBuffer().CommitBuffer();
			m_canvasPrimitiveDirty = false;
		}

		if (m_terrainCanvasPrimitive && m_terrainCanvasDirty)
		{
			m_terrainCanvasPrimitive->GetVertexBuffer().CommitBuffer();
			m_terrainCanvasDirty = false;
		}

		if (m_collisionCanvasPrimitive && m_collisionCanvasDirty)
		{
			m_collisionCanvasPrimitive->GetVertexBuffer().CommitBuffer();
			m_collisionCanvasDirty = false;
		}

		m_forceRefresh = false;
	}
}

void ViewPanel::CreateCanvas(int width, int height)
{
	//Create rendering primitive
	if(m_canvasPrimitive)
		delete m_canvasPrimitive;

	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	m_canvasPrimitive = new ion::render::Chessboard(ion::render::Chessboard::Axis::xy, ion::Vector2((float)width * (tileWidth / 2.0f), (float)height * (tileHeight / 2.0f)), width, height, true);
	m_canvasSize.x = width;
	m_canvasSize.y = height;
	m_canvasPrimitiveDirty = true;
}

void ViewPanel::CreateCollisionCanvas(int width, int height)
{
	//Create rendering primitives
	if (m_terrainCanvasPrimitive)
		delete m_terrainCanvasPrimitive;

	if (m_collisionCanvasPrimitive)
		delete m_collisionCanvasPrimitive;

	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	m_terrainCanvasPrimitive = new ion::render::Chessboard(ion::render::Chessboard::Axis::xy, ion::Vector2((float)(width * tileWidth) / 2.0f, (float)(height * tileHeight) / 2.0f), width, height, true);
	m_collisionCanvasPrimitive = new ion::render::Chessboard(ion::render::Chessboard::Axis::xy, ion::Vector2((float)(width * tileWidth) / 2.0f, (float)(height * tileHeight) / 2.0f), width, height, true);

	m_terrainCanvasDirty = true;
	m_collisionCanvasDirty = true;
}

void ViewPanel::CreateGrid(int width, int height, int cellsX, int cellsY)
{
	if(m_gridPrimitive)
		delete m_gridPrimitive;

	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	m_gridPrimitive = new ion::render::Grid(ion::render::Grid::Axis::xy, ion::Vector2((float)width * (tileWidth / 2.0f), (float)height * (tileHeight / 2.0f)), cellsX, cellsY);
}

ion::Vector2i ViewPanel::CalcCanvasSize()
{
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	wxSize panelSize = GetClientSize();
	const Tileset& tileset = m_project->GetTileset(GetTilesetId());
	int numTiles = tileset.GetCount();
	int numCols = ion::maths::Ceil((float)panelSize.x / 8.0f / tileWidth);
	int numRows = ion::maths::Max(numCols, (int)ion::maths::Ceil((float)numTiles / (float)numCols));
	return ion::Vector2i(numCols, numRows);
}

void ViewPanel::PaintContents()
{
	TilesetId tilesetId = GetTilesetId();
	const Tileset& tileset = m_project->GetTileset(tilesetId);

	int i = 0;
	for (const auto& it : tileset.GetTiles())
	{
		int x = ion::maths::Max(0, i % m_canvasSize.x);
		int y = ion::maths::Max(0, m_canvasSize.y - 1 - (i / m_canvasSize.x));
		i++;

		PaintTile(tilesetId, it.first, x, y, 0);
	}
}

void ViewPanel::PaintTile(TilesetId tilesetId, TileId tileId, int x, int y, u32 flipFlags)
{
	ion::render::TexCoord coords[4];

	//Set texture coords for cell
	if (tileId != InvalidTileId)
		m_renderResources->GetTileTexCoords(tilesetId, tileId, coords, flipFlags);

	m_canvasPrimitive->SetTexCoords((y * m_canvasSize.x) + x, coords);
	m_canvasPrimitiveDirty = true;
}

void ViewPanel::PaintCollisionTile(TerrainTileId terrainTileId, int x, int y, u16 collisionFlags)
{
	ion::render::TexCoord coords[4] = { ion::Vector2(0.0f, 0.0f) };

	//Set texture coords for terrain cell
	if (terrainTileId != InvalidTerrainTileId)
		m_renderResources->GetTerrainTileTexCoords(terrainTileId, coords);

	m_terrainCanvasPrimitive->SetTexCoords((y * m_canvasSize.x) + x, coords);
	m_terrainCanvasDirty = true;

	//Set texture coords for collision cell
	if (terrainTileId != InvalidTerrainTileId)
		m_renderResources->GetCollisionTypeTexCoords(collisionFlags, coords);

	m_collisionCanvasPrimitive->SetTexCoords((y * m_canvasSize.x) + x, coords);
	m_collisionCanvasDirty = true;
}

void ViewPanel::PaintStamp(TilesetId tilesetId, const Stamp& stamp, int x, int y, u32 flipFlags)
{
	//Paint stamp overlays first
	for (const auto& overlay : stamp.GetOverlays())
	{
		PaintPaletteOverlay(stamp, overlay.first);
	}

	for (const auto& anim : stamp.GetStampAnims())
	{
		PaintAnimationOverlay(stamp, anim.first);
	}

	int width = stamp.GetWidth();
	int height = stamp.GetHeight();

	for(int stampX = 0; stampX < width; stampX++)
	{
		for(int stampY = 0; stampY < height; stampY++)
		{
			int sourceX = (flipFlags & Map::eFlipX) ? (width - 1 - stampX) : stampX;
			int sourceY = (flipFlags & Map::eFlipY) ? (height - 1 - stampY) : stampY;

			TileId tileId = stamp.GetTile(sourceX, sourceY);
			u32 tileFlags = stamp.GetTileFlags(sourceX, sourceY);
			tileFlags ^= flipFlags;
			int canvasX = stampX + x;
			int canvasY = stampY + y;
			int y_inv = m_canvasSize.y - 1 - canvasY;

			//Can place stamps outside canvas boundaries, only draw tiles that are inside
			if(canvasX >= 0 && canvasX < m_canvasSize.x && y_inv >= 0 && y_inv < m_canvasSize.y)
			{
				//Paint on canvas
				PaintTile(tilesetId, tileId, canvasX, y_inv, tileFlags);
			}
		}
	}
}

void ViewPanel::PaintPaletteOverlay(const Stamp& stamp, OverlayId overlayId)
{
	auto& it = m_primitiveStampOverlays.find(overlayId);
	if (it != m_primitiveStampOverlays.end())
	{
		delete it->second;
		m_primitiveStampOverlays.erase(it);
	}

	const Stamp::Overlay& overlay = stamp.GetOverlay(overlayId);
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;
	const int width = overlay.bottomRight.x - overlay.topLeft.x + 1;
	const int height = overlay.bottomRight.y - overlay.topLeft.y + 1;

	ion::render::Chessboard* primitive = new ion::render::Chessboard(ion::render::Chessboard::Axis::xy, ion::Vector2((float)width * (tileWidth / 2.0f), (float)height * (tileHeight / 2.0f)), width, height, true);

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			int tileX = overlay.topLeft.x + x;
			int tileY = overlay.topLeft.y + y;
			u32 tileFlags = stamp.GetTileFlags(tileX, tileY);
			int y_inv = height - 1 - y;

			//Set texture coords for cell
			ion::render::TexCoord coords[4];
			m_renderResources->GetTileTexCoords(overlayId, x, y, coords, tileFlags);
			primitive->SetTexCoords((y_inv * width) + x, coords);
		}
	}

	primitive->GetVertexBuffer().CommitBuffer();
	m_primitiveStampOverlays.insert(std::make_pair(overlayId, primitive));
}

void ViewPanel::PaintAnimationOverlay(const Stamp& stamp, StampAnimId animId)
{
	const Stamp::StampAnim& overlay = stamp.GetStampAnim(animId);

	auto& it = m_primitiveStampOverlays.find(overlay.spriteAnimId);
	if (it != m_primitiveStampOverlays.end())
	{
		delete it->second;
		m_primitiveStampOverlays.erase(it);
	}

	const Actor* actor = m_project->GetActor(overlay.actorId);
	const SpriteSheet* sheet = actor->GetSpriteSheet(overlay.spriteSheetId);

	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;
	const int width = sheet->GetWidthTiles();
	const int height = sheet->GetHeightTiles();

	ion::render::Chessboard* primitive = new ion::render::Chessboard(ion::render::Chessboard::Axis::xy, ion::Vector2((float)width * (tileWidth / 2.0f), (float)height * (tileHeight / 2.0f)), width, height, true);

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			int tileX = overlay.position.x + x;
			int tileY = overlay.position.y + y;
			int y_inv = height - 1 - y;

			//Set texture coords for cell
			ion::render::TexCoord coords[4];
			m_renderResources->GetTileTexCoords(overlay.spriteAnimId, x, y, coords, 0);
			primitive->SetTexCoords((y_inv * width) + x, coords);
		}
	}

	primitive->GetVertexBuffer().CommitBuffer();
	m_primitiveStampOverlays.insert(std::make_pair(overlay.spriteAnimId, primitive));
}

void ViewPanel::PaintStampCollision(const Stamp& stamp, int x, int y, u32 flipFlags)
{
	int width = stamp.GetWidth();
	int height = stamp.GetHeight();

	for (int stampX = 0; stampX < width; stampX++)
	{
		for (int stampY = 0; stampY < height; stampY++)
		{
			int sourceX = (flipFlags & Map::eFlipX) ? (width - 1 - stampX) : stampX;
			int sourceY = (flipFlags & Map::eFlipY) ? (height - 1 - stampY) : stampY;

			TerrainTileId tileId = stamp.GetTerrainTile(sourceX, sourceY);
			u16 tileFlags = stamp.GetCollisionTileFlags(sourceX, sourceY);
			tileFlags ^= flipFlags;
			int canvasX = stampX + x;
			int canvasY = stampY + y;
			int y_inv = m_canvasSize.y - 1 - canvasY;

			//Can place stamps outside canvas boundaries, only draw tiles that are inside
			if (canvasX >= 0 && canvasX < m_canvasSize.x && y_inv >= 0 && y_inv < m_canvasSize.y)
			{
				//Paint on canvas
				PaintCollisionTile(tileId, canvasX, y_inv, tileFlags);
			}
		}
	}
}

void ViewPanel::FillTiles(TilesetId tilesetId, TileId tileId, const ion::Vector2i& boxCorner1, const ion::Vector2i& boxCorner2)
{
	//Sanitise ordering before looping
	int top = ion::maths::Min(boxCorner1.y, boxCorner2.y);
	int left = ion::maths::Min(boxCorner1.x, boxCorner2.x);
	int bottom = ion::maths::Max(boxCorner1.y, boxCorner2.y);
	int right = ion::maths::Max(boxCorner1.x, boxCorner2.x);

	for(int x = left; x <= right; x++)
	{
		for(int y = top; y <= bottom; y++)
		{
			//Invert for OpenGL
			int y_inv = m_canvasSize.y - 1 - y;

			//Paint tile to canvas
			PaintTile(tilesetId, tileId, x, y_inv, 0);
		}
	}
}

void ViewPanel::FillTiles(TilesetId tilesetId, TileId tileId, const std::vector<ion::Vector2i>& selection)
{
	for(int i = 0; i < selection.size(); i++)
	{
		int x = selection[i].x;
		int y = selection[i].y;

		//Invert for OpenGL
		int y_inv = m_canvasSize.y - 1 - y;

		//Paint tile to canvas
		PaintTile(tilesetId, tileId, x, y_inv, 0);
	}
}

void ViewPanel::SetTool(ToolType tool)
{
	m_currentTool = m_toolFactory.GetTool(tool);
	if (m_currentTool)
	{
		m_mainWindow->SetStatusText(m_currentTool->GetStatusText());
	}
}

void ViewPanel::FindBounds(const std::vector<ion::Vector2i>& tiles, int& left, int& top, int& right, int& bottom) const
{
	left = INT_MAX;
	top = INT_MAX;
	right = 0;
	bottom = 0;

	for(int i = 0; i < tiles.size(); i++)
	{
		int x = tiles[i].x;
		int y = tiles[i].y;

		if(x < left)
			left = x;
		if(x > right)
			right = x;
		if(y < top)
			top = y;
		if(y > bottom)
			bottom = y;
	}
}

ion::Vector2 ViewPanel::GetCameraPos() const
{
	float prevZoom = m_cameraZoom;

	//Reset camera zoom
	ion::render::Camera camera = m_camera;
	camera.SetZoom(ion::Vector3(1.0f, 1.0f, 1.0f));

	//Compensate camera pos
	ion::Vector2 originalViewportSize((float)m_panelSize.x / prevZoom, (float)m_panelSize.y / prevZoom);
	ion::Vector2 newViewportSize((float)m_panelSize.x, (float)m_panelSize.y);
	ion::Vector3 cameraPos = camera.GetPosition();
	cameraPos.x -= (newViewportSize.x - originalViewportSize.x) / 2.0f;
	cameraPos.y -= (newViewportSize.y - originalViewportSize.y) / 2.0f;

	return ion::Vector2(cameraPos.x, cameraPos.y);
}

float ViewPanel::GetCameraZoom() const
{
	return m_cameraZoom;
}

void ViewPanel::OnMouse(wxMouseEvent& event, const ion::Vector2i& mouseDelta)
{
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	//Get mouse position in panel space
	wxClientDC clientDc(this);
	wxPoint mousePanelPosWx = event.GetLogicalPosition(clientDc);

	//Centre of map quad is 0,0
	ion::Vector2 mousePos(mousePanelPosWx.x, m_panelSize.y - mousePanelPosWx.y);
	ion::Vector2 viewportSize(m_panelSize.x, m_panelSize.y);
	ion::Vector2 cameraPos(m_camera.GetPosition().x * m_cameraZoom, m_camera.GetPosition().y * m_cameraZoom);
	ion::Vector2 canvasSizePixels(m_canvasSize.x * tileWidth * m_cameraZoom, m_canvasSize.y * tileHeight * m_cameraZoom);
	ion::Vector2 mousePosCanvasSpace;
	mousePosCanvasSpace.x = (canvasSizePixels.x - (canvasSizePixels.x / 2.0f - cameraPos.x - mousePos.x)) / m_cameraZoom;
	mousePosCanvasSpace.y = (canvasSizePixels.y - (canvasSizePixels.y / 2.0f - cameraPos.y - mousePos.y)) / m_cameraZoom;

	//Get pixel x/y
	int mousePixelY_inv = (m_canvasSize.y * tileHeight) - (int)floor(mousePosCanvasSpace.y) - 1;
	ion::Vector2i mousePixelPosCanvas((int)floor(mousePosCanvasSpace.x), mousePixelY_inv);

	//Get tile x/y
	int x = (int)floor(mousePosCanvasSpace.x / (float)tileWidth);
	int y_inv = (int)floor(mousePosCanvasSpace.y / (float)tileHeight);

	//Invert for OpenGL
	int y = (m_canvasSize.y - 1 - y_inv);

	//Get button bits
	int buttonBits = 0;
	if(event.LeftIsDown())
		buttonBits |= eMouseLeft;
	if(event.MiddleIsDown())
		buttonBits |= eMouseMiddle;
	if(event.RightIsDown())
		buttonBits |= eMouseRight;

	ion::Vector2i tileDelta(x - m_prevMouseOverTilePos.x, y - m_prevMouseOverTilePos.y);

	if((buttonBits != m_prevMouseBits) || (x != m_prevMouseOverTilePos.x) || (y != m_prevMouseOverTilePos.y))
	{
		//Mouse button clicked or changed grid pos
		OnMouseTileEvent(mousePixelPosCanvas, mouseDelta, tileDelta, buttonBits, x, y);

		m_prevMouseOverTilePos.x = x;
		m_prevMouseOverTilePos.y = y;
	}

	if((buttonBits != m_prevMouseBits) || (mousePixelPosCanvas.x != m_prevMouseOverPixelPos.x) || (mousePixelPosCanvas.y != m_prevMouseOverPixelPos.y))
	{
		//Mouse button clicked or changed pixel pos
		OnMousePixelEvent(mousePixelPosCanvas, mouseDelta, tileDelta, buttonBits, x, y);
		m_prevMouseOverPixelPos = mousePixelPosCanvas;
	}

	m_prevMouseBits = buttonBits;

	//Camera pan/zoom
	float zoomDelta = 0.0f;

	if(event.Dragging() && event.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
	{
		if(event.ShiftDown())
		{
			//SHIFT + middle mouse + drag = zoom
			float zoomSpeed = 0.05f;
			zoomDelta -= mouseDelta.y * zoomSpeed;
		}
		else
		{
			if(m_enablePan)
			{
				//Middle mouse + drag = pan
				ion::Vector3 pos = m_camera.GetPosition();
				pos.x -= mouseDelta.x * m_cameraPanSpeed / m_cameraZoom;
				pos.y += mouseDelta.y * m_cameraPanSpeed / m_cameraZoom;
				m_camera.SetPosition(pos);

				//Invalidate rect
				Refresh();
			}
		}
	}
	else if(event.GetWheelRotation() != 0)
	{
		int wheelDelta = event.GetWheelRotation();

		if (m_enableZoom)
		{
			//Zoom camera
			float zoomSpeed = 1.0f;

			//Reduce speed for <1.0f
			if ((wheelDelta < 0 && m_cameraZoom <= 1.0f) || (wheelDelta > 0 && m_cameraZoom < 1.0f))
			{
				zoomSpeed = 0.2f;
			}

			//One notch at a time
			if (wheelDelta > 0)
				zoomDelta = zoomSpeed;
			else if (wheelDelta < 0)
				zoomDelta = -zoomSpeed;

			if (zoomDelta != 0.0f)
			{
				float zoom = m_cameraZoom + zoomDelta;

				//Clamp
				if (zoom < 0.2f)
					zoom = 0.2f;
				else if (zoom > 10.0f)
					zoom = 10.0f;

				//Set camera zoom
				SetCameraZoom(zoom);

				//Invalidate rect
				Refresh();
			}
		}
		else if (m_enableScroll)
		{
			if (wheelDelta != 0.0f)
			{
				ion::Vector3 cameraPos = m_camera.GetPosition();
				cameraPos.y += wheelDelta * m_cameraPanSpeed / m_cameraZoom;

				//Clamp to size
				float halfCanvas = (m_canvasSize.y * (tileHeight / 2.0f));
				float minY = -halfCanvas;
				float maxY = halfCanvas - ((float)m_panelSize.y / m_cameraZoom);

				if (cameraPos.y > maxY)
					cameraPos.y = maxY;
				else if (cameraPos.y < minY)
					cameraPos.y = minY;

				m_camera.SetPosition(cameraPos);

				Refresh();
			}
		}
	}

	/*
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	//Camera pan Y (if canvas is taller than panel)
	if ((m_canvasSize.y * tileHeight) > (m_panelSize.y / m_cameraZoom))
	{
		float panDeltaY = 0.0f;

		if (event.Dragging() && event.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
		{
			panDeltaY = mouseDelta.y;
		}
		else if (event.GetWheelRotation() != 0)
		{
			panDeltaY = (float)event.GetWheelRotation();
		}

		if (panDeltaY != 0.0f)
		{
			ion::Vector3 cameraPos = m_camera.GetPosition();
			cameraPos.y += panDeltaY * m_cameraPanSpeed / m_cameraZoom;

			//Clamp to size
			float halfCanvas = (m_canvasSize.y * (tileHeight / 2.0f));
			float minY = -halfCanvas;
			float maxY = halfCanvas - ((float)m_panelSize.y / m_cameraZoom);

			if (cameraPos.y > maxY)
				cameraPos.y = maxY;
			else if (cameraPos.y < minY)
				cameraPos.y = minY;

			m_camera.SetPosition(cameraPos);

			Refresh();
		}
	}
	*/
}

void ViewPanel::OnKeyboard(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		ResetToolData();
		m_currentTool = nullptr;
		Refresh();
	}

	//The new way of doing things
	if (m_currentTool)
	{
		m_currentTool->OnKeyboard(event);
	}
}

void ViewPanel::OnMouseTileEvent(ion::Vector2i mousePos, ion::Vector2i mouseDelta, ion::Vector2i tileDelta, int buttonBits, int x, int y)
{
	if (m_currentTool)
	{
		m_currentTool->OnMouseTileEvent(mousePos, mouseDelta, tileDelta, buttonBits, x, y);
		if (m_currentTool->NeedsRedraw())
			Refresh();
		return;
	}
	else
	{
		TileId selectedTile = InvalidTileId;

		//If in range, get tile under mouse cursor
		if (x >= 0 && y >= 0 && x < m_canvasSize.x && y < m_canvasSize.y)
		{
			// TODO: tile ids are no longer indices, maintain an index map in Tileset
			//selectedTile = (y * m_canvasSize.x) + x;
		}

		//Set mouse hover tile
		m_hoverTile = selectedTile;
		m_hoverTilePos.x = x;
		m_hoverTilePos.y = y;

		if ((buttonBits & eMouseLeft) && !(m_prevMouseBits & eMouseLeft))
		{
			//Left click, set current tile
			m_selectedTile = selectedTile;
			m_selectedTilePos = m_hoverTilePos;

			//Set as current painting tile
			m_project->SetPaintTile(selectedTile);
		}

		//Redraw
		Refresh();
	}
}

void ViewPanel::OnMousePixelEvent(ion::Vector2i mousePos, ion::Vector2i mouseDelta, ion::Vector2i tileDelta, int buttonBits, int tileX, int tileY)
{
	if (m_currentTool)
	{
		m_currentTool->OnMousePixelEvent(mousePos, mouseDelta, tileDelta, buttonBits, tileX, tileY);
		if (m_currentTool->NeedsRedraw())
			Refresh();
	}
}

void ViewPanel::OnRender(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float& z, float zOffset)
{
	TilesetId tilesetId = GetTilesetId();

	//Render canvas
	RenderCanvas(renderer, cameraInverseMtx, projectionMtx, z, tilesetId);

	z += zOffset;

	//Render selected tile
	if (m_selectedTile != InvalidTileId)
	{
		ion::Vector2 size(1, 1);
		const ion::Colour& colour = m_renderResources->GetColour(RenderResources::eColourSelected);
		RenderBox(m_selectedTilePos, size, colour, renderer, cameraInverseMtx, projectionMtx, z);
	}

	z += zOffset;

	//Render mouse hover tile
	if (m_hoverTile != InvalidTileId && m_hoverTile != m_selectedTile)
	{
		ion::Vector2 size(1, 1);
		const ion::Colour& colour = m_renderResources->GetColour(RenderResources::eColourHighlight);
		RenderBox(m_hoverTilePos, size, colour, renderer, cameraInverseMtx, projectionMtx, z);
	}

	z += zOffset;

	//Render grid
	if (m_project->GetShowGrid())
	{
		RenderGrid(renderer, cameraInverseMtx, projectionMtx, z);
	}

	z += zOffset;

	//Render currently active tool
	if (m_currentTool)
	{
		m_currentTool->OnRender(renderer, *m_renderResources, cameraInverseMtx, projectionMtx, z, zOffset);
		z += zOffset;
	}
}

void ViewPanel::RenderCanvas(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float z, TilesetId tilesetId)
{
	if(m_canvasPrimitive)
	{
		//No depth test (stops grid cells Z fighting)
		renderer.SetDepthTest(ion::render::Renderer::DepthTest::Always);

		ion::render::Material* material = m_renderResources->GetMaterial(tilesetId);

		//Draw map
		material->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f, 1.0f));
		renderer.BindMaterial(*material, ion::Matrix4(), cameraInverseMtx, projectionMtx);
		renderer.DrawVertexBuffer(m_canvasPrimitive->GetVertexBuffer(), m_canvasPrimitive->GetIndexBuffer());
		renderer.UnbindMaterial(*material);

		renderer.SetDepthTest(ion::render::Renderer::DepthTest::LessOrEqual);
	}
}

void ViewPanel::RenderGrid(ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float z)
{
	//Draw grid
	ion::render::Material* material = m_renderResources->GetMaterial(RenderResources::eMaterialFlatColour);
	const ion::Colour& colour = m_renderResources->GetColour(RenderResources::eColourGridTile);

	ion::Matrix4 gridMtx;
	gridMtx.SetTranslation(ion::Vector3(0.0f, 0.0f, z));
	gridMtx.SetScale(ion::Vector3((float)m_project->GetGridSize(), (float)m_project->GetGridSize(), 1.0f));
	material->SetDiffuseColour(colour);
	renderer.BindMaterial(*material, gridMtx, cameraInverseMtx, projectionMtx);
	renderer.DrawVertexBuffer(m_gridPrimitive->GetVertexBuffer());
	renderer.UnbindMaterial(*material);
}

void ViewPanel::RenderBox(const ion::Vector2i& pos, const ion::Vector2& size, const ion::Colour& colour, ion::render::Renderer& renderer, const ion::Matrix4& cameraInverseMtx, const ion::Matrix4& projectionMtx, float z)
{
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	float bottom = (m_canvasSize.y - (pos.y + size.y));

	ion::Matrix4 boxMtx;
	ion::Vector3 boxScale(size.x, size.y, 0.0f);
	ion::Vector3 boxPos(floor((pos.x - (m_canvasSize.x / 2.0f) + (boxScale.x / 2.0f)) * tileWidth),
		floor((bottom - (m_canvasSize.y / 2.0f) + (boxScale.y / 2.0f)) * tileHeight), z);

	boxMtx.SetTranslation(boxPos);
	boxMtx.SetScale(boxScale);

	ion::render::Material* material = m_renderResources->GetMaterial(RenderResources::eMaterialFlatColour);

	renderer.SetAlphaBlending(ion::render::Renderer::AlphaBlendType::Translucent);
	material->SetDiffuseColour(colour);
	renderer.BindMaterial(*material, boxMtx, cameraInverseMtx, projectionMtx);
	renderer.DrawVertexBuffer(m_selectionPrimitive->GetVertexBuffer(), m_selectionPrimitive->GetIndexBuffer());
	renderer.UnbindMaterial(*material);
	renderer.SetAlphaBlending(ion::render::Renderer::AlphaBlendType::None);
}

void ViewPanel::OnResize(wxSizeEvent& event)
{
	if(!m_mainWindow->IsRefreshLocked())
	{
		wxSize clientSize = event.GetSize();

		if (clientSize.x != m_panelSize.x || clientSize.y != m_panelSize.y)
		{
			m_panelSize.x = clientSize.x;
			m_panelSize.y = clientSize.y;

			if (m_panelSize.x > m_selectableUnitSize.x && m_panelSize.y > m_selectableUnitSize.y)
			{
				ion::Vector2i canvasSize = CalcCanvasSize();

				if (canvasSize.x != m_canvasSize.x || canvasSize.y != m_canvasSize.y)
				{
					InitPanel();
				}

				ResetZoomPan();

				//Filter out superflous resize events (wx sends them if UI thread doesn't respond during saving/loading)
				if (m_prevPanelSize.x != m_panelSize.x || m_prevPanelSize.y != m_panelSize.y)
				{
					m_prevPanelSize = m_panelSize;
					m_viewport.Resize(m_panelSize.x, m_panelSize.y);
				}
			}

			Refresh();
		}
	}
}

void ViewPanel::CentreCamera()
{
	//Reset zoom to identity
	float zoom = m_cameraZoom;
	SetCameraZoom(1.0f);

	//Centre camera
	ion::Vector3 cameraPos(-(m_panelSize.x / 2.0f), -(m_panelSize.y / 2.0f), 0.0f);
	m_camera.SetPosition(cameraPos);

	//Re-apply zoom
	SetCameraZoom(zoom);
}

void ViewPanel::SetCameraZoom(float zoom)
{
	float prevZoom = m_cameraZoom;

	//Set camera zoom
	m_camera.SetZoom(ion::Vector3(zoom, zoom, 1.0f));

	//Compensate camera pos
	ion::Vector2 originalViewportSize((float)m_panelSize.x / prevZoom, (float)m_panelSize.y / prevZoom);
	ion::Vector2 newViewportSize((float)m_panelSize.x / zoom, (float)m_panelSize.y / zoom);
	ion::Vector3 cameraPos = m_camera.GetPosition();
	cameraPos.x -= (newViewportSize.x - originalViewportSize.x) / 2.0f;
	cameraPos.y -= (newViewportSize.y - originalViewportSize.y) / 2.0f;
	m_camera.SetPosition(cameraPos);

	m_cameraZoom = zoom;
}

void ViewPanel::ResetZoomPan()
{
	const int tileWidth = m_project->GetPlatformConfig().tileWidth;
	m_cameraZoom = (float)m_panelSize.x / (m_canvasSize.x * tileWidth);
	ion::Vector3 cameraPos(-(m_panelSize.x / 2.0f / m_cameraZoom), -(m_panelSize.y / 2.0f / m_cameraZoom), 0.0f);

	m_camera.SetZoom(ion::Vector3(m_cameraZoom, m_cameraZoom, 1.0f));
	m_camera.SetPosition(cameraPos);
}

void ViewPanel::ScrollToTop()
{
	//Reset zoom to identity
	//float zoom = m_cameraZoom;
	//SetCameraZoom(1.0f);

	const int tileHeight = m_project->GetPlatformConfig().tileHeight;

	//Scroll to top
	float halfCanvas = (m_canvasSize.y * (tileHeight / 2.0f));
	float maxY = halfCanvas - ((float)m_panelSize.y / m_cameraZoom);
	ion::Vector3 cameraPos(-(m_panelSize.x / 2.0f / m_cameraZoom), maxY, 0.0f);

	//Re-apply zoom
	//SetCameraZoom(zoom);
}

void ViewPanel::EditStampCollisionDlg(StampId stampId)
{
	StampSetId stampSetId = m_project->GetEditingMap().GetStampSetId();
	DialogEditStamp dialog(*m_mainWindow, stampSetId, stampId, *m_project, *m_renderer, *m_glContext, *m_renderResources);
	dialog.ShowModal();
}
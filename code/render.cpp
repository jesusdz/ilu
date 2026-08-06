static constexpr float4 ColorWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr float4 ColorBlack = { 0.0f, 0.0f, 0.0f, 0.0f };
static constexpr float4 ColorRed = { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr float4 ColorGreen = { 0.0f, 1.0f, 0.0f, 1.0f };
static constexpr float4 ColorBlue = { 0.0f, 0.0f, 1.0f, 1.0f };
static constexpr float4 ColorOrange = { 1.0f, 0.5f, 0.0f, 1.0f };


////////////////////////////////////////////////////////////////////////
// Immediate draw

static void DebugDrawAppendBatch(Graphics &gfx, ImageH imageH, u32 vertexCount)
{
	if ( gfx.debugDrawBatchCount > 0 )
	{
		DebugDrawBatch &lastBatch = gfx.debugDrawBatches[gfx.debugDrawBatchCount - 1];
		if ( lastBatch.imageH.index == imageH.index )
		{
			lastBatch.vertexCount += vertexCount;
			return;
		}
	}

	ASSERT( gfx.debugDrawBatchCount < MAX_DEBUG_DRAW_BATCHES );
	DebugDrawBatch &batch = gfx.debugDrawBatches[gfx.debugDrawBatchCount++];
	batch.imageH = imageH;
	batch.vertexIndex = gfx.debugDrawVertexCount - vertexCount;
	batch.vertexCount = vertexCount;
}

void DrawSprite(ID spriteId, float2 worldPos, float4 pcolor)
{
	Graphics &gfx = engine->gfx;

	ASSERT( gfx.debugDrawVertexCount + 6 <= MAX_DEBUG_DRAW_VERTICES );

	const SpriteDesc &sprite = GetSprite(spriteId).desc;
	const Texture &texture = GetTexture(sprite.textureId);

	const float2 uvPos  = { (f32)sprite.pos.x / texture.size.x, (f32)sprite.pos.y / texture.size.y };
	const float2 uvSize = { (f32)sprite.size.x / texture.size.x, (f32)sprite.size.y / texture.size.y };
	const float2 worldSize = float2{ (f32)sprite.size.x, (f32)sprite.size.y } / PIXELS_PER_METER;
	const rgba color = Rgba(pcolor);

	DebugDrawVertex *v = gfx.debugDrawVerticesCPU + gfx.debugDrawVertexCount;
	v[0] = DebugDrawVertex{ worldPos + float2{0, 0},                     uvPos + float2{0, uvSize.y}, color };
	v[1] = DebugDrawVertex{ worldPos + float2{worldSize.x, worldSize.y}, uvPos + float2{uvSize.x, 0}, color };
	v[2] = DebugDrawVertex{ worldPos + float2{0, worldSize.y},           uvPos,                       color };
	v[3] = DebugDrawVertex{ worldPos + float2{0, 0},                     uvPos + float2{0, uvSize.y}, color };
	v[4] = DebugDrawVertex{ worldPos + float2{worldSize.x, 0},           uvPos + uvSize,               color };
	v[5] = DebugDrawVertex{ worldPos + float2{worldSize.x, worldSize.y}, uvPos + float2{uvSize.x, 0}, color };
	gfx.debugDrawVertexCount += 6;

	const ImageH imageH = GetTextureImage(gfx, sprite.textureId, gfx.pinkImageH);
	DebugDrawAppendBatch(gfx, imageH, 6);
}


////////////////////////////////////////////////////////////////////////
// Debug draw

void DrawBox(float2 pos, float2 size, float4 color)
{
	Graphics &gfx = engine->gfx;

	ASSERT( gfx.debugDrawVertexCount + 6 <= MAX_DEBUG_DRAW_VERTICES );
	const rgba c = Rgba(color);
	const float2 uv = {}; // Any UV works here, gfx.whiteImageH is a single white pixel.
	DebugDrawVertex *v = gfx.debugDrawVerticesCPU + gfx.debugDrawVertexCount;
	v[0] = DebugDrawVertex{ pos + float2{0, 0}, uv, c };
	v[1] = DebugDrawVertex{ pos + float2{size.x, size.y}, uv, c };
	v[2] = DebugDrawVertex{ pos + float2{0, size.y}, uv, c };
	v[3] = DebugDrawVertex{ pos + float2{0, 0}, uv, c };
	v[4] = DebugDrawVertex{ pos + float2{size.x, 0}, uv, c };
	v[5] = DebugDrawVertex{ pos + float2{size.x, size.y}, uv, c };
	gfx.debugDrawVertexCount += 6;

	DebugDrawAppendBatch(gfx, gfx.whiteImageH, 6);
}

void DrawBoxOutline(float2 pos, float2 size, float4 color)
{
	constexpr f32 d = 1.0f / PIXELS_PER_METER;
	DrawBox(pos, float2{size.x, d}, color);
	DrawBox(pos, float2{d, size.y}, color);
	DrawBox(pos + dX(size) - float2{d, 0}, float2{d, size.y}, color);
	DrawBox(pos + dY(size) - float2{0, d}, float2{size.x, d}, color);
}


float3 UpDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const f32 pitch = angles.y;
	const float3 up = { Sin(yaw)*Sin(pitch), Cos(pitch), Cos(yaw)*Sin(pitch) };
	return up;
}

float3 ForwardDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const f32 pitch = angles.y;
	const float3 forward = { -Sin(yaw)*Cos(pitch), Sin(pitch), -Cos(yaw)*Cos(pitch) };
	return forward;
}

float3 RightDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const float3 right = { Cos(yaw), 0.0f, -Sin(yaw) };
	return right;
}

float4x4 ViewMatrixFromCamera(const Camera &camera)
{
	const float3 forward = ForwardDirectionFromAngles(camera.orientation);
	const float3 vrp = Add(camera.position, forward);
	constexpr float3 up = {0, 1, 0};
	const float4x4 res = LookAt(vrp, camera.position, up);
	return res;
}


struct Plane
{
	float3 normal; // Orientation of the plane
	float3 point; // Point in the plane
	//float distance; // Distance from the origin to the nearest point in the plane
};

struct FrustumPlanes
{
	Plane planes[6];
};

static FrustumPlanes FrustumPlanesFromCamera(float3 cameraPosition, float3 cameraForward, float zNear, float zFar, float fovy, float aspect)
{
	const float3 up = Float3(0.0f, 1.0f, 0.0f);
	const float3 cameraRight = Normalize(Cross(cameraForward, up));
	const float3 cameraUp = Normalize(Cross(cameraRight, cameraForward));

    const float farHalfY = zFar * Tan(0.5f * fovy * ToRadians);
    const float farHalfX = farHalfY * aspect;
	const float3 farRight = Mul(cameraRight, farHalfX);
	const float3 farLeft = Negate(farRight);
	const float3 farTop = Mul(cameraUp, farHalfY);
	const float3 farBot = Negate(farTop);
	const float3 farForward = Mul(cameraForward, zFar);

	const float3 topLeft = Add(Add(farForward, farLeft), farTop);
	const float3 topRight = Add(Add(farForward, farRight), farTop);
	const float3 botLeft = Add(Add(farForward, farLeft), farBot);
	const float3 botRight = Add(Add(farForward, farRight), farBot);

	const float3 topFaceNormal = Normalize(Cross(topLeft, topRight));
	const float3 botFaceNormal = Normalize(Cross(botRight, botLeft));
	const float3 rightFaceNormal = Normalize(Cross(topRight, botRight));
	const float3 leftFaceNormal = Normalize(Cross(botLeft, topLeft));
	const float3 farFaceNormal = Negate(cameraForward);
	const float3 nearFaceNormal = cameraForward;

	const float3 topFacePoint = cameraPosition;
	const float3 botFacePoint = cameraPosition;
	const float3 rightFacePoint = cameraPosition;
	const float3 leftFacePoint = cameraPosition;
	const float3 farFacePoint = Add(cameraPosition, Mul(cameraForward, zFar));
	const float3 nearFacePoint = Add(cameraPosition, Mul(cameraForward, zNear));

	const FrustumPlanes frustumPlanes = {
		.planes = {
			{ .normal = rightFaceNormal, .point = rightFacePoint },
			{ .normal = leftFaceNormal, .point = leftFacePoint },
			{ .normal = topFaceNormal, .point = topFacePoint },
			{ .normal = botFaceNormal, .point = botFacePoint },
			{ .normal = nearFaceNormal, .point = nearFacePoint },
			{ .normal = farFaceNormal, .point = farFacePoint },
		},
	};
    return frustumPlanes;
}


static bool PointIsInFrontOfPlane(float3 point, const Plane &plane)
{
	const float3 dir = FromTo(plane.point, point);
	const float dotResult = Dot(plane.normal, dir);
	return dotResult >= 0.0f;
}

static bool PointsAreInFrustum(const float3 *points, u32 pointCount, const FrustumPlanes &frustum)
{
	bool entityIsBehindPlane = false;
	for (u32 j = 0; j < ARRAY_COUNT(frustum.planes); ++j)
	{
		entityIsBehindPlane = true;
		for (u32 i = 0; i < pointCount; ++i)
		{
			if (PointIsInFrontOfPlane(points[i], frustum.planes[j]))
			{
				entityIsBehindPlane = false;
				break;
			}
		}
		if (entityIsBehindPlane)
		{
			break;
		}
	}
	return !entityIsBehindPlane;
}

static void GetSpriteBounds(const SpriteDesc &sprite, float2 bounds[4])
{
	const float2 size = float2{(f32)sprite.size.x, (f32)sprite.size.y} / PIXELS_PER_METER;
	bounds[0] = { 0, 0 };
	bounds[1] = { (f32)sprite.size.x, 0 };
	bounds[3] = { 0, (f32)sprite.size.y };
	bounds[2] = { (f32)sprite.size.x, (f32)sprite.size.y };
}

static bool EntityIsInFrustum3D(const Entity &entity, const FrustumPlanes &frustum)
{
	bool entityIsInFrustum = false;

	float3 points[8] = {};
	u32 pointCount = 0;

	if (entity.spriteId)
	{
		const SpriteDesc &sprite = GetSprite(entity.spriteId).desc;
		float2 sbounds[4] = {};
		GetSpriteBounds(sprite, sbounds);

		for (u32 i = 0; i < ARRAY_COUNT(sbounds); ++i) {
			points[pointCount++] = Float3(sbounds[i], 0.0f);
		}
	}
	else
	{
		float x = 0.5f * entity.scale;
		float y = 0.5f * entity.scale;
		const float z = 0.5f * entity.scale;

		points[pointCount++] = Add(entity.position, Float3(-x,-y,-z));
		points[pointCount++] = Add(entity.position, Float3( x,-y,-z));
		points[pointCount++] = Add(entity.position, Float3(-x, y,-z));
		points[pointCount++] = Add(entity.position, Float3( x, y,-z));
		points[pointCount++] = Add(entity.position, Float3(-x,-y, z));
		points[pointCount++] = Add(entity.position, Float3( x,-y, z));
		points[pointCount++] = Add(entity.position, Float3(-x, y, z));
		points[pointCount++] = Add(entity.position, Float3( x, y, z));
	}

	entityIsInFrustum = PointsAreInFrustum(points, pointCount, frustum);

	return entityIsInFrustum;
}

static float4 GetOrthographicCameraMinMaxRect(const Camera &camera, f32 ar)
{
	const f32 height = camera.height;
	const float2 halfExtent = { height*ar, height };
	const float2 rectMin = { camera.position.x - halfExtent.x, camera.position.y - halfExtent.y };
	const float2 rectMax = { camera.position.x + halfExtent.x, camera.position.y + halfExtent.y };
	const float4 minMaxRect = { .xy = rectMin, .zw =  rectMax };
	return minMaxRect;
}

static bool Intersects(float2 aMin, float2 aMax, float2 bMin, float2 bMax)
{
	const bool outsideX = aMax.x < bMin.x || aMin.x > bMax.x;
	const bool outsideY = aMax.y < bMin.y || aMin.y > bMax.y;
	return !(outsideX || outsideY);
}

static bool EntityIsInFrustum2D(const Entity &entity, float2 rectMin, float2 rectMax)
{
	float2 halfSize = { 0.5f * entity.scale, 0.5f * entity.scale };

	if (entity.spriteId)
	{
		const SpriteDesc &sprite = GetSprite(entity.spriteId).desc;
		halfSize = 0.5f * float2{(f32)sprite.size.x, (f32)sprite.size.y} / PIXELS_PER_METER;
	}

	const float2 entityMin = entity.position.xy;// { entity.position.x - halfSize.x, entity.position.y - halfSize.y };
	const float2 entityMax = entity.position.xy + 2.0f * halfSize;//{ entity.position.x + halfSize.x, entity.position.y + halfSize.y };

	return Intersects(entityMin, entityMax, rectMin, rectMax);
}


static f32 GetSceneAspectRatio(const Graphics &gfx)
{
	const uint2 sceneSize = gfx.renderTargets.sceneSize;
	const f32 preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
	ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
	const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
	const f32 ar = isLandscapeRotation ? (f32) sceneSize.x / sceneSize.y : (f32) sceneSize.y / sceneSize.x;
	return ar;
}

static float2 GetRoomScrollRatio(const float2 &cameraPos, const float2 &roomPos, const float2 &roomSizeWorld, const float2 &viewportSizeWorld)
{
	const float2 viewMin = cameraPos - 0.5f * viewportSizeWorld - roomPos;
	const float2 travel = roomSizeWorld - viewportSizeWorld;
	float2 ratio = { 0.5f, 0.5f };
	if (travel.x > 0.0f) ratio.x = Clamp(viewMin.x / travel.x, 0.0f, 1.0f);
	if (travel.y > 0.0f) ratio.y = Clamp(viewMin.y / travel.y, 0.0f, 1.0f);
	return ratio;
}

static float2 GetParallaxOffset(const float2 &scrollRatio, const uint2 &baseLayerSize, const uint2 &layerSize, const float2 &viewportSizeWorld)
{
	const float2 maxSlack = Max(Float2(baseLayerSize) - viewportSizeWorld, float2{0.0f, 0.0f});
	const float2 slack = Min(Max(Float2(baseLayerSize) - Float2(layerSize), float2{0.0f, 0.0f}), maxSlack);
	const float2 offset = scrollRatio * slack;
	constexpr f32 pixelSize = 1.0f / PIXELS_PER_METER;
	return pixelSize * Floor(offset / pixelSize);
}

bool RenderGraphics(Engine &engine)
{
	PROFILE_BLOCK(RenderGraphics);

	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;
	Window &window = *sPlatform->window;
#if USE_EDITOR
	Editor &editor = engine.editor;
#endif

	static f32 totalSeconds = 0.0f;
	totalSeconds += gfx.deltaSeconds;

	u32 frameIndex = gfx.device.frameIndex;

	{
		PROFILE_BLOCK(BeginFrame);
		BeginFrame(gfx.device);
	}

	// Now that BeginFrame waited for this frame slot fences, the timestamps written
	// MAX_FRAMES_IN_FLIGHT frames ago into this same slot can finally be read back
	PROFILE_GPU_RESOLVE(gfx.device);

#if USE_UI
	UI_FinalizeDrawData(engine.ui);
#endif

	// Display size
	const i32 displayWidth = gfx.device.swapchain.extent.width;
	const i32 displayHeight = gfx.device.swapchain.extent.height;

	// Camera setup
	float4x4 viewMatrix = Eye();
	float4x4 inverseViewMatrix = Eye();
	float4x4 viewportRotationMatrix = Eye();
	float4x4 projectionMatrix = Eye();
	float4 frustumTopLeft = {};
	float4 frustumBottomRight = {};

	// In game mode the scene renders to a low-res target; snap the camera and
	// entity positions to the pixel grid so sprites don't shimmer at sub-pixel
	// offsets. The game keeps its own unsnapped camera, so smooth movements
	// (e.g. lerps) are not affected by this.
	const bool snapToPixelGrid = engine.game.state == GameStateRunning;
	constexpr f32 pixelSize = 1.0f / PIXELS_PER_METER;

	Camera camera = engine.gfx.camera;
	if (snapToPixelGrid && camera.projectionType == ProjectionOrthographic)
	{
		camera.position.x = Round(camera.position.x / pixelSize) * pixelSize;
		camera.position.y = Round(camera.position.y / pixelSize) * pixelSize;
	}

	const f32 preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
	const f32 ar = GetSceneAspectRatio(gfx);
	const float4 cameraMinMaxRect = GetOrthographicCameraMinMaxRect(camera, ar);

	if (camera.projectionType == ProjectionPerspective)
	{
		// Calculate camera matrices
		viewMatrix = ViewMatrixFromCamera(camera);
		inverseViewMatrix = Float4x4(Transpose(Float3x3(viewMatrix)));
		const float fovy = camera.fovy;
		const float znear = camera.znear;
		const float zfar = camera.zfar;
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 perspectiveMatrix = Perspective(fovy, ar, znear, zfar);
		projectionMatrix = Mul(viewportRotationMatrix, perspectiveMatrix);

		// Frustum vectors
		const float hypotenuse  = znear / Cos( 0.5f * fovy * ToRadians );
		const float top = hypotenuse * Sin( 0.5f * fovy * ToRadians );
		const float bottom = -top;
		const float right = top * ar;
		const float left = -right;
		frustumTopLeft = Float4( Float3(left, top, -znear), 0.0f );
		frustumBottomRight = Float4( Float3(right, bottom, -znear), 0.0f );


		// CPU Frustum culling
		const float3 cameraForward = ForwardDirectionFromAngles(camera.orientation);
		const FrustumPlanes frustumPlanes = FrustumPlanesFromCamera(camera.position, cameraForward, znear, zfar, fovy, ar);
		for (u32 i = 0; i < scene.entityCount; ++i)
		{
			Entity &entity = scene.entities[i];
			entity.culled = !EntityIsInFrustum3D(entity, frustumPlanes);
		}
	}
	else
	{
		const f32 height = camera.height;
		// Calculate camera matrices
		viewMatrix = ViewMatrixFromCamera(camera);
		//inverseViewMatrix = Inverse2D(viewMatrix);
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 orthographicMatrix = Orthogonal(-height*ar, height*ar, -height, height, camera.znear, camera.zfar);
		projectionMatrix = Mul(viewportRotationMatrix, orthographicMatrix);

		// Frustum vectors
		frustumTopLeft = Float4( Float3(-height*ar, height, 0), 0.0f );
		frustumBottomRight = Float4( Float3(height*ar, -height, 0), 0.0f );

		// CPU Frustum culling
		for (u32 i = 0; i < scene.entityCount; ++i)
		{
			Entity &entity = scene.entities[i];
			entity.culled = !EntityIsInFrustum2D(entity, cameraMinMaxRect.xy, cameraMinMaxRect.zw);
		}
	}

	// Sun matrices
	const float3 sunDirUnnormalized =
		(camera.projectionType == ProjectionPerspective) ?
		Float3(-2.0f, 2.0f, -1.0f) : Float3(0.0f, 0.0f, -1.0f);

	const float4x4 sunRotationMatrix = Rotate(float3{0.0, 1.0, 0.0}, 180.0f);
	const float3 sunDir = Normalize(MulVector(sunRotationMatrix, sunDirUnnormalized));
	const float3 sunPos = Float3(0.0f, 0.0f, 0.0f);
	const float3 sunVrp = Sub(sunPos, sunDir);
	const float3 sunUp = Float3(0.0f, 1.0f, 0.0f);
	const float4x4 sunViewMatrix = LookAt(sunVrp, sunPos, sunUp);
	const float4x4 sunProjMatrix = Orthogonal(-5.0f, 5.0f, -10.0f, 5.0f, -5.0f, 10.0f);

	// Camera UI 2D
	const f32 l = 0.0f;
	const f32 r = (f32) displayWidth;
	const f32 t = 0.0f;
	const f32 b = (f32) displayHeight;
	const f32 n = 0.0f;
	const f32 f = 1.0f;
	const float4x4 camera2dProjection = Orthogonal(l, r, b, t, n, f);

	const ID selectedEntity = EditorGetSelectedEntity(editor);
	const u32 selectedEntityDrawId = selectedEntity
		? EntityDrawId(scene, selectedEntity) : 0xFFFFFFFF;

	// Update globals struct
	const Globals globals = {
		.cameraView = viewMatrix,
		.cameraViewInv = inverseViewMatrix,
		.cameraProj = projectionMatrix,
		.camera2dProj = camera2dProjection, // UI
		.viewportRotationMatrix = viewportRotationMatrix,
		.cameraFrustumTopLeft = frustumTopLeft,
		.cameraFrustumBottomRight = frustumBottomRight,
		.sunView = sunViewMatrix,
		.sunProj = sunProjMatrix,
		.sunDir = Float4(sunDir, 0.0f),
		.eyePosition = Float4(camera.position, 1.0f),
		.shadowmapDepthBias = 0.005,
		.time = totalSeconds,
		.sceneResolution = GetFramebufferSize(gfx.renderTargets.sceneFramebuffer),
		.mousePosition = window.mouse.pos,
#if USE_EDITOR
		.selectedEntity = selectedEntityDrawId,
#endif
	};

	// Update globals buffer
	Globals *globalsBufferPtr = (Globals*)GetBufferPtr(gfx.device, gfx.globalsBuffer[frameIndex]);
	*globalsBufferPtr = globals;

	// Advance animation states for animated sprites (frameCount > 1)
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		const SpriteDesc &sprite = scene.sprites[i].desc;
		if (sprite.frameCount <= 1) continue;
		SpriteAnimState &state = scene.spriteAnimStates[i];
		state.elapsedTime += gfx.deltaSeconds;
		const f32 totalDuration = (f32)sprite.frameCount / (f32)sprite.fps;
		if (sprite.loop && state.elapsedTime >= totalDuration)
			state.elapsedTime = fmodf(state.elapsedTime, totalDuration);
		state.currentFrame = (u32)(state.elapsedTime * (f32)sprite.fps);
		if (state.currentFrame >= sprite.frameCount)
			state.currentFrame = sprite.frameCount - 1;
	}

	// Update sprite data buffer (UV per sprite)
	SSpriteData *spriteDataPtr = (SSpriteData*)GetBufferPtr(gfx.device, gfx.spriteDataBuffer[frameIndex]);
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		const SpriteDesc &sprite = scene.sprites[i].desc;
		const Texture &texture = GetTexture(sprite.textureId);
		{
			const float2 frameUvPos  = { (f32)sprite.pos.x / texture.size.x, (f32)sprite.pos.y / texture.size.y };
			const float2 frameUvSize = { (f32)sprite.size.x / texture.size.x, (f32)sprite.size.y / texture.size.y };
			const float frameOffsetU = sprite.frameCount > 1
				? scene.spriteAnimStates[i].currentFrame * frameUvSize.x
				: 0.0f;
			spriteDataPtr[i].uvOffset  = {frameUvPos.x + frameOffsetU, frameUvPos.y};
			spriteDataPtr[i].uvSize    = frameUvSize;
			spriteDataPtr[i].worldSize = float2{(f32)sprite.size.x, (f32)sprite.size.y} / PIXELS_PER_METER;
		}
	}

	// Layer: Update tile data buffer
	const u32 tileScratchSize = MAX_TILES * (sizeof(STileData) + sizeof(ID));
	Scratch tileScratch(tileScratchSize);
	STileData *tileDataPtr = PushArray(tileScratch.arena, STileData, MAX_TILES);
	ID *tileSpriteIds = PushArray(tileScratch.arena, ID, MAX_TILES);


	const float2 viewportSizeWorld = Float2(GetFramebufferSize(gfx.renderTargets.sceneFramebuffer)) / PIXELS_PER_METER;
	const bool parallaxEnabled = engine.game.state == GameStateRunning; // The editor shows all layers unshifted

	u32 tileCount = 0;
	for (u32 roomIndex = 0; roomIndex < scene.roomCount; ++roomIndex)
	{
		const Room &room = scene.rooms[roomIndex];

		// TODO: Skip room if not in camera

		// We always need a base layer to render a room
		const Layer *baseLayer = GetBaseLayer(room);
		if (!baseLayer) continue;

		const uint2 baseLayerSize = baseLayer->size;
		const float2 roomPos = Float2(room.pos);
		const float2 scrollRatio = GetRoomScrollRatio(camera.position.xy, roomPos, Float2(baseLayerSize), viewportSizeWorld);

		for (i32 i = ARRAY_COUNT(room.layers) - 1; i >= 0; --i)
		{
			const Layer &layer = room.layers[i];

			if (layer.initialized && layer.visible && !layer.isCollider)
			{
				const float2 parallax = parallaxEnabled ? GetParallaxOffset(scrollRatio, baseLayerSize, layer.size, viewportSizeWorld) : float2{0.0f, 0.0f};

				for (i32 y = 0; y < layer.size.y; ++y)
				{
					for (i32 x = 0; x < layer.size.x; ++x)
					{
						// TODO: Skip cell if not in camera

						const ID spriteId = layer.cells[x][y].spriteId;
						if (spriteId && tileCount < MAX_TILES)
						{
							tileDataPtr[tileCount].pos.xy = roomPos + Float2(int2{x, y}) + parallax;
							tileDataPtr[tileCount].pos.z = -(float)i;
							tileDataPtr[tileCount].spriteIndex = GetSpriteIndex(scene, spriteId);
							tileSpriteIds[tileCount] = spriteId;
							tileCount++;
						}
					}
				}
			}
		}
	}

	STileData *gpuTileDataPtr = (STileData*)GetBufferPtr(gfx.device, gfx.tileDataBuffer[frameIndex]);
	MemCopy(gpuTileDataPtr, tileDataPtr, tileCount * sizeof(STileData));

	// Update entity data
	SEntity *entities = (SEntity*)GetBufferPtr(gfx.device, gfx.entityBuffer[frameIndex]);
	for (u32 i = 0; i < scene.entityCount; ++i)
	{
		const Entity &entity = scene.entities[i];
		float3 entityScale = Float3(entity.scale);
		float3 entityPosition = entity.position;
		if (snapToPixelGrid)
		{
			entityPosition.x = Round(entityPosition.x / pixelSize) * pixelSize;
			entityPosition.y = Round(entityPosition.y / pixelSize) * pixelSize;
		}
		const float4x4 worldMatrix = Mul(Translate(entityPosition), Scale(entityScale)); // TODO: Apply also rotation
		entities[i].world = worldMatrix;

		const u32 spriteIndex = entity.spriteId ? GetSpriteIndex(scene, entity.spriteId) : 0;
		entities[i].spriteIndex = spriteIndex;
	}

	// Update materials
	if (gfx.shouldUpdateMaterials)
	{
		gfx.shouldUpdateMaterials = false;
		UploadMaterialData(gfx);
	}

	// Update bind groups
	if (gfx.shouldUpdateGlobalBindGroups)
	{
		gfx.shouldUpdateGlobalBindGroups = false;
		UpdateGlobalBindGroups(gfx);
	}

	if (gfx.shouldUpdateMaterialBindGroups)
	{
		gfx.shouldUpdateMaterialBindGroups = false;
		UpdateMaterialBindGroups(gfx);
	}

	// Reset per-frame bind group allocators
	ResetDynamicBindGroups( gfx );

	// Record commands
	CommandList commandList = BeginCommandList(gfx.device);

	// Timestamps for this frame (resolved MAX_FRAMES_IN_FLIGHT frames from now)
	PROFILE_GPU_FRAME_BEGIN(commandList);

	#if USE_COMPUTE_TEST
	{
		const Pipeline &pipeline = GetPipeline(gfx.device, gfx.computeClearH);

		SetPipeline(commandList, gfx.computeClearH);

		const BindGroupDesc bindGroupDesc = {
			.layout = pipeline.layout.bindGroupLayouts[0],
			.bindings = {
				{ .index = 0, .bufferView = gfx.computeBufferViewH },
			},
		};
		const BindGroup bindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

		SetBindGroup(commandList, 0, bindGroup);

		Dispatch(commandList, 1, 1, 1);
	}
	#endif // USE_COMPUTE_TEST

	const BufferH globalsBuffer = gfx.globalsBuffer[frameIndex];
	const BufferH entityBuffer = gfx.entityBuffer[frameIndex];
	const BufferH vertexBuffer = gfx.globalVertexArena.buffer;
	const BufferH indexBuffer = gfx.globalIndexArena.buffer;

	// Shadow map
	if (camera.projectionType == ProjectionPerspective)
	{
		PROFILE_BLOCK(ShadowMap);
		PROFILE_GPU_BLOCK(commandList, ShadowMap);

		BeginDebugGroup(commandList, "Shadow map", ColorBlack);

		SetClearDepth(commandList, 0, 0.0f);

		const Framebuffer shadowmapFramebuffer = GetShadowmapFramebuffer(gfx);
		BeginRenderPass(commandList, shadowmapFramebuffer);

		const uint2 shadowmapSize = GetFramebufferSize(shadowmapFramebuffer);
		SetViewportAndScissor(commandList, shadowmapSize);

		SetPipeline(commandList, gfx.pipelines[Pipeline_Shadowmap]);

		SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

		for (u32 entityIndex = 0; entityIndex < scene.entityCount; ++entityIndex)
		{
			const Entity &entity = scene.entities[entityIndex];

			if ( !entity.visible ) continue;

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/sizeof(Index);
			const uint32_t firstIndex = entity.indices.offset/sizeof(Index);
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, EntityDrawId(scene, scene.entities[entityIndex].id));
		}

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

	const Format depthFormat = gfx.device.defaultDepthFormat;
	const ImageH shadowmapImage = gfx.renderTargets.shadowmapImage;
	TransitionImageLayout(commandList, shadowmapImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

	// Scene
	{
		PROFILE_BLOCK(Scene);
		PROFILE_GPU_BLOCK(commandList, Scene);

		BeginDebugGroup(commandList, "Scene", ColorBlack);

		SetClearColorFloat4(commandList, 0, { 0.0f, 0.0f, 0.0f, 0.0f } );
		SetClearDepth(commandList, 1, 0.0f);

		const Framebuffer &sceneFramebuffer = gfx.renderTargets.sceneFramebuffer;
		BeginRenderPass(commandList, sceneFramebuffer);

		const uint2 displaySize = GetFramebufferSize(sceneFramebuffer);
		SetViewportAndScissor(commandList, displaySize);

		// Entities
		for (u32 i = 0; i < scene.entityCount; ++i)
		{
			const Entity &entity = scene.entities[i];

			if ( !entity.visible || entity.culled ) continue;
			if ( !entity.materialId ) continue;

			const ID materialId = entity.materialId;
			const Material &material = GetMaterial(materialId);

			BeginDebugGroup(commandList, material.desc.name, ColorBlack);

			// Pipeline
			SetPipeline(commandList, gfx.pipelines[material.pipelineIndex]);

			// Bind groups
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 1, gfx.materialBindGroups[GetMaterialIndex(gfx, materialId)]);

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/sizeof(Index);
			const uint32_t firstIndex = entity.indices.offset/sizeof(Index);
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, EntityDrawId(scene, entity.id));

			EndDebugGroup(commandList);
		}

		// Layer tiles
		{
			PROFILE_BLOCK(LayerTiles);

			BeginDebugGroup(commandList, "Tiles", ColorBlack);

			const Pipeline &tilePipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_Shading2DTile]);
			const uint32_t tileIndexCount = gfx.spriteIndices.size / sizeof(Index);
			const uint32_t tileFirstIndex = gfx.spriteIndices.offset / sizeof(Index);
			const int32_t tileFirstVertex = gfx.spriteVertices.offset / sizeof(Vertex);

			SetPipeline(commandList, gfx.pipelines[Pipeline_Shading2DTile]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 i = 0; i < tileCount; ++i)
			{
				const ID spriteId = tileSpriteIds[i];
				const SpriteDesc &sprite = GetSprite(spriteId).desc;
				const ImageH imageH = GetTextureImage(gfx, sprite.textureId, gfx.pinkImageH);
				const BindGroupDesc textureBindGroupDesc = {
					.layout = tilePipeline.layout.bindGroupLayouts[2],
					.bindings = {
						{ .index = 0, .image = imageH },
					},
				};
				const BindGroup textureBindGroup = GetOrCreateDynamicBindGroup(gfx, textureBindGroupDesc);

				SetBindGroup(commandList, 2, textureBindGroup);
				DrawIndexed(commandList, tileIndexCount, tileFirstIndex, tileFirstVertex, i);
			}

			EndDebugGroup(commandList);
		}

		{
			PROFILE_BLOCK(Sprites);
			// Sprite entities
			const Pipeline &spritePipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_Shading2D]);
			const uint32_t spriteIndexCount = gfx.spriteIndices.size / sizeof(Index);
			const uint32_t spriteFirstIndex = gfx.spriteIndices.offset / sizeof(Index);
			const int32_t spriteFirstVertex = gfx.spriteVertices.offset / sizeof(Vertex);

			SetPipeline(commandList, gfx.pipelines[Pipeline_Shading2D]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 i = 0; i < scene.entityCount; ++i)
			{
				const Entity &entity = scene.entities[i];

				if (!entity.visible || entity.culled) continue;

				ID textureId = {};
				if (entity.spriteId)
					textureId = GetSprite(entity.spriteId).desc.textureId;
				else
					continue;

				const ImageH imageH = GetTextureImage(gfx, textureId, gfx.pinkImageH);
				const BindGroupDesc textureBindGroupDesc = {
					.layout = spritePipeline.layout.bindGroupLayouts[2],
					.bindings = {
						{ .index = 0, .image = imageH },
					},
				};
				const BindGroup textureBindGroup = GetOrCreateDynamicBindGroup(gfx, textureBindGroupDesc);

				BeginDebugGroup(commandList, entity.name ? entity.name : "sprite", ColorBlack);
				SetBindGroup(commandList, 2, textureBindGroup);
				DrawIndexed(commandList, spriteIndexCount, spriteFirstIndex, spriteFirstVertex, EntityDrawId(scene, entity.id));
				EndDebugGroup(commandList);
			}
		}

		// Sky
		if (camera.projectionType == ProjectionPerspective)
		{
			PROFILE_BLOCK(Sky);

			const ImageH &skyImage = GetTextureImage(gfx, gfx.skyTexture, gfx.grayImageH);
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_Sky]);
			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/sizeof(Index);
			const uint32_t firstIndex = indices.offset/sizeof(Index);
			const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

			const BindGroupDesc bindGroupDesc = {
				.layout = pipeline.layout.bindGroupLayouts[3],
				.bindings = {
					{ .index = 0, .sampler = gfx.skySamplerH },
					{ .index = 1, .image = skyImage },
				},
			};
			const BindGroup bindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

			BeginDebugGroup(commandList, "sky", ColorBlack);

			SetPipeline(commandList, gfx.pipelines[Pipeline_Sky]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 3, bindGroup);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			EndDebugGroup(commandList);
		}

#if USE_EDITOR
		// Editor grid
		if (editor.showGrid && engine.game.state == GameStateStopped)
		{
			PROFILE_BLOCK(EditorGrid);

			if (camera.projectionType == ProjectionPerspective)
			{
				const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
				const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
				const uint32_t indexCount = indices.size/sizeof(Index);
				const uint32_t firstIndex = indices.offset/sizeof(Index);
				const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

				BeginDebugGroup(commandList, "grid_3d", ColorBlack);

				SetPipeline(commandList, gfx.pipelines[Pipeline_Grid3D]);
				SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
				SetVertexBuffer(commandList, vertexBuffer);
				SetIndexBuffer(commandList, indexBuffer);
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

				EndDebugGroup(commandList);
			}
			else // if (IsEngineMode2D(engine.mode))
			{
				const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
				const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
				const uint32_t indexCount = indices.size/sizeof(Index);
				const uint32_t firstIndex = indices.offset/sizeof(Index);
				const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

				BeginDebugGroup(commandList, "grid_2d", ColorBlack);

				SetPipeline(commandList, gfx.pipelines[Pipeline_Grid2D]);
				SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
				SetVertexBuffer(commandList, vertexBuffer);
				SetIndexBuffer(commandList, indexBuffer);
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

				EndDebugGroup(commandList);
			}
		}
#endif

		{ // Debug draw
			PROFILE_BLOCK(DebugDraw);

			MemCopy(gfx.debugDrawVertices[frameIndex], gfx.debugDrawVerticesCPU, gfx.debugDrawVertexCount * sizeof(DebugDrawVertex));

			BeginDebugGroup(commandList, "DebugDraw", ColorBlack);

			const Pipeline &debugDrawPipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_DebugDraw]);
			const BindGroupLayout &bindGroupLayout = debugDrawPipeline.layout.bindGroupLayouts[3];

			SetPipeline(commandList, gfx.pipelines[Pipeline_DebugDraw]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, gfx.debugDrawVertexBuffer[frameIndex]);

			for (u32 i = 0; i < gfx.debugDrawBatchCount; ++i)
			{
				const DebugDrawBatch &batch = gfx.debugDrawBatches[i];

				const BindGroupDesc bindGroupDesc = {
					.layout = bindGroupLayout,
					.bindings = {
						{ .index = 0, .sampler = gfx.pointSamplerH },
						{ .index = 1, .image = batch.imageH },
					},
				};
				const BindGroup bindGroup = GetOrCreateDynamicBindGroup(gfx, bindGroupDesc);
				SetBindGroup(commandList, 3, bindGroup);

				Draw(commandList, batch.vertexCount, batch.vertexIndex);
			}

			gfx.debugDrawVertexCount = 0;
			gfx.debugDrawBatchCount = 0;

			EndDebugGroup(commandList);
		}

		if ( engine.game.state == GameStateRunning )
		{
			PROFILE_BLOCK(Fog);
			PROFILE_GPU_BLOCK(commandList, Fog);

			BeginDebugGroup(commandList, "Fog", ColorBlack);

			SetPipeline(commandList, gfx.pipelines[Pipeline_Fog]);

			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/sizeof(Index);
			const uint32_t firstIndex = indices.offset/sizeof(Index);
			const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			EndDebugGroup(commandList);
		}

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

	// Display
	{
		PROFILE_BLOCK(Display);
		PROFILE_GPU_BLOCK(commandList, Display);

		BeginDebugGroup(commandList, "Display", ColorBlack);

		TransitionImageLayout(commandList, gfx.renderTargets.sceneImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

		const Framebuffer displayFramebuffer = GetDisplayFramebuffer(gfx);
		BeginRenderPass(commandList, displayFramebuffer);

		const uint2 displaySize = GetFramebufferSize(displayFramebuffer);
		SetViewportAndScissor(commandList, displaySize);

		{ // Scene blit
			PROFILE_BLOCK(Blit);
			PROFILE_GPU_BLOCK(commandList, Blit);

			BeginDebugGroup(commandList, "Blit", ColorBlack);

			const uint2 sceneSize = gfx.renderTargets.sceneSize;
			const u32 multiplier = Min(displaySize.x / sceneSize.x, displaySize.y / sceneSize.y);
			const uint2 scaledSceneSize = multiplier * sceneSize;
			const rect viewport = {
				displaySize.x > scaledSceneSize.x ? (i32)(displaySize.x - scaledSceneSize.x) / 2 : 0,
				displaySize.y > scaledSceneSize.y ? (i32)(displaySize.y - scaledSceneSize.y) / 2 : 0,
				scaledSceneSize.x,
				scaledSceneSize.y,
			};
			SetViewport(commandList, viewport);
			SetScissor(commandList, viewport);

			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_Blit]);
			const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[3];

			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/sizeof(Index);
			const uint32_t firstIndex = indices.offset/sizeof(Index);
			const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

			SetPipeline(commandList, gfx.pipelines[Pipeline_Blit]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			ImageH sceneImage = gfx.renderTargets.sceneImage;
			const BindGroupDesc bindGroupDesc = {
				.layout = bindGroupLayout,
				.bindings = {
					{ .index = 0, .sampler = gfx.screenSamplerH },
					{ .index = 1, .image = sceneImage },
				},
			};
			const BindGroup textureBindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);
			SetBindGroup(commandList, 3, textureBindGroup);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			SetViewportAndScissor(commandList, displaySize);

			EndDebugGroup(commandList);
		}

#if USE_UI
		{ // GUI
			PROFILE_BLOCK(GUI);
			PROFILE_GPU_BLOCK(commandList, GUI);

			BeginDebugGroup(commandList, "GUI", ColorBlack);

			const UI &ui = engine.ui;

			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_UI]);
			const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[3];

			SetPipeline(commandList, gfx.pipelines[Pipeline_UI]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, UI_GetVertexBuffer(ui));

			for (u32 i = 0; i < UI_DrawListCount(ui); ++i)
			{
				const UIDrawList &drawList = UI_GetDrawListAt(ui, i);
				SetScissor(commandList, drawList.scissorRect);

				const BindGroupDesc textureBindGroupDesc = {
					.layout = bindGroupLayout,
					.bindings = {
						{ .index = 0, .sampler = gfx.pointSamplerH },
						{ .index = 1, .image = drawList.imageHandle },
					},
				};
				const BindGroup textureBindGroup = GetOrCreateDynamicBindGroup(gfx, textureBindGroupDesc);
				SetBindGroup(commandList, 3, textureBindGroup);

				for (u32 i = 0; i < drawList.vertexRangeCount; ++i)
				{
					const UIVertexRange &range = drawList.vertexRanges[i];
					Draw(commandList, range.count, range.index);
				}
			}

			EndDebugGroup(commandList);
		}
#endif

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

#if USE_EDITOR
	EditorRender(engine, commandList);
#endif // USE_EDITOR


	TransitionImageLayout(commandList, shadowmapImage, ImageStateShaderInput, ImageStateRenderTarget, 0, 1);

	PROFILE_GPU_FRAME_END(commandList);

	EndCommandList(commandList);

	SubmitResult submitRes;

	{
		PROFILE_BLOCK(Submit);
		submitRes = Submit(gfx.device, commandList);
	}

	{
		PROFILE_BLOCK(Present);
		if ( !Present(gfx.device, submitRes) ) {
			return false;
		}
	}

	// TODO: Check if this should be executed even if Present failed...
	PROFILE_BLOCK(EndFrame);
	EndFrame(gfx.device);

	return true;
}


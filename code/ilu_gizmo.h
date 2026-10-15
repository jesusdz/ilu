/*
 * ilu_gizmo.h
 * Author: Jesus Diaz Garcia
 *
 * Immediate mode gizmo API.
 */

#ifndef ILU_GIZMO_H
#define ILU_GIZMO_H

#ifndef ILU_UI_H
#error "ilu_ui.h needs to be defined before ilu_gizmo.h"
#endif

// Sizes in pixels, so handles keep their size on screen at any zoom
#define GIZMO_AXIS_LENGTH 64.0f
#define GIZMO_AXIS_THICKNESS 2.0f
#define GIZMO_ARROW_SIZE 12.0f
#define GIZMO_CENTER_SIZE 14.0f
#define GIZMO_SCALE_SIZE 10.0f
#define GIZMO_SCALE_OFFSET 40.0f
#define GIZMO_PICK_MARGIN 4.0f

constexpr float4 GizmoColorX = { 0.9, 0.2, 0.2, 1.0 };
constexpr float4 GizmoColorY = { 0.2, 0.9, 0.2, 1.0 };
constexpr float4 GizmoColorXY = { 0.3, 0.5, 1.0, 0.8 };
constexpr float4 GizmoColorScale = { 0.9, 0.9, 0.9, 1.0 };
constexpr float4 GizmoColorHot = { 1.0, 0.8, 0.0, 1.0 };

enum GizmoHandle
{
	GizmoHandle_None,
	GizmoHandle_MoveXY,
	GizmoHandle_MoveX,
	GizmoHandle_MoveY,
	GizmoHandle_Scale,
};

// Orthographic 2D view. Pixel (0,0) is the top-left corner, world Y points up.
struct GizmoView
{
	float2 center;
	f32 worldPerPixel;
	uint2 viewportSize;
};

struct Gizmo
{
	UI *ui;
	GizmoView view;
	bool inputBlocked;
	bool visible;

	GizmoHandle hot;
	GizmoHandle active;
	GizmoHandle requested;
	// Modal drags follow the mouse with no button held: a left click confirms them
	bool modal;
	bool mouseConsumed;

	float2 dragStartMouse;
	float3 dragStartPosition;
	f32 dragStartScale;
};

static float2 Gizmo_WorldToScreen(const GizmoView &view, float2 world)
{
	const float2 offset = ( world - view.center ) / view.worldPerPixel;
	const float2 res = 0.5f * Float2(view.viewportSize) + float2{offset.x, -offset.y};
	return res;
}

static float2 Gizmo_ScreenToWorld(const GizmoView &view, float2 screen)
{
	const float2 offset = screen - 0.5f * Float2(view.viewportSize);
	const float2 res = view.center + view.worldPerPixel * float2{offset.x, -offset.y};
	return res;
}

static bool Gizmo_PointInRect(float2 point, float2 min, float2 max)
{
	const float2 margin = { GIZMO_PICK_MARGIN, GIZMO_PICK_MARGIN };
	const float2 lo = min - margin;
	const float2 hi = max + margin;
	const bool res = point.x >= lo.x && point.x < hi.x && point.y >= lo.y && point.y < hi.y;
	return res;
}

static void Gizmo_AddRect(UI &ui, float2 min, float2 max, float4 color)
{
	const float2 tl = min;
	const float2 bl = { min.x, max.y };
	const float2 br = max;
	const float2 tr = { max.x, min.y };
	UI_AddTriangle(ui, tl, bl, br, color);
	UI_AddTriangle(ui, tl, br, tr, color);
}

static f32 Gizmo_Snap(f32 value, f32 snap)
{
	const f32 res = snap > 0.0f ? Round(value / snap) * snap : value;
	return res;
}

static float3 Gizmo_Snap(float3 value, f32 snap)
{
	const float3 res = snap > 0.0f ? Round(value / snap) * snap : value;
	return res;
}

static float4 Gizmo_HandleColor(const Gizmo &g, GizmoHandle handle, float4 color)
{
	const GizmoHandle highlighted = g.active ? g.active : g.hot;
	const float4 res = highlighted == handle ? GizmoColorHot : color;
	return res;
}

bool Gizmo_IsHovered(const Gizmo &g)
{
	return g.hot != GizmoHandle_None;
}

bool Gizmo_IsActive(const Gizmo &g)
{
	return g.active != GizmoHandle_None;
}

bool Gizmo_WantsMouse(const Gizmo &g)
{
	return g.active != GizmoHandle_None || g.mouseConsumed;
}

void Gizmo_StartModalDrag(Gizmo &g, GizmoHandle handle)
{
	g.requested = handle;
}

void Gizmo_Initialize(Gizmo &g, UI &ui)
{
	g = {};
	g.ui = &ui;
}

void Gizmo_SetViewportSize(Gizmo &g, uint2 size)
{
	g.view.viewportSize = size;
}

void Gizmo_SetCamera(Gizmo &g, float2 center, f32 worldPerPixel)
{
	g.view.center = center;
	g.view.worldPerPixel = worldPerPixel;
}

void Gizmo_BeginFrame(Gizmo &g)
{
	g.inputBlocked = UI_IsHovered(*g.ui);
	g.visible = false;
	g.mouseConsumed = false;
	g.hot = GizmoHandle_None;
}

void Gizmo_EndFrame(Gizmo &g)
{
	// A drag only lives while its gizmo keeps being visible
	if ( !g.visible )
	{
		g.active = GizmoHandle_None;
	}
	g.requested = GizmoHandle_None;
}

bool Gizmo_Transform(Gizmo &g, float3 &position, f32 &scale, f32 snap = 0.0f, f32 scaleSnap = 0.0f)
{
	ASSERT( g.ui != nullptr );
	UI &ui = *g.ui;
	const Mouse &mouse = ui.input.mouse;
	const float2 mousePos = Float2(mouse.pos);
	const float2 mouseWorld = Gizmo_ScreenToWorld(g.view, mousePos);

	g.visible = true;

	const float2 origin = Gizmo_WorldToScreen(g.view, position.xy);
	const f32 halfCenter = 0.5f * GIZMO_CENTER_SIZE;
	const f32 halfArrow = 0.5f * GIZMO_ARROW_SIZE;
	const f32 halfThickness = 0.5f * GIZMO_AXIS_THICKNESS;
	const f32 halfScale = 0.5f * GIZMO_SCALE_SIZE;

	const float2 centerMin = origin - float2{halfCenter, halfCenter};
	const float2 centerMax = origin + float2{halfCenter, halfCenter};
	const float2 xTip = origin + float2{GIZMO_AXIS_LENGTH, 0.0f};
	const float2 yTip = origin - float2{0.0f, GIZMO_AXIS_LENGTH};
	const float2 scalePos = origin + float2{GIZMO_SCALE_OFFSET, -GIZMO_SCALE_OFFSET};

	if ( !g.active && !g.inputBlocked )
	{
		if ( Gizmo_PointInRect(mousePos, centerMin, centerMax) ) {
			g.hot = GizmoHandle_MoveXY;
		} else if ( Gizmo_PointInRect(mousePos, scalePos - float2{halfScale, halfScale}, scalePos + float2{halfScale, halfScale}) ) {
			g.hot = GizmoHandle_Scale;
		} else if ( Gizmo_PointInRect(mousePos, float2{centerMax.x, origin.y - halfArrow}, xTip + float2{GIZMO_ARROW_SIZE, halfArrow}) ) {
			g.hot = GizmoHandle_MoveX;
		} else if ( Gizmo_PointInRect(mousePos, yTip - float2{halfArrow, GIZMO_ARROW_SIZE}, float2{origin.x + halfArrow, centerMin.y}) ) {
			g.hot = GizmoHandle_MoveY;
		}
	}

	bool committed = false;

	if ( !g.active )
	{
		const bool clicked = g.hot && MouseButtonPress(mouse, MOUSE_BUTTON_LEFT);
		if ( g.requested || clicked )
		{
			g.active = g.requested ? g.requested : g.hot;
			g.modal = g.requested != GizmoHandle_None;
			g.mouseConsumed = clicked;
			g.dragStartMouse = mouseWorld;
			g.dragStartPosition = position;
			g.dragStartScale = scale;
		}
	}
	else if ( KeyPress(ui.input.keyboard, K_ESCAPE) || ( g.modal && MouseButtonPress(mouse, MOUSE_BUTTON_RIGHT) ) )
	{
		position = g.dragStartPosition;
		scale = g.dragStartScale;
		g.active = GizmoHandle_None;
	}
	else if ( g.modal ? MouseButtonPress(mouse, MOUSE_BUTTON_LEFT) : !MouseButtonPressed(mouse, MOUSE_BUTTON_LEFT) )
	{
		const float3 start = g.dragStartPosition;
		committed = position.x != start.x || position.y != start.y || scale != g.dragStartScale;
		g.mouseConsumed = g.modal;
		g.active = GizmoHandle_None;
	}
	else
	{
		const float2 delta = mouseWorld - g.dragStartMouse;
		const float3 start = Gizmo_Snap(g.dragStartPosition, snap);

		switch ( g.active )
		{
			case GizmoHandle_MoveXY:
				position.x = start.x + Gizmo_Snap(delta.x, snap);
				position.y = start.y + Gizmo_Snap(delta.y, snap);
				break;
			case GizmoHandle_MoveX:
				position.x = start.x + Gizmo_Snap(delta.x, snap);
				break;
			case GizmoHandle_MoveY:
				position.y = start.y + Gizmo_Snap(delta.y, snap);
				break;
			case GizmoHandle_Scale:
			{
				const float2 center = g.dragStartPosition.xy;
				const f32 startDistance = Length(g.dragStartMouse - center);
				const f32 distance = Length(mouseWorld - center);
				const f32 ratio = startDistance > 0.0f ? distance / startDistance : 1.0f;
				const f32 snapped = Gizmo_Snap(g.dragStartScale * ratio, scaleSnap);
				scale = scaleSnap > 0.0f ? Max(snapped, scaleSnap) : snapped;
				break;
			}
			default:
				INVALID_CODE_PATH();
		}
	}

	// Drawn from the position as it is after this frame's drag
	const float2 o = Gizmo_WorldToScreen(g.view, position.xy);
	const float2 x = o + float2{GIZMO_AXIS_LENGTH, 0.0f};
	const float2 y = o - float2{0.0f, GIZMO_AXIS_LENGTH};
	const float2 s = o + float2{GIZMO_SCALE_OFFSET, -GIZMO_SCALE_OFFSET};
	const float4 colorX = Gizmo_HandleColor(g, GizmoHandle_MoveX, GizmoColorX);
	const float4 colorY = Gizmo_HandleColor(g, GizmoHandle_MoveY, GizmoColorY);

	const rect viewportRect = { .pos = {0, 0}, .size = g.view.viewportSize };
	UI_PushDrawList(ui, viewportRect, ui.fontAtlasH);
	// Highest layer sorts first: the gizmo draws under every window
	UI_GetDrawList(ui).sortKey.layer = U16_MAX;

	Gizmo_AddRect(ui, o - float2{0.0f, halfThickness}, x + float2{0.0f, halfThickness}, colorX);
	UI_AddTriangle(ui, x - float2{0.0f, halfArrow}, x + float2{0.0f, halfArrow}, x + float2{GIZMO_ARROW_SIZE, 0.0f}, colorX);

	Gizmo_AddRect(ui, y - float2{halfThickness, 0.0f}, o + float2{halfThickness, 0.0f}, colorY);
	UI_AddTriangle(ui, y + float2{halfArrow, 0.0f}, y - float2{0.0f, GIZMO_ARROW_SIZE}, y - float2{halfArrow, 0.0f}, colorY);

	Gizmo_AddRect(ui, s - float2{halfScale, halfScale}, s + float2{halfScale, halfScale}, Gizmo_HandleColor(g, GizmoHandle_Scale, GizmoColorScale));
	Gizmo_AddRect(ui, o - float2{halfCenter, halfCenter}, o + float2{halfCenter, halfCenter}, Gizmo_HandleColor(g, GizmoHandle_MoveXY, GizmoColorXY));

	UI_PopDrawList(ui);

	return committed;
}

#endif // #ifndef ILU_GIZMO_H

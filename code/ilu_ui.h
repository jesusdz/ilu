/*
 * ilu_ui.h
 * Author: Jesus Diaz Garcia
 *
 * Immediate mode UI API.
 */

#ifndef ILU_UI_H
#define ILU_UI_H

#ifndef ILU_GFX_H
#error "ilu_gfx.h needs to be defined before ilu_ui.h"
#endif

#define UI_VERTEX_BUFFER_SIZE KB(512)
#define UI_TEMP_STRING_SIZE KB(8)

// Fixed capacities. All of them are hard limits guarded by ASSERTs at their
// push sites, so raising a ceiling is a matter of bumping the value here.
#define UI_MAX_WINDOWS 64
#define UI_MAX_WINDOW_SECTIONS 16
#define UI_MAX_WINDOW_CAPTION 64
#define UI_MAX_WIDGET_INFOS 1024
#define UI_MAX_DRAW_LISTS 128
// A draw list gains one range per nested draw list popped inside it, so a single
// parent can need as many ranges as there are draw lists in the frame.
#define UI_MAX_DRAW_LIST_VERTEX_RANGES UI_MAX_DRAW_LISTS
#define UI_MAX_LAYOUT_GROUPS 16
#define UI_MAX_ELEMENT_COLOR_STACK 8
#define UI_MAX_COLOR_STACK 16
#define UI_MAX_CURSOR_STACK 16
#define UI_MAX_INDENT_STACK 16
#define UI_MAX_PADDING_STACK 16
#define UI_MAX_WINDOW_STACK 16
#define UI_MAX_MODAL_WINDOW_STACK 16
#define UI_MAX_ID_STACK 16
#define UI_MAX_WIDGET_STACK 16
#define UI_MAX_TABLES 16
#define UI_MAX_TABLE_STACK 4
#define UI_MAX_TABLE_COLUMNS 8
#define UI_MAX_TABLE_COLUMN_LABEL 32
#define UI_FONT_CHAR_COUNT 255
#define UI_TEXT_BUFFER_SIZE 128
#define UI_LABEL_BUFFER_SIZE 128

#define UI_VSPRINTF(format, text) \
	va_list vaList; \
	va_start(vaList, format); \
	VSNPrintf(ui.tempString, UI_TEMP_STRING_SIZE, format, vaList); \
	va_end(vaList); \
	const char *text = ui.tempString;

// Literal ink, not theme slots: glyph shadows, checkmarks, arrows and text
// cursors are always drawn in plain black or white. Everything themeable lives
// in UIStyle below and is reached through ui.style.
constexpr float4 UiColorWhite = { 1.0, 1.0, 1.0, 1.0 };
constexpr float4 UiColorDarkGray = { 0.1, 0.1, 0.1, 1.0 };
constexpr float4 UiColorGray = { 0.5, 0.5, 0.5, 1.0 };
constexpr float4 UiColorBlack = { 0.0, 0.0, 0.0, 1.0 };

// Default theme palette. Only UI_StyleDefault() reads these; widgets must not.
constexpr f32 CR = 0.05;
constexpr f32 CG = 0.15;
constexpr f32 CB = 0.3;

constexpr float4 UiColorAccent = { 1.0, 0.6, 0.0, 1.0 };
constexpr float4 UiColorBorder = { 0.1, 0.1, 0.1, 0.9 };
constexpr float4 UiColorCaption = { CR, CG, CB, 1.0 };
constexpr float4 UiColorCaptionInactive = { 0.05, 0.05, 0.05, 1.0 };
constexpr float4 UiColorBackground = { 0.02, 0.02, 0.02, 0.97 };

constexpr float4 UiColorSection = { CR, CG, CB, 1.0 };
constexpr float4 UiColorSectionHover = { 2*CR, 2*CG, 2*CB, 1.0 };

constexpr float4 UiColorButton = { CR, CG, CB, 1.0 };
constexpr float4 UiColorButtonHover = { 2*CR, 2*CG, 2*CB, 1.0 };

constexpr float4 UiColorToggle = { CR, CG, CB, 1.0 };
constexpr float4 UiColorToggleHover = { 2*CR, 2*CG, 2*CB, 1.0 };

constexpr float4 UiColorInput = { CR, CG, CB, 1.0 };
constexpr float4 UiColorInputHover = { 2 * CR, 2 * CG, 2 * CB, 1.0 };

constexpr float4 UiColorBox = { 0.06, 0.06, 0.06, 1.0 };
constexpr float4 UiColorBoxHover = { 0.08, 0.08, 0.08, 1.0 };

constexpr float4 UiColorScrollbar = { 0.1, 0.1, 0.1, 1.0 };
constexpr float4 UiColorScrollbarHover = { 0.2, 0.2, 0.2, 1.0 };

constexpr float4 UiColorMenu = { 0.02, 0.02, 0.02, 1.0 };
constexpr float4 UiColorMenuHover = { 2*CR, 2*CG, 2*CB, 1.0 };

constexpr float4 UiColorTableHeader = { CR, CG, CB, 1.0 };
constexpr float4 UiColorTableHeaderHover = { 2*CR, 2*CG, 2*CB, 1.0 };

// Rows are transparent by default: only hovered and selected ones paint.
constexpr float4 UiColorTableRow = { 0.0, 0.0, 0.0, 0.0 };
constexpr float4 UiColorTableRowHover = { CR, CG, CB, 1.0 };
constexpr float4 UiColorTableRowSelected = { 2*CR, 2*CG, 2*CB, 1.0 };

constexpr float4 UiColorMeter = { 0.06, 0.06, 0.06, 1.0 };
constexpr float4 UiColorMeterFill = { 2*CR, 2*CG, 2*CB, 1.0 };
constexpr float4 UiColorMeterFillHover = { 3*CR, 3*CG, 3*CB, 1.0 };

// Sentinel for spacing arguments: "use whatever the current style says". A
// default argument cannot read ui.style (ui is itself a parameter), so callers
// that want the styled value fall through this instead.
constexpr f32 UI_StyleSpacing = -1.0f;

// Identifies a window / widget / stored piece of state. Derived from a label hash
// combined with the enclosing window and ID stack. Zero means "no ID".
typedef u32 UIID;

struct UIVertex
{
	float2 position;
	float2 texCoord;
	rgba color;
};

struct UIInput
{
	Mouse mouse;
	Keyboard keyboard;
	Chars chars;
	int2 lastMouseClickPos;
};

struct UISection
{
	u32 hash;
	bool open;
};

enum UIWindowFlags
{
	UIWindowFlag_None = 0,
	UIWindowFlag_Draggable = 1 << 0,
	UIWindowFlag_Resizable = 1 << 1,
	UIWindowFlag_Titlebar = 1 << 2,
	UIWindowFlag_Background = 1 << 3,
	UIWindowFlag_Border = 1 << 4,
	UIWindowFlag_ClipContents = 1 << 5,
	UIWindowFlag_CloseButton = 1 << 6,
	UIWindowFlag_NoRaise = 1 << 7,
};

enum UIModalFlags
{
	UIModalFlag_Default = 0,
	UIModalFlag_NoBackground = 1 << 0,
};

struct UIWindow
{
	UIID id;
	i32 index;
	char caption[UI_MAX_WINDOW_CAPTION];
	float2 pos;
	float2 size;
	float2 anchor;
	float2 pivot;
	float2 displacement;
	float2 containerPos;
	float2 containerSize;
	float2 contentSize; // Only known after filling container
	u32 contentLayoutGroup; // Index of the layout group wrapping the window contents
	float contentOffset;
	float contentOffsetBeforeScrolling;
	bool visible;
	bool dragging;
	bool resizing;
	bool scrolling;
	bool wasMenuOpen;
	int2 sizeBeforeResize;
	bool disableWidgets;
	bool clippedContents;
	u32 layer;
	u32 flags;
	u32 modalFlags;

	UISection sections[UI_MAX_WINDOW_SECTIONS];
	u32 sectionCount;
};

enum UIWidgetFlags
{
	UIWidgetFlag_None = 0,
	UIWidgetFlag_Outline = (1<<0),
	UIWidgetFlag_Expand = (1<<1),
	UIWidgetFlag_Centered = (1<<2),
};

struct UIWidget
{
	float2 pos;
	float2 size;
};

enum UILayout
{
	UILayout_Vertical,
	UILayout_Horizontal,
	UILayout_ItemBrowser,
};

struct UILayoutGroup
{
	UILayout layout;
	float2 pos;
	float2 size;
};

struct UIVertexRange
{
	u32 index;
	u32 count;
};

struct UISortKey
{
	u16 layer;
	u16 order;
};

enum UIDrawListFlags
{
	UIDrawListFlag_None = 0,
	UIDrawListFlag_Topmost = 1 << 0,
};

struct UIDrawList
{
	rect scissorRect;
	UIVertexRange vertexRanges[UI_MAX_DRAW_LIST_VERTEX_RANGES];
	u32 vertexRangeCount;
	ImageH imageHandle;
	UISortKey sortKey;
};

struct UICombo
{
	UIID id;
	float2 pos;
	float2 size;
};

struct UIColorPicker
{
	UIID id;
};

struct UIIcon
{
	ImagePixels image;

	// Known after packed into the atlas
	int2 pos;
	float2 uv;
	float2 uvSize;
};

union UIPayload
{
	void *ptr;
	u32 uvalue;
};

struct UIDragAndDrop
{
	const char *payloadType;
	UIPayload payload;
	ImageH imageH;
	float4 uvRect;
};

struct UIElementColor
{
	float4 base;
	float4 hovered;
	float4 active;
	float4 inactive;
};

struct UIElementColorStack
{
	UIElementColor stack[UI_MAX_ELEMENT_COLOR_STACK];
	uint stackSize;
};

enum UIElement
{
	UIElementText,
	UIElementBackground,
	UIElementSection,
	UIElementButton,
	UIElementToggle,
	UIElementInput,
	UIElementBox,
	UIElementScrollbar,
	UIElementMenu,
	UIElementCaption,
	UIElementTableHeader,
	UIElementTableRow,
	UIElementMeter, // base is the track, active the fill, hovered the fill under the mouse
	UIElementCount,
};

// The theme: every colour and metric a widget is allowed to depend on. Widgets
// read it through ui.style (or UI_GetElemColor / UI_GetPadding / UI_GetSpacing,
// which layer the temporary override stacks on top), never from file-scope
// constants. Adding a themeable value means adding a field here and a default
// in UI_StyleDefault, not hunting for a literal in a widget body.
struct UIStyle
{
	// Colours
	UIElementColor colors[UIElementCount];
	float4 borderColor;
	float4 accentColor;   // Selection outlines, histogram bars, canvas boxes
	float4 modalOverlayColor;

	// Metrics
	float2 borderSize;
	float2 windowPadding;
	float2 framePadding;  // Inside one-line controls, between the box and its text
	float2 minWindowSize;
	f32 itemSpacing;
	f32 itemSpacingTight; // Between stacked components of a vector control
	f32 indentWidth;
	f32 titlebarHeight;
	f32 menuBarHeight;
	f32 scrollbarWidth;
	f32 scrollSpeed;      // Pixels advanced per mouse wheel notch
	f32 resizeCornerSize;
	f32 dragClickThreshold; // Max drag distance (px) still counted as a click
	f32 labelRatio;       // Fraction of the container a labelled control's box takes
	f32 tableGripWidth;   // Grab area around a table column divider
	f32 tableMinColumnWidth;
};

struct UIInfo
{
	UIID id;
	bool isOpen;
};

enum UINextWindowBits
{
	UINextWindow_None = 0,
	UINextWindow_Size = 1 << 0,
	UINextWindow_AnchorAndPivot = 1 << 1,
	UINextWindow_Displacement = 1 << 2,
	UINextWindow_Modal = 1 << 3,
};

// One-shot overrides applied to the next UI_BeginWindow and cleared by it.
// Adding a new override means adding a bit plus a field, not another bool pair.
struct UINextWindow
{
	u32 setMask;
	float2 size;
	float2 anchor;
	float2 pivot;
	float2 displacement;
	u32 modalFlags;
};

enum UITreeNodeFlags
{
	UITreeNodeFlag_None = 0,
	UITreeNodeFlag_Leaf = 1 << 0,
};

enum UITableFlags
{
	UITableFlag_None = 0,
	UITableFlag_Resizable = 1 << 0, // Column dividers in the header can be dragged
	UITableFlag_RowHighlight = 1 << 1, // Paint the row under the mouse
	UITableFlag_Default = UITableFlag_Resizable | UITableFlag_RowHighlight,
};

enum UITableColumnSizing
{
	UITableColumnSizing_Stretch, // Shares the leftover width, proportionally to its size value
	UITableColumnSizing_Fixed,   // Takes its size value in pixels
};

struct UITableColumn
{
	char label[UI_MAX_TABLE_COLUMN_LABEL];
	UITableColumnSizing sizing;
	f32 size;   // Weight when stretching, pixels when fixed
	f32 width;  // Resolved width, in pixels
	f32 offset; // Resolved offset from the left edge of the table, in pixels
};

// The part of a table that outlives the frame: widths the user dragged the
// columns to, plus the drag in progress. Everything else is rebuilt every frame.
struct UITableState
{
	UIID id;
	u32 columnCount;
	f32 userWidths[UI_MAX_TABLE_COLUMNS]; // 0 means "the column was never resized"
	i32 resizedColumn;                    // -1 when no divider is being dragged
	f32 widthBeforeResize;
};

// A table being filled in, from UI_BeginTable to UI_EndTable. Rows are laid out
// top to bottom and cells left to right; a cell is a layout group of its own, so
// any widget can be placed inside one.
struct UITable
{
	UIID id;
	UITableState *state;
	u32 flags;
	float2 pos;
	f32 width;
	f32 height;      // Grown by the header and every row added so far
	f32 rowHeight;
	UITableColumn columns[UI_MAX_TABLE_COLUMNS];
	u32 columnCount; // Announced in UI_BeginTable
	u32 setupCount;  // Columns described through UI_TableSetupColumn so far
	bool resolved;   // Column widths already computed for this frame
	i32 rowIndex;
	i32 columnIndex;
	float2 rowPos;
	bool rowOpen;
	bool cellOpen;

	// Window state a cell replaces so that widgets measuring themselves against
	// the container end up measuring themselves against the cell instead
	float2 containerSizeBackup;
	u32 contentLayoutGroupBackup;
};

constexpr const char *UIElementName[] =
{
	"Text",
	"Background",
	"Section",
	"Button",
	"Toggle",
	"Input",
	"Box",
	"Scrollbar",
	"Menu",
	"Caption",
	"TableHeader",
	"TableRow",
	"Meter",
};

CT_ASSERT(ARRAY_COUNT(UIElementName) == UIElementCount);

struct UI
{
	u32 frameIndex;

	UIVertex *frontendVertices;

	BufferH vertexBuffer[MAX_FRAMES_IN_FLIGHT];
	UIVertex *backendVertices[MAX_FRAMES_IN_FLIGHT];
	UIVertex *vertexPtr;
	u32 vertexCount;
	u32 vertexCountLimit;
	bool vertexOverflow;

	UIDrawList drawLists[UI_MAX_DRAW_LISTS];
	u32 drawListCount;

	u32 drawListStack[UI_MAX_DRAW_LISTS];
	u32 drawListStackSize;

	ImageH fontAtlasH;
	float2 fontAtlasSize;
	float fontScale, fontAscent, fontDescent, fontLineGap;

	float2 whitePixelUv;

	stbtt_packedchar charData[UI_FONT_CHAR_COUNT];

	UIStyle style;

	float4 colors[UI_MAX_COLOR_STACK];
	u32 colorCount;

	// Temporary per-element overrides layered on top of ui.style.colors.
	// An empty stack means "use the theme".
	UIElementColorStack colorElems[UIElementCount];

	float2 cursorStack[UI_MAX_CURSOR_STACK];
	u32 cursorStackSize;

	f32 indentStack[UI_MAX_INDENT_STACK];
	u32 indentStackSize;

	float2 paddingStack[UI_MAX_PADDING_STACK];
	u32 paddingStackSize;

	UIInput input;
	uint2 viewportSize;

	UIWindow windows[UI_MAX_WINDOWS];
	u32 windowCount;

	u32 windowStack[UI_MAX_WINDOW_STACK];
	u32 windowStackSize;

	UIWindow *modalWindowStack[UI_MAX_MODAL_WINDOW_STACK];
	u32 modalWindowStackSize;

	UIID idStack[UI_MAX_ID_STACK];
	u32 idStackSize;

	UIWindow *activeWindow;
	UIWindow *hoveredWindow;

	float2 defaultWindowDisplacement;
	float2 defaultWindowSize;

	UINextWindow nextWindow;

	UIWidget widgetStack[UI_MAX_WIDGET_STACK];
	u32 widgetStackSize;

	UIID activeWidgetId;

	// Tracks the widget under the mouse when the left button went down, so a
	// click only fires when the button is released over that same widget.
	bool widgetPressActive;
	float2 pressedWidgetPos;
	float2 pressedWidgetSize;

	float2 lastWidgetPos;
	float2 lastWidgetSize;

	UILayoutGroup layoutGroups[UI_MAX_LAYOUT_GROUPS];
	u32 layoutGroupCount;

	UICombo comboBox;
	UIColorPicker colorPicker;

	bool avoidWindowInteraction;
	bool wantsInput;

	UIIcon *icons;
	u32 iconCount;

	char *tempString;

	bool menuBarBegan;
	bool toolBarBegan;
	UIWindow *activeMenu;
	UIVertex *activeMenuVertexPtr;

	UIDragAndDrop dragAndDrop;

	UIInfo info[UI_MAX_WIDGET_INFOS];
	u32 infoCount;

	UITableState tableStates[UI_MAX_TABLES];
	u32 tableStateCount;

	UITable tableStack[UI_MAX_TABLE_STACK];
	u32 tableStackSize;
};

static const char *UI_RemoveNamePrefix(const char *label)
{
	const char *res = StrChar(label, '#');
	return res ? res + 1 : label;
}

static void UI_SetNextWindowDefaultSize(UI &ui, uint2 size)
{
	ui.defaultWindowSize = {(f32)size.x, (f32)size.y};
}

static void UI_SetNextWindowSize(UI &ui, uint2 size)
{
	ui.nextWindow.size = {(f32)size.x, (f32)size.y};
	ui.nextWindow.setMask |= UINextWindow_Size;
}

static void UI_SetNextWindowDefaultDisplacement(UI &ui, float2 disp)
{
	ui.defaultWindowDisplacement = disp;
}

static void UI_SetNextWindowDisplacement(UI &ui, float2 displacement)
{
	ui.nextWindow.displacement = displacement;
	ui.nextWindow.setMask |= UINextWindow_Displacement;
}

static void UI_SetNextWindowAnchorAndPivot(UI &ui, float2 anchor, float2 pivot)
{
	ui.nextWindow.anchor = anchor;
	ui.nextWindow.pivot = pivot;
	ui.nextWindow.setMask |= UINextWindow_AnchorAndPivot;
}

static void UI_SetNextWindowAnchor(UI &ui, float2 anchor)
{
	UI_SetNextWindowAnchorAndPivot(ui, anchor, anchor);
}

static void UI_ResetWindowDefaults(UI &ui)
{
	ui.defaultWindowSize = {200, 300};
	ui.defaultWindowDisplacement = {0, 0};
}

static float2 UI_GetAnchorPos(uint2 viewportSize, float2 anchor)
{
	const float2 anchorPos = Floor(anchor * float2{(f32)viewportSize.x, (f32)viewportSize.y});
	return anchorPos;
}

static float2 UI_GetPivotDisplacement(float2 windowSize, float2 pivot)
{
	const float2 pivotDisplacement = -1.0f * pivot * windowSize;
	return pivotDisplacement;
}

static void UI_PositionWindow(UIWindow &window, uint2 viewportSize, float2 windowSize, float2 anchor, float2 displacement)
{
	const float2 anchorPos = UI_GetAnchorPos(viewportSize, anchor);
	const float2 pivotDisp = UI_GetPivotDisplacement(windowSize, window.pivot);
	const float2 windowPos = anchorPos + pivotDisp + displacement;
	window.pos = windowPos;
}

static void UI_PositionWindow(UIWindow &window, float2 position)
{
	window.pos = position;
	window.anchor = {0, 0};
	window.pivot = {0, 0};
	window.displacement = position;
}

bool UI_IsMousePressWithAnyButton(const UI &ui)
{
	const bool press =
		ui.input.mouse.buttons[MOUSE_BUTTON_LEFT] == BUTTON_STATE_PRESS ||
		ui.input.mouse.buttons[MOUSE_BUTTON_MIDDLE] == BUTTON_STATE_PRESS ||
		ui.input.mouse.buttons[MOUSE_BUTTON_RIGHT] == BUTTON_STATE_PRESS;
	return press;
}

bool UI_IsMousePress(const UI &ui, MouseButton button)
{
	const bool press = ui.input.mouse.buttons[button] == BUTTON_STATE_PRESS;
	return press;
}

bool UI_IsMousePress(const UI &ui)
{
	const bool press = UI_IsMousePress(ui, MOUSE_BUTTON_LEFT);
	return press;
}

bool UI_IsMouseRelease(const UI &ui, MouseButton button)
{
	const bool release = ui.input.mouse.buttons[button] == BUTTON_STATE_RELEASE;
	return release;
}

bool UI_IsMouseRelease(const UI &ui)
{
	const bool release = UI_IsMouseRelease(ui, MOUSE_BUTTON_LEFT);
	return release;
}

bool UI_IsMouseIdle(const UI &ui)
{
	const bool idle = ui.input.mouse.buttons[MOUSE_BUTTON_LEFT] == BUTTON_STATE_IDLE;
	return idle;
}

bool UI_IsHovered(const UI &ui)
{
	const bool res = ui.hoveredWindow != nullptr;
	return res;
}

int2 UI_MouseScroll(const UI &ui)
{
	return ui.input.mouse.wheel;
}

int2 UI_MousePos(const UI &ui)
{
	return ui.input.mouse.pos;
}

int2 UI_LastMouseClickPos(const UI &ui)
{
	const int2 pos = { .x = ui.input.lastMouseClickPos.x, .y = ui.input.lastMouseClickPos.y };
	return pos;
}

int2 UI_LastMouseClickPosInWidget(const UI &ui)
{
	const int2 widgetPos = { .x = (i32)ui.lastWidgetPos.x, .y = (i32)ui.lastWidgetPos.y };
	const int2 mousePos = UI_LastMouseClickPos(ui);
	const int2 relativePos = mousePos - widgetPos;
	return relativePos;
}

float2 UI_LastMouseClickPosInWidgetNormalized(const UI &ui)
{
	const int2 relativePos = UI_LastMouseClickPosInWidget(ui);
	const float2 normalizedPos = {
		.x = (f32)relativePos.x / ui.lastWidgetSize.x,
		.y = (f32)relativePos.y / ui.lastWidgetSize.y,
	};
	return normalizedPos;
}

UIWindow &UI_GetCurrentWindow(UI &ui)
{
	ASSERT(ui.windowStackSize > 0);
	const u32 windowIndex = ui.windowStack[ui.windowStackSize-1];
	return ui.windows[windowIndex];
}

const UIWindow &UI_GetCurrentWindow(const UI &ui)
{
	ASSERT(ui.windowStackSize > 0);
	const u32 windowIndex = ui.windowStack[ui.windowStackSize-1];
	return ui.windows[windowIndex];
}

float2 UI_GetCursorPos(const UI &ui)
{
	ASSERT(ui.cursorStackSize > 0);
	const float2 cursorPos = ui.cursorStack[ui.cursorStackSize-1];
	return cursorPos;
}

void UI_PushCursorPos(UI &ui, float2 pos)
{
	ASSERT(ui.cursorStackSize < ARRAY_COUNT(ui.cursorStack));
	ui.cursorStack[ui.cursorStackSize++] = pos;
}

void UI_PopCursorPos(UI &ui)
{
	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStackSize--;
}

void UI_SetCursorPos(UI &ui, float2 pos)
{
	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStack[ui.cursorStackSize-1] = pos;
}

void UI_SetCursorPosX(UI &ui, f32 x)
{
	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStack[ui.cursorStackSize-1].x = x;
}

void UI_SetCursorPosXFromRight(UI &ui, f32 x)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);

	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStack[ui.cursorStackSize-1].x = window.pos.x + window.size.x - x;
}

void UI_MoveCursorDown(UI &ui, float amount)
{
	float2 cursorPos = UI_GetCursorPos(ui);
	cursorPos.y += amount;
	UI_SetCursorPos(ui, cursorPos);
}

void UI_MoveCursorRight(UI &ui, float amount)
{
	float2 cursorPos = UI_GetCursorPos(ui);
	cursorPos.x += amount;
	UI_SetCursorPos(ui, cursorPos);
}

// Resolve a spacing/indent argument that may be the UI_StyleSpacing sentinel.
f32 UI_GetSpacing(const UI &ui, f32 spacing)
{
	return spacing < 0.0f ? ui.style.itemSpacing : spacing;
}

f32 UI_GetIndentWidth(const UI &ui, f32 amount)
{
	return amount < 0.0f ? ui.style.indentWidth : amount;
}

void UI_Indent(UI &ui, f32 amount = UI_StyleSpacing)
{
	ASSERT(ui.indentStackSize < ARRAY_COUNT(ui.indentStack));
	ui.indentStack[ui.indentStackSize++] = UI_GetCursorPos(ui).x;
	UI_MoveCursorRight(ui, UI_GetIndentWidth(ui, amount));
}

void UI_Unindent(UI &ui)
{
	ASSERT(ui.indentStackSize > 0);
	UI_SetCursorPosX(ui, ui.indentStack[--ui.indentStackSize]);
}

UILayout UI_GetLayout(const UI &ui)
{
	ASSERT(ui.layoutGroupCount > 0);
	const UILayout layout = ui.layoutGroups[ui.layoutGroupCount - 1].layout;
	return layout;
}

UILayoutGroup &UI_GetLayoutGroup(UI &ui)
{
	ASSERT(ui.layoutGroupCount > 0);
	UILayoutGroup &group = ui.layoutGroups[ui.layoutGroupCount - 1];
	return group;
}

void UI_GrowLayoutGroup(UI &ui, float2 pos, float2 size)
{
	UILayoutGroup &group = UI_GetLayoutGroup(ui);
	const float2 maxPoint = pos + size;
	const float2 maxLayoutPoint = group.pos + group.size;
	if (maxPoint.x > maxLayoutPoint.x) {
		group.size.x = maxPoint.x - group.pos.x;
	}
	if (maxPoint.y > maxLayoutPoint.y) {
		group.size.y = maxPoint.y - group.pos.y;
	}
}

const UIDrawList &UI_GetDrawList(const UI &ui);

void UI_CursorAdvance(UI &ui, float2 prevWidgetSize, float spacingArg = UI_StyleSpacing)
{
	UI_GrowLayoutGroup(ui, UI_GetCursorPos(ui), prevWidgetSize);

	const f32 spacing = UI_GetSpacing(ui, spacingArg);
	const UILayout layout = UI_GetLayout(ui);
	const UILayoutGroup group = UI_GetLayoutGroup(ui);

	if ( layout == UILayout_Horizontal )
	{
		UI_MoveCursorRight(ui, prevWidgetSize.x + spacing);
	}
	else if ( layout == UILayout_Vertical )
	{
		UI_MoveCursorDown(ui, prevWidgetSize.y + spacing);
	}
	else if ( layout == UILayout_ItemBrowser )
	{
		const UIDrawList &drawlist = UI_GetDrawList(ui);
		const f32 prevWidgetX = UI_GetCursorPos(ui).x;
		const f32 nextWidgetEnd = prevWidgetX + prevWidgetSize.x * 2.0f;
		const f32 contentEnd = drawlist.scissorRect.pos.x + drawlist.scissorRect.size.x;
		const bool spaceAtRight = nextWidgetEnd < contentEnd;

		if ( spaceAtRight )
		{
			UI_MoveCursorRight(ui, prevWidgetSize.x + spacing);
		}
		else
		{
			UI_SetCursorPosX(ui, group.pos.x);
			UI_MoveCursorDown(ui, prevWidgetSize.y + spacing);
		}
	}
}

void UI_BeginLayout(UI &ui, UILayout layout)
{
	ASSERT(ui.layoutGroupCount < ARRAY_COUNT(ui.layoutGroups));
	UILayoutGroup &group = ui.layoutGroups[ui.layoutGroupCount++];
	group.layout = layout;
	group.pos = UI_GetCursorPos(ui);
	group.size = {};
}

void UI_EndLayout(UI &ui, bool growParent = true)
{
	ASSERT(ui.layoutGroupCount > 0);

	ui.layoutGroupCount--;

	if ( ui.layoutGroupCount && growParent )
	{
		const UILayoutGroup &endedGroup = ui.layoutGroups[ui.layoutGroupCount];
		UI_SetCursorPos(ui, endedGroup.pos);
		UI_CursorAdvance(ui, endedGroup.size);
	}
}

void UI_SetInputState(UI &ui, const Keyboard &keyboard, const Mouse &mouse, const Chars &chars)
{
	UIInput &input = ui.input;

	const Mouse prevMouse = input.mouse;

	input.mouse = mouse;
	input.keyboard = keyboard;
	input.chars = chars;

	// Workaround to avoid missing some dx,dy updates...
	input.mouse.delta = mouse.pos - prevMouse.pos;
}

void UI_SetViewportSize(UI &ui, uint2 size)
{
	ui.viewportSize = size;
}

u32 UI_DrawListCount(const UI &ui)
{
	return ui.drawListCount;
}

const UIDrawList &UI_GetDrawListAt(const UI &ui, u32 i)
{
	ASSERT(i < ui.drawListCount);
	return ui.drawLists[i];
}

UIDrawList &UI_GetDrawList(UI &ui)
{
	ASSERT(ui.drawListCount > 0 && ui.drawListStackSize > 0);
	const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
	return ui.drawLists[drawListIndex];
}

const UIDrawList &UI_GetDrawList(const UI &ui)
{
	ASSERT(ui.drawListCount > 0 && ui.drawListStackSize > 0);
	const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
	return ui.drawLists[drawListIndex];
}

void UI_PushDrawList(UI &ui, rect scissorRect, ImageH imageHandle, UIDrawListFlags flags = UIDrawListFlag_None)
{
	ASSERT(ui.drawListCount < ARRAY_COUNT(ui.drawLists));

	const u32 drawListIndex = ui.drawListCount++;

	UIDrawList &drawList = ui.drawLists[drawListIndex];
	drawList.vertexRanges[0].index = ui.vertexCount;
	drawList.vertexRanges[0].count = 0;
	drawList.vertexRangeCount = 1;
	drawList.scissorRect = scissorRect;
	drawList.imageHandle = imageHandle;

	drawList.sortKey.order = ( flags & UIDrawListFlag_Topmost ) ? U32_MAX : drawListIndex;
	if (ui.drawListStackSize > 0)
	{
		const u32 parentDrawListIndex = ui.drawListStack[ui.drawListStackSize-1];
		drawList.sortKey.layer = ui.drawLists[parentDrawListIndex].sortKey.layer;
	}

	ui.drawListStack[ui.drawListStackSize++] = drawListIndex;
}

void UI_PushDrawList(UI &ui, ImageH imageHandle)
{
	const UIDrawList &currDrawList = UI_GetDrawList(ui);
	UI_PushDrawList(ui, currDrawList.scissorRect, imageHandle);
}

void UI_PopDrawList(UI &ui)
{
	ASSERT(ui.drawListStackSize > 0);

	// Remove empty ranges in the draw list before popping
	const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
	UIDrawList &drawList = ui.drawLists[drawListIndex];
	ASSERT(drawList.vertexRangeCount > 0);
	UIVertexRange &range = drawList.vertexRanges[drawList.vertexRangeCount-1];
	if (range.count == 0)
	{
		drawList.vertexRangeCount--;
	}

	ui.drawListStackSize--;

	if (ui.drawListStackSize > 0)
	{
		// Setup the next range in the parent draw list
		const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
		UIDrawList &drawList = ui.drawLists[drawListIndex];
		ASSERT(drawList.vertexRangeCount > 0);
		UIVertexRange &range = drawList.vertexRanges[drawList.vertexRangeCount-1];
		if (range.count == 0)
		{
			// Reuse range, it was empty
			range.index = ui.vertexCount;
		}
		else
		{
			// Previous range was not empty, create a new one
			ASSERT(drawList.vertexRangeCount < ARRAY_COUNT(drawList.vertexRanges));
			UIVertexRange &nextRange = drawList.vertexRanges[drawList.vertexRangeCount++];
			nextRange.index = ui.vertexCount;
			nextRange.count = 0;
		}
	}

}

// Merges each draw list's scattered vertex ranges into one contiguous run in the
// mapped GPU buffer, leaving every draw list with a single range. Call once all
// draw lists for the frame are closed.
void UI_FinalizeDrawData(UI &ui)
{
	const UIVertex *srcVertexBase = ui.frontendVertices;
	UIVertex *dstVertexBase = ui.backendVertices[ui.frameIndex];
	u32 totalVertexCount = 0;

	// Merge vertex ranges and copy draw list vertices to GPU
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		UIDrawList &drawList = ui.drawLists[i];
		u32 drawListVertexCount = 0;

		// Copy merged vertex ranges into a contiguous chunk of GPU memory
		for (u32 j = 0; j < drawList.vertexRangeCount; ++j)
		{
			const UIVertexRange &range = drawList.vertexRanges[j];
			const UIVertex *srcVertex = srcVertexBase + range.index;
			UIVertex *dstVertex = dstVertexBase + totalVertexCount;
			MemCopy(dstVertex, srcVertex, range.count * sizeof(UIVertex));
			drawListVertexCount += range.count;
			totalVertexCount += range.count;
		}

		// Modify draw list range info (now there's only one range)
		drawList.vertexRangeCount = 1;
		drawList.vertexRanges[0].index = totalVertexCount - drawListVertexCount;
		drawList.vertexRanges[0].count = drawListVertexCount;
	}

	ASSERT(ui.vertexCount == totalVertexCount);

}

BufferH UI_GetVertexBuffer(const UI& ui)
{
	return ui.vertexBuffer[ui.frameIndex];
}

bool UI_MouseInArea(const UI &ui, float2 pos, float2 size)
{
	const bool inArea =
			ui.input.mouse.pos.x >= pos.x &&
			ui.input.mouse.pos.x <  pos.x + size.x &&
			ui.input.mouse.pos.y >= pos.y &&
			ui.input.mouse.pos.y <  pos.y + size.y;
	return inArea;
}

void UI_RaiseWindow(UI &ui, UIWindow &window)
{
	// Push down all windows in front of the one to be raised
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		if (ui.windows[i].layer < window.layer)
		{
			ui.windows[i].layer++;
		}
	}

	// Finally put this window in front
	window.layer = 0;
}

void UI_RaiseWindow(UI &ui)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	UI_RaiseWindow(ui, window);
}

void UI_FocusWindow(UI &ui, UIWindow &window)
{
	ui.activeWindow = &window;
}

void UI_FocusWindow(UI &ui)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	UI_FocusWindow(ui, window);
}

bool UI_IsFocusedWindow(UI &ui)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	const bool res = ui.activeWindow == &window;
	return res;
}

bool UI_WindowHovered(const UI &ui)
{
	const UIWindow &currentWindow = UI_GetCurrentWindow(ui);
	const bool hovered = &currentWindow == ui.hoveredWindow;
	return hovered;
}

bool UI_WidgetHovered(const UI &ui, float2 widgetPos, float2 widgetSize)
{
	bool hover = false;
	const UIWindow &currentWindow = UI_GetCurrentWindow(ui);

	if (currentWindow.disableWidgets)
	{
		return false;
	}

	if ( UI_WindowHovered(ui) )
	{
		const rect scissor = UI_GetDrawList(ui).scissorRect;
		const float2 scissorPos = { (f32)scissor.pos.x, (f32)scissor.pos.y };
		const float2 scissorSize = { (f32)scissor.size.x, (f32)scissor.size.y };

		hover = UI_MouseInArea(ui, widgetPos, widgetSize) &&
			UI_MouseInArea(ui, scissorPos, scissorSize);
	}

	return hover;
}

bool UI_WidgetHovered(const UI &ui)
{
	bool hover = false;
	if ( ui.widgetStackSize > 0 )
	{
		const UIWidget &widget = ui.widgetStack[ui.widgetStackSize-1];
		hover = UI_WidgetHovered(ui, widget.pos, widget.size);
	}
	return hover;
}

bool UI_WidgetPressed(const UI &ui, float2 widgetPos, float2 widgetSize)
{
	const bool pressed = UI_IsMousePress(ui) && UI_WidgetHovered(ui, widgetPos, widgetSize);
	return pressed;
}

bool UI_WidgetPressed(const UI &ui)
{
	const bool pressed = UI_IsMousePress(ui) && UI_WidgetHovered(ui);
	return pressed;
}

bool UI_LastWidgetPressed(const UI &ui)
{
	const bool pressed = UI_WidgetPressed(ui, ui.lastWidgetPos, ui.lastWidgetSize);
	return pressed;
}

bool UI_LastWidgetPressed(const UI &ui, MouseButton button)
{
	const bool pressed = UI_IsMousePress(ui, button) && UI_WidgetHovered(ui, ui.lastWidgetPos, ui.lastWidgetSize);
	return pressed;
}

bool UI_WidgetClicked(UI &ui, float2 widgetPos, float2 widgetSize)
{
	const bool hovered = UI_WidgetHovered(ui, widgetPos, widgetSize);

	if ( UI_IsMousePress(ui) && hovered )
	{
		ui.widgetPressActive = true;
		ui.pressedWidgetPos = widgetPos;
		ui.pressedWidgetSize = widgetSize;
	}

	const bool releasedHere = UI_IsMouseRelease(ui) && hovered;
	const bool sameWidgetAsPress =
		ui.pressedWidgetPos.x == widgetPos.x && ui.pressedWidgetPos.y == widgetPos.y &&
		ui.pressedWidgetSize.x == widgetSize.x && ui.pressedWidgetSize.y == widgetSize.y;

	const bool clicked = ui.widgetPressActive && releasedHere && sameWidgetAsPress;
	return clicked;
}

bool UI_WidgetClicked(UI &ui)
{
	ASSERT(ui.widgetStackSize > 0);
	const UIWidget &widget = ui.widgetStack[ui.widgetStackSize-1];
	const bool clicked = UI_WidgetClicked(ui, widget.pos, widget.size);
	return clicked;
}

bool UI_LastWidgetClicked(UI &ui)
{
	const bool clicked = UI_WidgetClicked(ui, ui.lastWidgetPos, ui.lastWidgetSize);
	return clicked;
}

void UI_BeginWidget(UI &ui, float2 pos, float2 size, bool avoidWindowInteraction = true)
{
	ASSERT(ui.widgetStackSize < ARRAY_COUNT(ui.widgetStack));
	const UIWidget widget = {
		.pos = pos,
		.size = size,
	};
	ui.widgetStack[ui.widgetStackSize++] = widget;

	UI_GrowLayoutGroup(ui, pos, size);

	// Disable window interaction in case the widget was pressed
	const UIWindow &currentWindow = UI_GetCurrentWindow(ui);
	if ( !currentWindow.disableWidgets && UI_WidgetPressed(ui) )
	{
		ui.avoidWindowInteraction = avoidWindowInteraction;
	}
}

void UI_EndWidget(UI &ui)
{
	ASSERT(ui.widgetStackSize > 0);
	ui.lastWidgetPos = ui.widgetStack[ui.widgetStackSize-1].pos;
	ui.lastWidgetSize = ui.widgetStack[ui.widgetStackSize-1].size;
	ui.widgetStackSize--;
}

const UIWidget &UI_GetCurrentWidget(UI &ui)
{
	ASSERT(ui.widgetStackSize > 0);
	return ui.widgetStack[ui.widgetStackSize-1];
}

void UI_PushPadding(UI &ui, float2 padding)
{
	ASSERT(ui.paddingStackSize < ARRAY_COUNT(ui.paddingStack));
	ui.paddingStack[ui.paddingStackSize++] = padding;
}

void UI_PopPadding(UI &ui)
{
	ASSERT(ui.paddingStackSize > 0);
	ui.paddingStackSize--;
}

float2 UI_GetPadding(const UI &ui)
{
	const float2 padding = ui.paddingStackSize > 0 ?
		ui.paddingStack[ui.paddingStackSize-1] :
		ui.style.framePadding;
	return padding;
}

void UI_PushColor(UI &ui, float4 color)
{
	ASSERT(ui.colorCount < ARRAY_COUNT(ui.colors));
	ui.colors[ui.colorCount++] = color;
}

void UI_PopColor(UI &ui)
{
	ASSERT(ui.colorCount > 1); // Avoid popping the default color
	ui.colorCount--;
}

float4 UI_GetColor(const UI &ui)
{
	const float4 color = ui.colors[ui.colorCount-1];
	return color;
}

void UI_PushElemColor(UI &ui, UIElement elem, UIElementColor color)
{
	ASSERT(ui.colorElems[elem].stackSize < ARRAY_COUNT(ui.colorElems[0].stack));
	ui.colorElems[elem].stack[ui.colorElems[elem].stackSize++] = color;
}

void UI_PopElemColor(UI &ui, UIElement elem)
{
	ASSERT(ui.colorElems[elem].stackSize > 0);
	ui.colorElems[elem].stackSize--;
}

const UIElementColor &UI_GetElemColor(UI &ui, UIElement elem)
{
	const UIElementColorStack &overrides = ui.colorElems[elem];
	return overrides.stackSize > 0 ?
		overrides.stack[overrides.stackSize-1] :
		ui.style.colors[elem];
}

void UI_PushColor(UI &ui, UIElement elem)
{
	const UIElementColor &elemColor = UI_GetElemColor(ui, elem);
	const float4 color = UI_WidgetHovered(ui) ? elemColor.hovered : elemColor.base;
	UI_PushColor(ui, color);
}

void UI_AddTriangle(UI &ui, const UIVertex &v0, const UIVertex &v1, const UIVertex v2)
{
	if ( ui.vertexCount + 3 <= ui.vertexCountLimit )
	{
		*ui.vertexPtr++ = v0;
		*ui.vertexPtr++ = v1;
		*ui.vertexPtr++ = v2;
		ui.vertexCount += 3;

		UIDrawList &drawList = UI_GetDrawList(ui);
		UIVertexRange &vertexRange = drawList.vertexRanges[drawList.vertexRangeCount-1];
		vertexRange.count += 3;
	}
	else
	{
		ui.vertexOverflow = true;
	}
}

void UI_AddTriangle(UI &ui, float2 p0, float2 p1, float2 p2, float4 fcolor )
{
	const float2 uv = ui.whitePixelUv;
	const rgba color  = Rgba(fcolor);
	UI_AddTriangle(ui,
			UIVertex{ p0, uv, color },
			UIVertex{ p1, uv, color },
			UIVertex{ p2, uv, color });
}

void UI_AddTriangle(UI &ui, float2 p0, float2 p1, float2 p2)
{
	const float4 color = UI_GetColor(ui);
	UI_AddTriangle(ui, p0, p1, p2, color);
}

void UI_AddQuad(UI &ui, float2 pos, float2 size, float2 uv, float2 uvSize, float4 fcolorTop, float4 fcolorBot)
{
	UIDrawList &drawList = UI_GetDrawList(ui);

	const int2 containerMin = drawList.scissorRect.pos;
	const int2 containerMax = drawList.scissorRect.pos + drawList.scissorRect.size;
	const float2 min = pos;
	const float2 max = pos + size;
	const bool outside =
		min.x >= containerMax.x || min.y >= containerMax.y ||
		max.x < containerMin.x || max.y < containerMin.y;

	if ( !outside )
	{
		// pos
		const float2 v0 = { pos.x, pos.y }; // top-left
		const float2 v1 = { pos.x, pos.y + size.y }; // bottom-left
		const float2 v2 = { pos.x + size.x, pos.y + size.y }; // bottom-right
		const float2 v3 = { pos.x + size.x, pos.y }; // top-right

		// uv
		const float2 uvTL = { uv.x, uv.y };
		const float2 uvBL = { uv.x, uv.y + uvSize.y };
		const float2 uvBR = { uv.x + uvSize.x , uv.y + uvSize.y };
		const float2 uvTR = { uv.x + uvSize.x , uv.y };

		// color
		const rgba colorTop  = Rgba(fcolorTop);
		const rgba colorBot  = Rgba(fcolorBot);

		UI_AddTriangle(ui,
				UIVertex{ v0, uvTL, colorTop },
				UIVertex{ v1, uvBL, colorBot },
				UIVertex{ v2, uvBR, colorBot });
		UI_AddTriangle(ui,
				UIVertex{ v0, uvTL, colorTop },
				UIVertex{ v2, uvBR, colorBot },
				UIVertex{ v3, uvTR, colorTop });
	}
}

void UI_AddQuad(UI &ui, float2 pos, float2 size, float2 uv, float2 uvSize, float4 fcolor)
{
	UI_AddQuad(ui, pos, size, uv, uvSize, fcolor, fcolor);
}

void UI_AddRectangle(UI &ui, float2 pos, float2 size)
{
	const float2 uvSize = {0, 0};
	const float4 color = UI_GetColor(ui);
	UI_AddQuad(ui, pos, size, ui.whitePixelUv, uvSize, color);
}

void UI_AddRectangle(UI &ui, float2 pos, float2 size, float4 colorTop, float4 colorBot)
{
	const float2 uvSize = {0, 0};
	UI_AddQuad(ui, pos, size, ui.whitePixelUv, uvSize, colorTop, colorBot);
}

void UI_AddCircle(UI &ui, float2 pos, float radius)
{
	const u32 numTriangles = 8;
	const float2 center = pos + float2{radius, radius};
	for (u32 i = 0; i < numTriangles; ++i)
	{
		const float angle0 = TwoPi * (float)i / numTriangles;
		const float angle1 = TwoPi * (float)(i + 1) / numTriangles;
		const float2 v0 = center + radius * float2{Sin(angle0),Cos(angle0)};
		const float2 v1 = center + radius * float2{Sin(angle1),Cos(angle1)};
		UI_AddTriangle(ui, center, v0, v1);
	}
}

void UI_AddBorder(UI &ui, float2 pos, float2 size, float borderSize)
{
	const float2 uvSize = {0, 0};
	const float4 color = UI_GetColor(ui);
	const float2 topPos = pos;
	const float2 leftPos = pos + float2{0.0, 1.0f};
	const float2 rightPos = pos + float2{size.x-1, 1.0f};
	const float2 bottomPos = pos + float2{0.0,size.y-1};
	const float2 hSize = float2{size.x, 1.0f};
	const float2 vSize = float2{1.0, size.y - 2.0f};
	UI_AddQuad(ui, topPos, hSize, ui.whitePixelUv, uvSize, color);
	UI_AddQuad(ui, leftPos, vSize, ui.whitePixelUv, uvSize, color);
	UI_AddQuad(ui, rightPos, vSize, ui.whitePixelUv, uvSize, color);
	UI_AddQuad(ui, bottomPos, hSize, ui.whitePixelUv, uvSize, color);
}

float UI_TextHeight(const UI &ui)
{
	const float textHeight = ui.fontAscent - ui.fontDescent;
	return textHeight;
}

// Shared height for one-line controls (buttons, checkboxes, text/number boxes...)
// so that widgets placed side by side in a horizontal layout line up.
float UI_ControlHeight(const UI &ui)
{
	const float controlHeight = UI_TextHeight(ui) + 2.0f * UI_GetPadding(ui).y;
	return controlHeight;
}

float UI_TextWidth(const UI &ui, const char *text, u32 maxChars = U32_MAX)
{
	float textWidth = 0.0f;

	const char *ptr = text;
	while ( *ptr && maxChars-- > 0 )
	{
		const char c = *ptr;
		const stbtt_packedchar &pc = ui.charData[c];
		textWidth += pc.xadvance;
		ptr++;
	}

	return (f32)(u32)textWidth;
}

float2 UI_AdjustTextVertically(const UI &ui, float2 pos, float height)
{
	const float textHeight = UI_TextHeight(ui);
	const float ypos = Round(pos.y + height / 2.0 - textHeight / 2.0);
	const float2 res = { pos.x, ypos };
	return res;
}

float2 UI_AdjustTextHorizontally(const UI &ui, float2 pos, float width, const char *text)
{
	const float textWidth = UI_TextWidth(ui, text);
	const float xpos = Round(pos.x + width / 2.0 - textWidth / 2.0);
	const float2 res = { xpos, pos.y };
	return res;
}

float2 UI_TextSize(const UI &ui, const char *text)
{
	const float textWidth = UI_TextWidth(ui, text);
	const float textHeight = UI_TextHeight(ui);
	const float2 textSize = { textWidth, textHeight };
	return textSize;
}

static bool UI_IsRectInRect(float2 charPos, float2 charSize, float2 containerPos, float2 containerSize)
{
	const bool outside = charPos.x + charSize.x < containerPos.x || charPos.y + charSize.y < containerPos.y ||
		charPos.x >= containerPos.x + containerSize.x ||
		charSize.y >= containerPos.y + containerSize.y;
	return !outside;
}

void UI_AdjustCharRect(float2 &charPos, float2 &charSize, float2 &charUvPos, float2 &charUvSize, const float2 containerPos, const float2 containerSize)
{
	if ( charSize.x <= 0.0f || charSize.y <= 0.0f )
	{
		charSize = float2{0.0f, 0.0f};
		charUvSize = float2{0.0f, 0.0f};
		return;
	}

	const float2 clippedMin = Max( charPos, containerPos );
	const float2 clippedMax = Min( charPos + charSize, containerPos + containerSize );

	if ( clippedMax.x <= clippedMin.x || clippedMax.y <= clippedMin.y )
	{
		charSize = float2{0.0f, 0.0f};
		charUvSize = float2{0.0f, 0.0f};
		return;
	}

	// Texels per pixel, to translate the clipping made in space coords into uv coords
	const float2 uvPerPixel = charUvSize / charSize;

	charUvPos = charUvPos + ( clippedMin - charPos ) * uvPerPixel;
	charUvSize = ( clippedMax - clippedMin ) * uvPerPixel;
	charPos = clippedMin;
	charSize = clippedMax - clippedMin;
}

void UI_AddText(UI &ui, float2 pos, const char *text)
{
	const float cursory = Round( pos.y + ui.fontAscent );
	float cursorx = pos.x;

	const char *ptr = text;
	while ( *ptr )
	{
		const char c = *ptr;
		const stbtt_packedchar &pc = ui.charData[c];

		const float charWidth = pc.x1 - pc.x0;
		const float charHeight = pc.y1 - pc.y0;

		float2 charPos = {cursorx + pc.xoff, cursory + pc.yoff};
		float2 charSize = {charWidth, charHeight};
		float2 charUv = {pc.x0/ui.fontAtlasSize.x, pc.y0/ui.fontAtlasSize.y};
		float2 charUvSize = {charWidth/ui.fontAtlasSize.x, charHeight/ui.fontAtlasSize.y};

		float2 containerPos = {0, 0};
		float2 containerSize = Float2(ui.viewportSize);
		if (ui.widgetStackSize > 0) {
			const UIWidget &widget = ui.widgetStack[ui.widgetStackSize-1];
			containerPos = widget.pos;
			containerSize = widget.size;
		}

		UI_AdjustCharRect(charPos, charSize, charUv, charUvSize, containerPos, containerSize);

		if ( charSize.x > 0.0f && charSize.y > 0.0f )
		{
			UI_AddQuad(ui, charPos + float2{1, 1}, charSize, charUv, charUvSize, UiColorBlack);

			const UIElementColor &textColor = UI_GetElemColor(ui, UIElementText);
			UI_AddQuad(ui, charPos, charSize, charUv, charUvSize, textColor.base);
		}

		cursorx = Round( cursorx + pc.xadvance );
		ptr++;
	}
}

UIWindow &UI_FindWindow(UI &ui, UIID windowId)
{
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		if ( window.id == windowId )
		{
			return window;
		}
	}

	ASSERT(0 && "Could not find window.");
	static UIWindow window = {};
	return window;
}

UIWindow &UI_FindOrCreateWindow(UI &ui, UIID windowId, const char *caption)
{
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		if ( window.id == windowId )
		{
			UI_ResetWindowDefaults(ui);
			ASSERT(window.caption != 0);
			return window;
		}
	}

	ASSERT(ui.windowCount < ARRAY_COUNT(ui.windows));
	const u32 windowIndex = ui.windowCount++;
	UIWindow &window = ui.windows[windowIndex];

	window.id = windowId;
	window.index = windowIndex;
	StrCopy(window.caption, caption);
	window.layer = windowIndex;

	window.displacement = ui.defaultWindowDisplacement;
	window.size = ui.defaultWindowSize;
	UI_ResetWindowDefaults(ui);

	UI_PositionWindow(window, ui.viewportSize, window.size, window.anchor, window.displacement);
	UI_RaiseWindow(ui, window);

	return window;
}

UIInfo &UI_FindOrCreateInfo(UI &ui, UIID infoId, bool *wasCreated)
{
	if (wasCreated) {
		*wasCreated = false;
	}

	for (u32 i = 0; i < ui.infoCount; ++i)
	{
		UIInfo &info = ui.info[i];
		if ( info.id == infoId )
		{
			if (wasCreated) {
			}
			return info;
		}
	}

	if (wasCreated) {
		*wasCreated = true;
	}

	ASSERT( ui.infoCount < ARRAY_COUNT(ui.info) );
	UIInfo &info = ui.info[ui.infoCount++];
	info.id = infoId;
	return info;
}

void UI_SetNextWindowModal(UI &ui, UIModalFlags flags = UIModalFlag_Default)
{
	ASSERT(ui.modalWindowStackSize < ARRAY_COUNT(ui.modalWindowStack));
	ui.nextWindow.modalFlags = flags;
	ui.nextWindow.setMask |= UINextWindow_Modal;
}

void UI_BeginWindow(UI &ui, UIID windowId, u32 flags, bool *isOpen = nullptr)
{
	ASSERT(ui.windowStackSize < ARRAY_COUNT(ui.windowStack));

	UIWindow &window = UI_FindWindow(ui, windowId);
	ui.windowStack[ui.windowStackSize++] = window.index;
	window.visible = true;
	window.flags = flags;

	const UINextWindow next = ui.nextWindow;
	ui.nextWindow = {};

	if (next.setMask & UINextWindow_Size)
	{
		window.size = next.size;
	}

	if (next.setMask & UINextWindow_AnchorAndPivot)
	{
		window.anchor = next.anchor;
		window.pivot = next.pivot;
	}

	if (next.setMask & UINextWindow_Displacement)
	{
		window.displacement = next.displacement;
	}

	constexpr u32 repositioningBits = UINextWindow_Size | UINextWindow_AnchorAndPivot | UINextWindow_Displacement;
	if (next.setMask & repositioningBits)
	{
		UI_PositionWindow(window, ui.viewportSize, window.size, window.anchor, window.displacement);
	}

	if (next.setMask & UINextWindow_Modal)
	{
		window.modalFlags = next.modalFlags;
		ASSERT(ui.modalWindowStackSize < ARRAY_COUNT(ui.modalWindowStack));
		ui.modalWindowStack[ui.modalWindowStackSize++] = &window;
	}

	UI_PushCursorPos(ui, window.pos);

	UI_PushDrawList(ui, rect{0, 0, ui.viewportSize.x, ui.viewportSize.y}, ui.fontAtlasH, UIDrawListFlag_None);

	UIDrawList &drawList = UI_GetDrawList(ui);
	drawList.sortKey.layer = window.layer;
	drawList.sortKey.order = 0;

	UI_BeginLayout(ui, UILayout_Vertical);
	float2 panelPos = window.pos;
	float2 panelSize = window.size;

	if ( flags & UIWindowFlag_Border )
	{
		UI_PushColor(ui, ui.style.borderColor);
		UI_AddBorder(ui, window.pos, window.size, ui.style.borderSize.x);
		UI_PopColor(ui);

		panelPos = panelPos + ui.style.borderSize;
		panelSize = panelSize - 2.0f * ui.style.borderSize;
	}

	if (flags & UIWindowFlag_Titlebar)
	{
		const bool activeWindow = ui.activeWindow == &window;
		const float2 titlebarPos = panelPos;
		const float2 titlebarSize = float2{panelSize.x, ui.style.titlebarHeight};

		const UIElementColor &captionColor = UI_GetElemColor(ui, UIElementCaption);
		UI_PushColor(ui, activeWindow ? captionColor.base : captionColor.inactive);
		UI_AddRectangle(ui, titlebarPos, titlebarSize);
		UI_PopColor(ui);

		const float2 captionPos = UI_AdjustTextVertically(ui, titlebarPos + float2{ui.style.windowPadding.x, 0.0}, titlebarSize.y);
		UI_AddText(ui, captionPos, window.caption);

		panelPos.y += titlebarSize.y;
		panelSize.y -= titlebarSize.y;

		if (flags & UIWindowFlag_CloseButton && isOpen != nullptr)
		{
			const float2 closePos =  titlebarPos + float2{titlebarSize.x - titlebarSize.y, 0.0};
			const float2 closeSize = float2{titlebarSize.y, titlebarSize.y};
			UI_BeginWidget(ui, closePos, closeSize, false);

			// Surface
			UI_PushColor(ui, UIElementButton);
			UI_AddRectangle(ui, closePos, closeSize);
			UI_PopColor(ui);

			// Cross
			UI_PushColor(ui, UiColorWhite);
			const float2 cornerTL = closePos;
			const float2 cornerTR = closePos + dX(closeSize);
			const float2 cornerBL = closePos + dY(closeSize);
			const float2 cornerBR = closePos + closeSize;
			const float d = 5;
			const float m = 3;
			const float2 cornerTL1 = {cornerTL.x + d, cornerTL.y + m};
			const float2 cornerTL2 = {cornerTL.x + m, cornerTL.y + d };
			const float2 cornerBL1 = {cornerBL.x + m, cornerBL.y - d};
			const float2 cornerBL2 = {cornerBL.x + d, cornerBL.y - m};
			const float2 cornerTR1 = {cornerTR.x - d, cornerTR.y + m};
			const float2 cornerTR2 = {cornerTR.x - m, cornerTR.y + d};
			const float2 cornerBR1 = {cornerBR.x - m, cornerBR.y - d};
			const float2 cornerBR2 = {cornerBR.x - d, cornerBR.y - m};
			UI_AddTriangle(ui, cornerBL2, cornerTR2, cornerTR1);
			UI_AddTriangle(ui, cornerBL2, cornerTR1, cornerBL1);
			UI_AddTriangle(ui, cornerTL2, cornerBR2, cornerBR1);
			UI_AddTriangle(ui, cornerTL2, cornerBR1, cornerTL1);
			UI_PopColor(ui);

			*isOpen = !UI_WidgetClicked(ui);
			UI_EndWidget(ui);
		}
	}

	if ( flags & UIWindowFlag_Background )
	{
		UI_PushColor(ui, UIElementBackground);
		UI_AddRectangle(ui, panelPos, panelSize);
		UI_PopColor(ui);
	}

	const float cornerSize = ui.style.resizeCornerSize;

	if ( flags & UIWindowFlag_Resizable )
	{
		const float2 cornerBR = window.pos + window.size - (flags & UIWindowFlag_Border ? ui.style.borderSize : float2{0, 0});
		const float2 cornerTR = cornerBR + float2{0.0, -cornerSize};
		const float2 cornerBL = cornerBR + float2{-cornerSize, 0.0};
		const float2 cornerTL = cornerBR + float2{-cornerSize, -cornerSize};
		UI_BeginWidget(ui, cornerTL, float2{cornerSize, cornerSize}, false);
		UI_PushColor(ui, UIElementButton);
		UI_AddTriangle(ui, cornerBL, cornerBR, cornerTR);
		UI_PopColor(ui);
		if (UI_WidgetPressed(ui))
		{
			window.sizeBeforeResize = {(i32)window.size.x, (i32)window.size.y};
			window.resizing = true;
			// Disable other widgets, resize widget has prevalence
			window.disableWidgets = true;
		}
		UI_EndWidget(ui);
	}

	float2 cursorPos = panelPos;

	if ( flags & UIWindowFlag_ClipContents )
	{
		window.clippedContents = true;

		const u32 containerHeight = (u32)(panelSize.y - 2.0f * ui.style.windowPadding.y);

		const bool renderScrollbar = containerHeight < window.contentSize.y;
		const i32 scrollbarWidth = renderScrollbar ? ui.style.scrollbarWidth : 0;
		const i32 scrollbarOuterWidth = renderScrollbar ? 3.0f * ui.style.scrollbarWidth : 0;

		const u32 containerWidth = renderScrollbar ?
			(u32)(panelSize.x - ui.style.windowPadding.x - scrollbarOuterWidth) :
			(u32)(panelSize.x - 2.0f * ui.style.windowPadding.x - scrollbarWidth);

		const int2 containerPos = {
			.x = (i32)(panelPos.x + ui.style.windowPadding.x),
			.y = (i32)(panelPos.y + ui.style.windowPadding.y),
		};
		const uint2 containerSize = {
			.x = containerWidth,
			.y = containerHeight,
		};

		if (renderScrollbar)
		{
			if (UI_IsMouseIdle(ui)) {
				window.scrolling = false;
			}

			if (window.scrolling) { // Scroll by widget
				const f32 mouseDelta = UI_LastMouseClickPos(ui).y - UI_MousePos(ui).y;
				const f32 scrollDelta = window.contentSize.y * mouseDelta / containerHeight;
				window.contentOffset = window.contentOffsetBeforeScrolling + scrollDelta;
			} else if (UI_WindowHovered(ui)) {
				const f32 scrollDelta = UI_MouseScroll(ui).y * ui.style.scrollSpeed; // Scroll by mouse
				window.contentOffset -= scrollDelta;
			}

			const f32 minTopPosition = Min(window.containerSize.y - window.contentSize.y, 0.0f);
			const f32 maxTopPosition = 0.0f;
			window.contentOffset = Min(window.contentOffset, maxTopPosition);
			window.contentOffset = Max(window.contentOffset, minTopPosition);

			const f32 scrollbarPortion = Min(containerHeight / window.contentSize.y , 1.0f);
			const f32 scrollbarHeight = containerHeight * scrollbarPortion;
			const float2 scrollbarSize = { scrollbarWidth * 2.0f, scrollbarHeight };
			const f32 scrollbarMinY = containerPos.y;
			const f32 scrollbarMaxY = scrollbarMinY + containerHeight - scrollbarHeight;
			const f32 scrollbarPosYNorm = 1.0f - (window.contentOffset - minTopPosition)/(maxTopPosition - minTopPosition);
			const f32 scrollbarPosY = scrollbarMinY + (scrollbarMaxY - scrollbarMinY) * scrollbarPosYNorm;
			const float2 scrollbarPos = { (f32)containerPos.x + (f32)containerSize.x + ui.style.windowPadding.x * 0.5f, scrollbarPosY };
			UI_BeginWidget(ui, scrollbarPos, scrollbarSize);
			UI_PushColor(ui, UIElementScrollbar);
			UI_AddRectangle(ui, scrollbarPos, scrollbarSize);
			UI_PopColor(ui);
			if (UI_WidgetPressed(ui)) {
				window.scrolling = true;
				window.contentOffsetBeforeScrolling = window.contentOffset;
			}
			UI_EndWidget(ui);
		}
		else
		{
			window.contentOffset = 0.0f;
		}

		const rect containerRect = { .pos = containerPos, .size = containerSize };
		UI_PushDrawList(ui, containerRect, ui.fontAtlasH);

		panelSize = {(f32)containerSize.x, (f32)containerSize.y};

		cursorPos = panelPos + ui.style.windowPadding;
		cursorPos.y += window.contentOffset;

		// Adjusted so containerPos includes the padding (I think)
		panelPos = {(f32)containerPos.x, (f32)containerPos.y};
	}

	window.containerPos = panelPos;
	window.containerSize = panelSize;

	UI_SetCursorPos(ui, cursorPos);
	window.contentLayoutGroup = ui.layoutGroupCount;
	UI_BeginLayout(ui, UILayout_Vertical);
}

void UI_PushID(UI &ui, UIID id)
{
	ASSERT(ui.idStackSize < ARRAY_COUNT(ui.idStack));
	const UIID parentId = ui.idStackSize > 0 ? ui.idStack[ui.idStackSize-1] : 1;
	ui.idStack[ui.idStackSize++] = parentId * id;
}

void UI_PopID(UI &ui)
{
	ASSERT(ui.idStackSize > 0);
	ui.idStackSize--;
}

UIID UI_MakeID(const UI &ui, const char *text, UIID parentId = 1)
{
	if ( ui.windowStackSize > 0 )
	{
		const UIWindow &window = UI_GetCurrentWindow(ui);
		parentId *= window.id;
	}

	if (ui.idStackSize > 0)
	{
		parentId *= ui.idStack[ui.idStackSize-1];
	}

	const UIID id = HashStringFNV(text, parentId);
	ASSERT(id != 0);
	return id;
}

UIID UI_MakeID(const UI &ui, const char *text, const void *parentPtr)
{
	const UIID res = UI_MakeID(ui, text, (UIID)(uintptr_t)(parentPtr));
	return res;
}

constexpr u32 UIWindowFlag_Default = UIWindowFlag_Draggable | UIWindowFlag_Resizable | UIWindowFlag_Titlebar | UIWindowFlag_CloseButton | UIWindowFlag_Border | UIWindowFlag_Background | UIWindowFlag_ClipContents;

void UI_BeginWindow(UI &ui, const char *caption, bool *isOpen, u32 flags = UIWindowFlag_Default)
{
	const UIID windowId = UI_MakeID(ui, caption);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, caption);

	UI_BeginWindow(ui, windowId, flags, isOpen);
}

void UI_EndWindow(UI &ui)
{
	UIWindow &window = UI_GetCurrentWindow(ui);

	window.contentSize = UI_GetLayoutGroup(ui).size;
	UI_EndLayout(ui); // Contents layout

	if (window.clippedContents)
	{
		UI_PopDrawList(ui);
	}
	UI_EndLayout(ui, false); // Panel layout
	UI_PopDrawList(ui);
	UI_PopCursorPos(ui);

	ASSERT(ui.windowStackSize > 0);
	ui.windowStackSize--;

}

UISection &UI_GetSection(UIWindow &window, const char *caption)
{
	const u32 sectionHash = HashStringFNV(caption);
	for (u32 i = 0; i < window.sectionCount; ++i)
	{
		if ( window.sections[i].hash == sectionHash )
		{
			return window.sections[i];
		}
	}

	ASSERT(window.sectionCount < ARRAY_COUNT(window.sections));
	UISection &section = window.sections[window.sectionCount++];
	section.hash = sectionHash;
	section.open = false;
	return section;
}

float2 UI_GetContainerPos(const UIWindow &window)
{
	return window.containerPos;
}

float2 UI_GetContainerSize(const UIWindow &window)
{
	return window.containerSize;
}

// Width of the left-hand control box of a labelled widget. The label is drawn to the
// right of this box, so we subtract however far the cursor already sits from the window
// content origin (indentation, plus whatever a horizontal layout placed before us) to
// keep every label aligned to the same column.
f32 UI_GetAlignedWidgetWidth(UI &ui)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 containerWidth = UI_GetContainerSize(window).x;
	const f32 contentOriginX = ui.layoutGroups[window.contentLayoutGroup].pos.x;
	const f32 offset = UI_GetCursorPos(ui).x - contentOriginX;
	return Round(containerWidth * ui.style.labelRatio - offset);
}

bool UI_Section(UI &ui, const char *caption)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	UISection &section = UI_GetSection(window, caption);

	const float containerWidth = UI_GetContainerSize(window).x;
	const float textHeight = UI_TextHeight(ui);
	constexpr f32 vpadding = 3.0f;
	const float2 pos = UI_GetCursorPos(ui);
	const float2 size = { containerWidth, textHeight + 2.0f * vpadding};

	UI_BeginWidget(ui, pos, size);

	UI_PushColor(ui, UIElementSection);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	constexpr f32 triangleHeight = 10.0f;
	constexpr float2 triangleOffset = {4.0f, 4.0f};
	const float2 p0 = pos + triangleOffset;
	const float2 p1 = p0 + ( section.open ?
		float2{triangleHeight*0.5f, triangleHeight} :
		float2{0.0f, triangleHeight} );
	const float2 p2 = p0 + ( section.open ?
		float2{triangleHeight, 0.0f} :
		float2{triangleHeight, triangleHeight * 0.5f} );
	UI_AddTriangle(ui, p0, p1, p2, UiColorWhite);

	constexpr float2 textOffset = {20.0f, 3.0f};
	const float2 textPos = pos + textOffset;
	UI_AddText(ui, textPos, caption);

	const bool clicked = UI_WidgetClicked(ui);
	if (clicked) {
		section.open = !section.open;
	}
	UI_EndWidget(ui);
	UI_CursorAdvance(ui, size);

	return section.open;
}

void UI_Label(UI &ui, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	const float2 pos = UI_AdjustTextVertically(ui, UI_GetCursorPos(ui), UI_ControlHeight(ui));
	UI_AddText(ui, pos, text);

	const f32 textWidth = UI_TextWidth(ui, text);
	const f32 textHeight = UI_TextHeight(ui);
	const float2 size = { textWidth + ui.style.itemSpacing, textHeight };
	UI_CursorAdvance(ui, size);
}

bool UI_Button(UI &ui, const char *text)
{
	const float2 padding = UI_GetPadding(ui);
	const float2 textSize = UI_TextSize(ui, text);
	const float2 size = { textSize.x + 2.0f * padding.x, UI_ControlHeight(ui) };

	const float2 pos = UI_GetCursorPos(ui);

	UI_BeginWidget(ui, pos, size);

	UI_PushColor(ui, UIElementButton);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	const float2 textPos = pos + padding;
	UI_AddText(ui, textPos, text);

	const bool clicked = UI_WidgetClicked(ui);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, size);

	return clicked;
}

void UI_BeginCanvas(UI &ui)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float2 pos = UI_GetCursorPos(ui);
	const float2 size = { UI_GetContainerSize(window).x, UI_ControlHeight(ui) };
	UI_BeginWidget(ui, pos, size, false);

	UI_PushColor(ui, UIElementBox);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);
}

void UI_EndCanvas(UI &ui)
{
	UI_EndWidget(ui);
	UI_CursorAdvance(ui, ui.lastWidgetSize);
}

// a and b are normalized coords within the Canvas widget
void UI_DrawBox(UI &ui, float2 a, float2 b)
{
	const UIWidget &canvas = UI_GetCurrentWidget(ui);
	const float2 pos = canvas.pos + a * canvas.size;
	const float2 size = (b - a) * canvas.size;

	UI_PushColor(ui, ui.style.accentColor);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);
}

static bool UI_IconWidget(UI &ui, u32 iconIndex, const bool *checked)
{
	ASSERT(iconIndex < ui.iconCount);
	const UIIcon &icon = ui.icons[iconIndex];

	constexpr float2 padding = {2.0f, 2.0f};
	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 iconPos = widgetPos + padding;
	const float2 iconSize = { (f32)icon.image.width, (f32)icon.image.height };
	const float2 widgetSize = iconSize + 2.0f * padding;

	UI_BeginWidget(ui, widgetPos, widgetSize);

	//UI_PushColor(ui, UIElementButton);
	//UI_AddRectangle(ui, widgetPos, widgetSize);
	//UI_PopColor(ui);

	const bool lit = ( checked == nullptr || *checked || UI_WidgetHovered(ui) );
	UI_AddQuad(ui, iconPos, iconSize, icon.uv, icon.uvSize, lit ? UiColorWhite : UiColorGray);

	const bool clicked = UI_WidgetClicked(ui);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

bool UI_ButtonIcon(UI &ui, u32 iconIndex)
{
	return UI_IconWidget(ui, iconIndex, nullptr);
}

bool UI_ToggleIcon(UI &ui, u32 iconIndex, bool *checked)
{
	ASSERT(checked != nullptr);

	const bool clicked = UI_IconWidget(ui, iconIndex, checked);
	if (clicked)
	{
		*checked = !*checked;
	}

	return clicked;
}

bool UI_Radio(UI &ui, const char *text, bool active)
{
	const float2 widgetPos = UI_GetCursorPos(ui);
	const float controlHeight = UI_ControlHeight(ui);
	const float2 textPos = widgetPos + float2{controlHeight + ui.style.itemSpacing * 0.5f, 0.0};
	const float2 adjustedPos = UI_AdjustTextVertically(ui, textPos, controlHeight);
	const float  textWidth = UI_TextWidth(ui, text);

	const float2 widgetSize = float2{controlHeight, controlHeight} + float2{ui.style.itemSpacing, 0.0f} + float2{textWidth, 0.0f};
	UI_BeginWidget(ui, widgetPos, widgetSize);

	UI_PushColor(ui, UIElementToggle);
	const float2 ballPos = widgetPos + float2{3, 3};
	const float2 ballSize = {controlHeight - 6, controlHeight - 6};
	UI_AddCircle(ui, ballPos, ballSize.y/2.0);
	UI_PopColor(ui);
	if (active)
	{
		const float2 margin = {3.0, 3.0};
		const float2 innerPos = ballPos + margin;
		const float2 innerSize = ballSize - 2.0 * margin;
		UI_PushColor(ui, UiColorWhite);
		UI_AddCircle(ui, innerPos, innerSize.y/2.0);
		UI_PopColor(ui);
	}
	const bool clicked = UI_WidgetClicked(ui);

	UI_AddText(ui, adjustedPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

bool UI_Checkbox(UI &ui, const char *text, bool *checked)
{
	ASSERT(checked != nullptr);

	const float2 boxPos = UI_GetCursorPos(ui);
	const float controlHeight = UI_ControlHeight(ui);
	const float2 boxSize = {controlHeight, controlHeight};
	const float2 textPos = boxPos + float2{boxSize.x + ui.style.itemSpacing * 0.5f, 0.0};
	const float2 adjustedPos = UI_AdjustTextVertically(ui, textPos, boxSize.y);
	const float  textWidth = UI_TextWidth(ui, text);

	const float2 widgetPos = boxPos;
	const float2 widgetSize = boxSize + float2{ui.style.itemSpacing, 0.0f} + float2{textWidth, 0.0f};
	UI_BeginWidget(ui, widgetPos, widgetSize);

	UI_PushColor(ui, UIElementToggle);
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	if (*checked)
	{
		const float2 margin = {3.0, 3.0};
		const float2 innerPos = boxPos + margin;
		const float2 innerSize = boxSize - 2.0 * margin;
		UI_PushColor(ui, UiColorWhite);
		UI_AddRectangle(ui, innerPos, innerSize);
		UI_PopColor(ui);
	}
	const bool clicked = UI_WidgetClicked(ui);
	if (clicked)
	{
		*checked = !*checked;
	}

	UI_AddText(ui, adjustedPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

void UI_Combo(UI &ui, const char *text, const char **items, u32 itemCount, u32 *selectedIndex)
{
	ASSERT(selectedIndex != nullptr);
	const u32 index = *selectedIndex;

	const float2 padding = UI_GetPadding(ui);
	const f32 side = UI_ControlHeight(ui);

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{UI_GetAlignedWidgetWidth(ui), side};

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const float2 boxPos = widgetPos;
	const float2 boxSize = float2{widgetSize.x - side, side};

	UI_PushColor(ui, UIElementInput);
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	//UI_PushColor(ui, ui.style.borderColor);
	//UI_AddBorder(ui, boxPos, boxSize, 1);
	//UI_PopColor(ui);

	const float2 textPos = widgetPos + padding;
	UI_AddText(ui, textPos, items[index]);

	const float2 butPos = widgetPos + float2{boxSize.x, 0.0f};
	const float2 butSize = {side, side};

	UI_PushColor(ui, UIElementButton);
	UI_AddRectangle(ui, butPos, butSize);
	UI_PopColor(ui);

	const f32 triangleSide = side * 0.6f;
	const float2 triangleOffset = 0.2f * float2{side, side};
	const float2 p0 = butPos + triangleOffset;
	const float2 p1 = p0 + float2{triangleSide*0.5f, triangleSide};
	const float2 p2 = p0 + float2{triangleSide, 0.0f};
	UI_AddTriangle(ui, p0, p1, p2, UiColorWhite);

	const bool mouseClick = UI_IsMousePress(ui);
	const bool clickedInside = UI_WidgetClicked(ui);

	UI_EndWidget(ui);
	UI_CursorAdvance(ui, widgetSize);

	const float2 text2Pos = butPos + float2{butSize.x + ui.style.itemSpacing, padding.y};
	UI_AddText(ui, text2Pos, text);

	const UIID comboId = UI_MakeID(ui, text);
	if ( clickedInside )
	{
		ui.comboBox.id = ui.comboBox.id == comboId ? 0 : comboId;
	}

	if (ui.comboBox.id == comboId)
	{
		const f32 itemHeight = UI_ControlHeight(ui);
		const float2 panelPos = boxPos + float2{0.0f, boxSize.y};
		float2 panelSize = 2.0f * ui.style.borderSize;
		for (u32 i = 0; i < itemCount; ++i)
		{
			const f32 itemWidth = Max( widgetSize.x, UI_TextWidth(ui, items[i]) + 2.0*padding.x );
			panelSize.x = Max(panelSize.x, itemWidth);
			panelSize.y += itemHeight;
		}

		UIWindow &comboPanel = UI_FindOrCreateWindow(ui, comboId, "$combo");
		UI_RaiseWindow(ui, comboPanel);
		comboPanel.size = panelSize;
		UI_PositionWindow(comboPanel, panelPos);

		UI_BeginWindow(ui, comboId, UIWindowFlag_Border);

		float2 itemPos = panelPos + ui.style.borderSize;
		const f32 itemWidth = panelSize.x - 2.0f*ui.style.borderSize.x;
		const float2 itemSize = float2{itemWidth, itemHeight};
		for (u32 i = 0; i < itemCount; ++i)
		{
			const f32 textWidth = Max( widgetSize.x, UI_TextWidth(ui, items[i]) );
			UI_BeginWidget(ui, itemPos, itemSize);
			const bool itemHovered = UI_WidgetHovered(ui);
			UI_PushColor(ui, itemHovered ?
				UI_GetElemColor(ui, UIElementInput).hovered :
				UI_GetElemColor(ui, UIElementBackground).base);
			UI_AddRectangle(ui, itemPos, itemSize);
			UI_PopColor(ui);
			const float2 textPos = itemPos + padding;
			UI_AddText(ui, textPos, items[i]);
			if (itemHovered && mouseClick)
			{
				*selectedIndex = i;
			}
			UI_EndWidget(ui);
			itemPos.y += itemHeight;
		}

		UI_EndWindow(ui);

		if ( mouseClick && !clickedInside ) {
			ui.comboBox.id = 0;
		}
	}

	UI_SetCursorPos(ui, widgetPos);
	UI_CursorAdvance(ui, widgetSize);
}

static float4 RGB0(const float4 rgba)
{
	const float4 res = {rgba.r, rgba.g, rgba.b, 0.0f};
	return res;
}

bool UI_TreeNode(UI &ui, const char *text, const void *ptr, bool *isOpen, u32 flags = UITreeNodeFlag_None)
{
	bool wasCreated;
	const UIID id = UI_MakeID(ui, text, ptr);
	UIInfo &info = UI_FindOrCreateInfo(ui, id, &wasCreated);

	if (wasCreated && isOpen) {
		info.isOpen = *isOpen;
	}

	const float2 boxPos = UI_GetCursorPos(ui);
	const float controlHeight = UI_ControlHeight(ui);
	const float2 boxSize = {controlHeight, controlHeight};

	UI_BeginWidget(ui, boxPos, boxSize);

	const bool drawOpenControl = !( flags & UITreeNodeFlag_Leaf );

	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float containerWidth = UI_GetContainerSize(window).x;
	const float2 rowPos = { window.containerPos.x, boxPos.y };
	const float2 rowSize = { containerWidth, controlHeight };
	UI_AddRectangle(ui, rowPos, rowSize, UiColorDarkGray, RGB0(UiColorDarkGray));

	if ( drawOpenControl )
	{
		const float4 arrowColor = ( info.isOpen || UI_WidgetHovered(ui) ) ? UiColorWhite : UiColorGray;

		const float2 margin = {6.0, 6.0};
		const float2 innerSize = boxSize - 2.0 * margin;

		if (info.isOpen)
		{
			const float2 p0 = boxPos + margin;
			const float2 p1 = p0 + dY(innerSize);
			const float2 p2 = p0 + innerSize;
			const float2 p3 = p0 + dX(innerSize);
			UI_AddTriangle(ui, p1, p2, p3, arrowColor);
		}
		else
		{
			const float2 p1 = boxPos + margin;
			const float2 p2 = p1 + dY(innerSize);
			const float2 p3 = p1 + float2{innerSize.x, 0.5f*innerSize.y};

			UI_AddTriangle(ui, p1, p2, p3, arrowColor);
		}
	}

	const bool nodeClicked = UI_WidgetClicked(ui);
	if (nodeClicked)
	{
		info.isOpen = !info.isOpen;
	}

	UI_EndWidget(ui);

	const float2 widgetPos = boxPos;

	const float2 textPos = boxPos + float2{boxSize.x + ui.style.itemSpacing * 0.5f, 0.0};
	const float2 adjustedPos = UI_AdjustTextVertically(ui, textPos, boxSize.y);
	const float  textWidth = UI_TextWidth(ui, text);

	UI_BeginWidget(ui, textPos, float2{textWidth, controlHeight});
	const float4 textColor = ( info.isOpen || UI_WidgetHovered(ui) ) ? UiColorWhite : UiColorGray;
	UI_PushElemColor(ui, UIElementText, {textColor, textColor, textColor, textColor} );
	UI_AddText(ui, adjustedPos, text);
	UI_PopElemColor(ui, UIElementText);
	const bool textClicked = UI_WidgetClicked(ui);
	UI_EndWidget(ui);

	const float2 widgetSize = boxSize + float2{ui.style.itemSpacing, 0.0f} + float2{textWidth, 0.0f};
	UI_CursorAdvance(ui, widgetSize);

	if (isOpen) {
		*isOpen = info.isOpen;
	}

	return textClicked;
}

void UI_Separator(UI &ui)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float containerWidth = UI_GetContainerSize(window).x;
	const float containerHeight = UI_GetContainerSize(window).y;

	const UILayout layout = UI_GetLayout(ui);

	const f32 spacing = (f32)Floor(0.3f * UI_ControlHeight(ui));
	const f32 hspacing = (f32)Floor(0.5f * spacing);

	const float2 pos = UI_GetCursorPos(ui) + float2{0.0f, hspacing};
	const float2 size = layout == UILayout_Vertical ?
		float2{ containerWidth, 1.0 } : float2{ 1.0, containerHeight };

	UI_PushColor(ui, ui.style.borderColor);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	UI_CursorAdvance(ui, size + float2{0.0f, spacing}, 0.0f);
}

void UI_SeparatorLabel(UI &ui, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float containerWidth = UI_GetContainerSize(window).x;

	const float2 cornerPos = UI_GetCursorPos(ui);
	const float2 size = { containerWidth, UI_TextHeight(ui) };

	const float controlHeight = UI_ControlHeight(ui);
	const float middle = controlHeight * 0.5f;
	const float padding = 4.0f;
	const float2 line1Pos = cornerPos + float2{0.0f, middle};
	const float2 line1Size = { 8.0f, 1.0 };
	const float2 textPos = UI_AdjustTextVertically(ui, cornerPos + float2{line1Size.x + padding, 0.0f}, controlHeight);
	const float2 textSize = UI_TextSize(ui, text);
	const float2 line2Pos = cornerPos + float2{line1Size.x + textSize.x + 2.0f*padding, middle};
	const float2 line2Size = float2{Max(0.0f, size.x - line1Size.x - textSize.x), line1Size.y};

	UI_PushColor(ui, ui.style.borderColor);
	UI_AddRectangle(ui, line1Pos, line1Size);
	UI_PopColor(ui);

	UI_AddText(ui, textPos, text);

	UI_PushColor(ui, ui.style.borderColor);
	UI_AddRectangle(ui, line2Pos, line2Size);
	UI_PopColor(ui);

	UI_CursorAdvance(ui, size, 12);
}

bool UI_Image(UI &ui, ImageH image, float2 proposedImageSize = float2{32, 32}, UIWidgetFlags flags = UIWidgetFlag_None, float4 uvRect = {0, 0, 1, 1})
{
	const float2 borderSize = { 1, 1 };

	float2 imageSize = proposedImageSize;
	if ( flags & UIWidgetFlag_Expand )
	{
		const UIWindow &window = UI_GetCurrentWindow(ui);
		const f32 containerWidth = UI_GetContainerSize(window).x;
		imageSize = float2{containerWidth, containerWidth} - 2.0f * borderSize;
	}

	float2 pos = UI_GetCursorPos(ui);
	if ( flags & UIWidgetFlag_Centered )
	{
		const UIWindow &window = UI_GetCurrentWindow(ui);
		const f32 containerWidth = UI_GetContainerSize(window).x;
		pos.x = window.pos.x + 0.5f * ( containerWidth - imageSize.x );
	}

	const float2 framePos = pos;
	const float2 frameSize = imageSize + 2.0f * borderSize;

	const float2 imagePos = framePos + borderSize;

	const float2 uvPos = uvRect.xy;
	const float2 uvSize = uvRect.zw;
	UI_PushDrawList(ui, image);
	UI_BeginWidget(ui, imagePos, imageSize);
	UI_AddQuad(ui, imagePos, imageSize, uvPos, uvSize, UiColorWhite);
	const bool clicked = UI_WidgetClicked(ui);
	UI_EndWidget(ui);
	UI_CursorAdvance(ui, imageSize);
	UI_PopDrawList(ui);

	if (flags & UIWidgetFlag_Outline )
	{
		UI_PushColor(ui, ui.style.accentColor);
		UI_AddBorder(ui, framePos, frameSize, 1);
		UI_PopColor(ui);
	}

	return clicked;
}

void UI_Text(UI &ui, const char *label, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	const float2 padding = UI_GetPadding(ui);
	const f32 textHeight = UI_TextHeight(ui);
	const f32 side = UI_ControlHeight(ui);

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{UI_GetAlignedWidgetWidth(ui), side};

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const float2 boxPos = widgetPos;
	const float2 boxSize = widgetSize;

	UI_PushColor(ui, UIElementBox);
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);

	const float2 textPos = boxPos + padding;

	UI_AddText(ui, textPos, text);

	UI_EndWidget(ui);

	const float2 labelPos = widgetPos + float2{widgetSize.x + ui.style.itemSpacing, padding.y};
	UI_AddText(ui, labelPos, label);

	UI_CursorAdvance(ui, widgetSize);
}

void UI_SetActiveWidget(UI &ui, UIID widgetId)
{
	ui.activeWidgetId = widgetId;
}

bool UI_IsActiveWidget(UI &ui, UIID widgetId)
{
	const bool isActive = widgetId == ui.activeWidgetId;
	return isActive;
}

enum UITextEditAction
{
	UITextEdit_None,
	UITextEdit_Updating,
	UITextEdit_Done,
	UITextEdit_Cancel,
};

enum UITextEditFilter
{
	UITextEditFilter_None,
	UITextEditFilter_Int,
	UITextEditFilter_Float,
};

UITextEditAction UI_UpdateText(UI &ui, char activeBuffer[UI_TEXT_BUFFER_SIZE], i32 &cursorIndex, UITextEditFilter filter = UITextEditFilter_None)
{
	UITextEditAction action = UITextEdit_Updating;

	const int len = StrLen(activeBuffer);
	char tmp[UI_TEXT_BUFFER_SIZE];

	if ( ui.input.chars.charCount > 0 )
	{
		StrCopy(tmp, activeBuffer + cursorIndex); // Save characters after the cursor
		for (u32 i = 0; i < ui.input.chars.charCount; ++i)
		{
			const char c = ui.input.chars.chars[i];
			if ( filter == UITextEditFilter_Int || filter == UITextEditFilter_Float )
			{
				const bool isDigit = c >= '0' && c <= '9';
				const bool isMinusSign = c == '-' && cursorIndex == 0;
				const bool isDot = filter == UITextEditFilter_Float && c == '.' && StrChar(activeBuffer, '.') == nullptr;
				if ( !isDigit && !isMinusSign && !isDot )
					continue;
			}
			activeBuffer[cursorIndex] = c;
			cursorIndex++;
		}
		StrCopy(activeBuffer + cursorIndex, tmp);
	}
	else if ( ui.input.keyboard.keys[K_BACKSPACE] == KEY_STATE_PRESS )
	{
		if (cursorIndex > 0) {
			StrCopy(tmp, activeBuffer + cursorIndex);
			StrCopy(activeBuffer + cursorIndex - 1, tmp);
			cursorIndex--;
		}
	}
	else if ( ui.input.keyboard.keys[K_DELETE] == KEY_STATE_PRESS )
	{
		if (cursorIndex < len) {
			StrCopy(tmp, activeBuffer + cursorIndex + 1);
			StrCopy(activeBuffer + cursorIndex, tmp);
		}
	}
	else if ( KeyPress(ui.input.keyboard, K_LEFT) )
	{
		cursorIndex = Max(cursorIndex - 1, 0);
	}
	else if ( KeyPress(ui.input.keyboard, K_RIGHT) )
	{
		cursorIndex = Min(cursorIndex + 1, len);
	}
	else if ( ui.input.keyboard.keys[K_RETURN] == KEY_STATE_PRESS )
	{
		action = UITextEdit_Done;
	}
	else if ( ui.input.keyboard.keys[K_ESCAPE] == KEY_STATE_PRESS )
	{
		action = UITextEdit_Cancel;
	}
	else
	{
		action = UITextEdit_None;
	}

	return action;
}

i32 UI_TextCursorIndexAtPos(const UI &ui, const char *text, f32 relativeX)
{
	const i32 len = StrLen(text);
	i32 cursorIndex = len;
	for (i32 i = 0; i <= len; ++i)
	{
		const f32 width = UI_TextWidth(ui, text, i);
		if (width >= relativeX)
		{
			if (i > 0)
			{
				const f32 prevWidth = UI_TextWidth(ui, text, i-1);
				const f32 midWidth = (prevWidth + width) * 0.5f;
				cursorIndex = (relativeX < midWidth) ? i - 1 : i;
			}
			else
			{
				cursorIndex = 0;
			}
			break;
		}
	}
	return cursorIndex;
}

bool UI_InputText(UI &ui, const char *label, char *buffer, u32 bufferSize)
{
	char bufferBeforeEdit[UI_TEXT_BUFFER_SIZE];
	StrCopy(bufferBeforeEdit, buffer);

	const float2 padding = UI_GetPadding(ui);
	const f32 textHeight = UI_TextHeight(ui);
	const f32 side = UI_ControlHeight(ui);

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{UI_GetAlignedWidgetWidth(ui), side};

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const float2 boxPos = widgetPos;
	const float2 boxSize = widgetSize;

	UI_PushColor(ui, UIElementInput);
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	//UI_PushColor(ui, ui.style.borderColor);
	//UI_AddBorder(ui, boxPos, boxSize, 1);
	//UI_PopColor(ui);

	const float2 textPos = boxPos + padding;

	const UIID id = UI_MakeID(ui, label);

	static char activeBuffer[UI_TEXT_BUFFER_SIZE] = {};
	static char originalBuffer[UI_TEXT_BUFFER_SIZE] = {};
	static Clock activeBeginClock;
	static i32 cursorIndex;

	const bool clicked = UI_WidgetClicked(ui);
	const bool active = UI_IsActiveWidget(ui, id);
	if (clicked && !active)
	{
		UI_SetActiveWidget(ui, id);
		StrCopy(activeBuffer, buffer);
		StrCopy(originalBuffer, buffer);
		activeBeginClock = GetClock();
		const f32 relativeX = UI_MousePos(ui).x - textPos.x;
		cursorIndex = UI_TextCursorIndexAtPos(ui, activeBuffer, relativeX);
	}

	if (active)
	{
		const UITextEditAction action = UI_UpdateText(ui, activeBuffer, cursorIndex);

		if ( action == UITextEdit_Cancel )
		{
			StrCopy(buffer, originalBuffer);
			UI_SetActiveWidget(ui, 0);
		}
		else
		{
			StrCopy(buffer, activeBuffer);
			if ( action == UITextEdit_Done )
			{
				UI_SetActiveWidget(ui, 0);
			}
			else if ( action == UITextEdit_Updating )
			{
				activeBeginClock = GetClock();
			}
		}

		const Clock currentClock = GetClock();
		const float secondsActive = GetSecondsElapsed(activeBeginClock, currentClock);

		const bool printCursor = (int)(secondsActive * 3) % 3 != 2; // Show 2/3 second, hide 1/3 second
		if ( printCursor )
		{
			const f32 textWidth = UI_TextWidth(ui, activeBuffer, cursorIndex);
			const float2 cursorPos = textPos + float2{textWidth + 1.0f, 0.0f};
			const float2 cursorSize = {1.0f, textHeight};
			UI_PushColor(ui, UiColorWhite);
			UI_AddRectangle(ui, cursorPos, cursorSize);
			UI_PopColor(ui);
		}
	}

	const char *text = active ? activeBuffer : buffer;
	UI_AddText(ui, textPos, text);

	const bool mouseClick = UI_IsMousePress(ui);
	const bool clickedInside = UI_WidgetClicked(ui);

	UI_EndWidget(ui);

	const float2 text2Pos = widgetPos + float2{widgetSize.x + ui.style.itemSpacing, padding.y};
	label = UI_RemoveNamePrefix(label);
	UI_AddText(ui, text2Pos, label);

	UI_CursorAdvance(ui, widgetSize);

	return !StrEq(buffer, bufferBeforeEdit);
}

bool UI_InputI64(UI &ui, const char *label, i64 *number, f32 spacing = UI_StyleSpacing)
{
	const i64 numberBeforeEdit = *number;

	const float2 padding = UI_GetPadding(ui);
	const f32 textHeight = UI_TextHeight(ui);

	const f32 side = UI_ControlHeight(ui);

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{UI_GetAlignedWidgetWidth(ui), side};

	const float2 boxPos = widgetPos;
	const float2 boxSize = widgetSize;

	const UIID id = UI_MakeID(ui, label);

	static char activeBuffer[UI_TEXT_BUFFER_SIZE] = {};
	static Clock activeBeginClock;
	static i32 cursorIndex;
	static i64 originalNumber;
	static bool dragging;
	static UIID draggingId;
	static i64 numberBeforeDrag;

	UI_BeginWidget(ui, boxPos, boxSize);
	UI_PushColor(ui, UIElementInput);
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	//UI_PushColor(ui, ui.style.borderColor);
	//UI_AddBorder(ui, boxPos, boxSize, 1);
	//UI_PopColor(ui);

	const bool boxClicked = UI_WidgetPressed(ui);
	const bool active = UI_IsActiveWidget(ui, id);

	if (boxClicked && !active)
	{
		dragging = true;
		draggingId = id;
		numberBeforeDrag = *number;
	}

	if (dragging && draggingId == id)
	{
		const f32 dragDelta = (f32)(UI_MousePos(ui).x - UI_LastMouseClickPos(ui).x);
		*number = numberBeforeDrag + (i64)dragDelta;

		if ( UI_IsMouseRelease(ui) )
		{
			dragging = false;
			if ( dragDelta > -ui.style.dragClickThreshold && dragDelta < ui.style.dragClickThreshold )
			{
				UI_SetActiveWidget(ui, id);
				SPrintf(activeBuffer, "%lld", *number);
				originalNumber = *number;
				activeBeginClock = GetClock();
				const float2 textOrigin = UI_AdjustTextHorizontally(ui, boxPos, boxSize.x, activeBuffer);
				const f32 relativeX = UI_MousePos(ui).x - textOrigin.x;
				cursorIndex = UI_TextCursorIndexAtPos(ui, activeBuffer, relativeX);
			}
		}
	}

	if (UI_WidgetHovered(ui))
	{
		const i32 wheel = UI_MouseScroll(ui).y;
		if (wheel != 0)
		{
			*number -= wheel; // Scrolling up (negative wheel) increases the number
			if (active)
			{
				SPrintf(activeBuffer, "%lld", *number);
				cursorIndex = StrLen(activeBuffer);
			}
		}
	}

	if (active)
	{
		const UITextEditAction action = UI_UpdateText(ui, activeBuffer, cursorIndex, UITextEditFilter_Int);

		if ( action == UITextEdit_Cancel )
		{
			*number = originalNumber;
			UI_SetActiveWidget(ui, 0);
		}
		else
		{
			if ( StrIsInteger(activeBuffer) )
			{
				*number = StrToI64(activeBuffer);
			}
			if ( action == UITextEdit_Done )
			{
				UI_SetActiveWidget(ui, 0);
			}
			else if ( action == UITextEdit_Updating )
			{
				activeBeginClock = GetClock();
			}
		}

		const Clock currentClock = GetClock();
		const float secondsActive = GetSecondsElapsed(activeBeginClock, currentClock);

		const bool printCursor = (int)(secondsActive * 3) % 3 != 2; // Show 2/3 second, hide 1/3 second
		if ( printCursor )
		{
			const float2 textOrigin = UI_AdjustTextHorizontally(ui, boxPos + float2{0.0f, padding.y}, boxSize.x, activeBuffer);
			const f32 prefixWidth = UI_TextWidth(ui, activeBuffer, cursorIndex);
			const float2 cursorPos = textOrigin + float2{prefixWidth + 1.0f, 0.0f};
			const float2 cursorSize = {1.0f, textHeight};
			UI_PushColor(ui, UiColorWhite);
			UI_AddRectangle(ui, cursorPos, cursorSize);
			UI_PopColor(ui);
		}
	}
	UI_EndWidget(ui);

	// Number
	if (active)
	{
		const float2 textPos = UI_AdjustTextHorizontally(ui, boxPos + float2{0.0f, padding.y}, boxSize.x, activeBuffer);
		UI_AddText(ui, textPos, activeBuffer);
	}
	else
	{
		char numberText[UI_TEXT_BUFFER_SIZE];
		SPrintf(numberText, "%lld", *number);
		const float2 textPos = UI_AdjustTextHorizontally(ui, boxPos + float2{0.0f, padding.y}, boxSize.x, numberText);
		UI_AddText(ui, textPos, numberText);
	}

	// Label
	const float2 labelPos = widgetPos + float2{widgetSize.x + ui.style.itemSpacing, padding.y};
	UI_AddText(ui, labelPos, label);

	UI_CursorAdvance(ui, widgetSize, spacing);

	return *number != numberBeforeEdit;
}

bool UI_InputInt(UI &ui, const char *label, i32 *number, f32 spacing = UI_StyleSpacing)
{
	i64 value = *number;
	const bool modified = UI_InputI64(ui, label, &value, spacing);
	*number = (i32)value;
	return modified;
}

bool UI_InputUInt(UI &ui, const char *label, u32 *number, f32 spacing = UI_StyleSpacing)
{
	i64 value = *number;
	const bool modified = UI_InputI64(ui, label, &value, spacing);
	if (value < 0) value = 0; // unsigned can't go below zero
	*number = (u32)value;
	return modified;
}

bool UI_InputFloat(UI &ui, const char *label, f32 *number, f32 step = 0.1f, f32 spacing = UI_StyleSpacing)
{
	const f32 numberBeforeEdit = *number;

	const float2 padding = UI_GetPadding(ui);
	const f32 textHeight = UI_TextHeight(ui);

	const f32 side = UI_ControlHeight(ui);

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{UI_GetAlignedWidgetWidth(ui), side};

	const float2 boxPos = widgetPos;
	const float2 boxSize = widgetSize;

	const UIID id = UI_MakeID(ui, label);

	static char activeBuffer[UI_TEXT_BUFFER_SIZE] = {};
	static Clock activeBeginClock;
	static i32 cursorIndex;
	static f32 originalNumber;
	static bool dragging;
	static UIID draggingId;
	static f32 numberBeforeDrag;

	UI_BeginWidget(ui, boxPos, boxSize);
	UI_PushColor(ui, UIElementInput);
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	//UI_PushColor(ui, ui.style.borderColor);
	//UI_AddBorder(ui, boxPos, boxSize, 1);
	//UI_PopColor(ui);

	const bool boxClicked = UI_WidgetPressed(ui);
	const bool active = UI_IsActiveWidget(ui, id);

	if (boxClicked && !active)
	{
		dragging = true;
		draggingId = id;
		numberBeforeDrag = *number;
	}

	if (dragging && draggingId == id)
	{
		const f32 dragDelta = (f32)(UI_MousePos(ui).x - UI_LastMouseClickPos(ui).x);
		*number = numberBeforeDrag + dragDelta * step;

		if ( UI_IsMouseRelease(ui) )
		{
			dragging = false;
			if ( dragDelta > -ui.style.dragClickThreshold && dragDelta < ui.style.dragClickThreshold )
			{
				UI_SetActiveWidget(ui, id);
				SPrintf(activeBuffer, "%f", *number);
				originalNumber = *number;
				activeBeginClock = GetClock();
				const float2 textOrigin = UI_AdjustTextHorizontally(ui, boxPos, boxSize.x, activeBuffer);
				const f32 relativeX = UI_MousePos(ui).x - textOrigin.x;
				cursorIndex = UI_TextCursorIndexAtPos(ui, activeBuffer, relativeX);
			}
		}
	}

	if (UI_WidgetHovered(ui))
	{
		const i32 wheel = UI_MouseScroll(ui).y;
		if (wheel != 0)
		{
			*number -= wheel * step; // Scrolling up (negative wheel) increases the number
			if (active)
			{
				SPrintf(activeBuffer, "%f", *number);
				cursorIndex = StrLen(activeBuffer);
			}
		}
	}

	if (active)
	{
		const UITextEditAction action = UI_UpdateText(ui, activeBuffer, cursorIndex, UITextEditFilter_Float);

		if ( action == UITextEdit_Cancel )
		{
			*number = originalNumber;
			UI_SetActiveWidget(ui, 0);
		}
		else
		{
			if ( StrIsFloat(activeBuffer) )
			{
				*number = StrToFloat(activeBuffer);
			}
			if ( action == UITextEdit_Done )
			{
				UI_SetActiveWidget(ui, 0);
			}
			else if ( action == UITextEdit_Updating )
			{
				activeBeginClock = GetClock();
			}
		}

		const Clock currentClock = GetClock();
		const float secondsActive = GetSecondsElapsed(activeBeginClock, currentClock);

		const bool printCursor = (int)(secondsActive * 3) % 3 != 2; // Show 2/3 second, hide 1/3 second
		if ( printCursor )
		{
			const float2 textOrigin = UI_AdjustTextHorizontally(ui, boxPos + float2{0.0f, padding.y}, boxSize.x, activeBuffer);
			const f32 prefixWidth = UI_TextWidth(ui, activeBuffer, cursorIndex);
			const float2 cursorPos = textOrigin + float2{prefixWidth + 1.0f, 0.0f};
			const float2 cursorSize = {1.0f, textHeight};
			UI_PushColor(ui, UiColorWhite);
			UI_AddRectangle(ui, cursorPos, cursorSize);
			UI_PopColor(ui);
		}
	}
	UI_EndWidget(ui);

	// Number
	if (active)
	{
		const float2 textPos = UI_AdjustTextHorizontally(ui, boxPos + float2{0.0f, padding.y}, boxSize.x, activeBuffer);
		UI_AddText(ui, textPos, activeBuffer);
	}
	else
	{
		char numberText[UI_TEXT_BUFFER_SIZE];
		SPrintf(numberText, "%f", *number);
		const float2 textPos = UI_AdjustTextHorizontally(ui, boxPos + float2{0.0f, padding.y}, boxSize.x, numberText);
		UI_AddText(ui, textPos, numberText);
	}

	// Label
	const float2 labelPos = widgetPos + float2{widgetSize.x + ui.style.itemSpacing, padding.y};
	UI_AddText(ui, labelPos, label);

	UI_CursorAdvance(ui, widgetSize, spacing);

	return *number != numberBeforeEdit;
}

bool UI_InputInt2(UI &ui, const char *label, int2 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 2.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputInt(ui, subLabel, &value->x, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputInt(ui, subLabel, &value->y);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}

bool UI_InputUInt2(UI &ui, const char *label, uint2 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 2.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputUInt(ui, subLabel, &value->x, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputUInt(ui, subLabel, &value->y);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}

bool UI_InputInt3(UI &ui, const char *label, int3 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 2.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputInt(ui, subLabel, &value->x, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputInt(ui, subLabel, &value->y, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Z");
	modified |= UI_InputInt(ui, subLabel, &value->z);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}

bool UI_InputInt4(UI &ui, const char *label, int4 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 2.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputInt(ui, subLabel, &value->x, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputInt(ui, subLabel, &value->y, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Z");
	modified |= UI_InputInt(ui, subLabel, &value->z, ui.style.itemSpacingTight);
	SPrintf(subLabel, "W");
	modified |= UI_InputInt(ui, subLabel, &value->w);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}

bool UI_InputFloat2(UI &ui, const char *label, float2 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 1.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputFloat(ui, subLabel, &value->x, 0.1f, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputFloat(ui, subLabel, &value->y);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}

bool UI_InputFloat3(UI &ui, const char *label, float3 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 1.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputFloat(ui, subLabel, &value->x, 0.1f, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputFloat(ui, subLabel, &value->y, 0.1f, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Z");
	modified |= UI_InputFloat(ui, subLabel, &value->z);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}

bool UI_InputFloat4(UI &ui, const char *label, float4 *value)
{
	const float2 padding = UI_GetPadding(ui);
	UI_PushPadding(ui, float2{padding.x, 1.0f});

	const UIID id = UI_MakeID(ui, label);
	UI_PushID(ui, id);

	bool modified = false;
	char subLabel[UI_LABEL_BUFFER_SIZE];
	SPrintf(subLabel, "X %s", label);
	modified |= UI_InputFloat(ui, subLabel, &value->x, 0.1f, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Y");
	modified |= UI_InputFloat(ui, subLabel, &value->y, 0.1f, ui.style.itemSpacingTight);
	SPrintf(subLabel, "Z");
	modified |= UI_InputFloat(ui, subLabel, &value->z, 0.1f, ui.style.itemSpacingTight);
	SPrintf(subLabel, "W");
	modified |= UI_InputFloat(ui, subLabel, &value->w);

	UI_PopID(ui);

	UI_PopPadding(ui);

	return modified;
}


////////////////////////////////////////////////////////////////////////
// Color picker

static float3 UI_RgbToHsv(float3 rgb)
{
	const f32 maxc = Max(Max(rgb.r, rgb.g), rgb.b);
	const f32 minc = Min(Min(rgb.r, rgb.g), rgb.b);
	const f32 delta = maxc - minc;

	f32 h = 0.0f;
	if (delta > 0.00001f)
	{
		if (maxc == rgb.r) {
			h = (rgb.g - rgb.b) / delta;
			if (h < 0.0f) h += 6.0f;
		} else if (maxc == rgb.g) {
			h = (rgb.b - rgb.r) / delta + 2.0f;
		} else {
			h = (rgb.r - rgb.g) / delta + 4.0f;
		}
		h /= 6.0f;
	}
	const f32 s = maxc <= 0.0f ? 0.0f : delta / maxc;
	const f32 v = maxc;
	return float3{ h, s, v };
}

static float3 UI_HsvToRgb(float3 hsv)
{
	const f32 h = Clamp(hsv.x, 0.0f, 1.0f) * 6.0f;
	const f32 s = Clamp(hsv.y, 0.0f, 1.0f);
	const f32 v = Clamp(hsv.z, 0.0f, 1.0f);

	const i32 i = Floor(h) % 6;
	const f32 f = h - (f32)Floor(h);
	const f32 p = v * (1.0f - s);
	const f32 q = v * (1.0f - f * s);
	const f32 t = v * (1.0f - (1.0f - f) * s);

	switch (i)
	{
		case 0:  return float3{ v, t, p };
		case 1:  return float3{ q, v, p };
		case 2:  return float3{ p, v, t };
		case 3:  return float3{ p, q, v };
		case 4:  return float3{ t, p, v };
		default: return float3{ v, p, q };
	}
}

// Draws a quad with a distinct color per corner, linearly interpolated across it (e.g. hue/SV gradients).
static void UI_AddGradientQuad(UI &ui, float2 pos, float2 size, float4 colorTL, float4 colorTR, float4 colorBL, float4 colorBR)
{
	const float2 uv = ui.whitePixelUv;
	const rgba cTL = Rgba(colorTL);
	const rgba cTR = Rgba(colorTR);
	const rgba cBL = Rgba(colorBL);
	const rgba cBR = Rgba(colorBR);

	const float2 v0 = pos;
	const float2 v1 = pos + float2{0.0f, size.y};
	const float2 v2 = pos + size;
	const float2 v3 = pos + float2{size.x, 0.0f};

	UI_AddTriangle(ui, UIVertex{v0, uv, cTL}, UIVertex{v1, uv, cBL}, UIVertex{v2, uv, cBR});
	UI_AddTriangle(ui, UIVertex{v0, uv, cTL}, UIVertex{v2, uv, cBR}, UIVertex{v3, uv, cTR});
}

// Small filled circle with a dark outline, used to mark a position on the SV box / strips
// regardless of the color underneath it.
static void UI_AddPickerMarker(UI &ui, float2 center, f32 radius)
{
	UI_PushColor(ui, float4{0.0f, 0.0f, 0.0f, 1.0f});
	UI_AddCircle(ui, center - float2{radius, radius}, radius);
	UI_PopColor(ui);

	const f32 innerRadius = radius - 1.5f;
	UI_PushColor(ui, UiColorWhite);
	UI_AddCircle(ui, center - float2{innerRadius, innerRadius}, innerRadius);
	UI_PopColor(ui);
}

// Thin horizontal handle marking a position within a vertical strip (hue / alpha).
static void UI_AddStripMarker(UI &ui, float2 stripPos, float2 stripSize, f32 normalizedY)
{
	constexpr f32 markerHeight = 3.0f;
	const float2 markerPos = stripPos + float2{-2.0f, Clamp(normalizedY * stripSize.y, 0.0f, stripSize.y) - markerHeight * 0.5f};
	const float2 markerSize = {stripSize.x + 4.0f, markerHeight};

	UI_PushColor(ui, float4{0.0f, 0.0f, 0.0f, 1.0f});
	UI_AddRectangle(ui, markerPos - float2{1.0f, 1.0f}, markerSize + float2{2.0f, 2.0f});
	UI_PopColor(ui);
	UI_PushColor(ui, UiColorWhite);
	UI_AddRectangle(ui, markerPos, markerSize);
	UI_PopColor(ui);
}

void UI_ColorPicker(UI &ui, float4 *color, bool *isOpen)
{
	constexpr float2 svSize = {160.0f, 160.0f};
	constexpr f32 stripWidth = 20.0f;
	constexpr f32 gap = 8.0f;

	const float2 padding = UI_GetPadding(ui);
	const f32 side = UI_ControlHeight(ui);

	static float3 activeHsv;
	static const float4 *activeColorPtr = nullptr;

	// True when the caller has just redirected the picker to a different color this frame (e.g.
	// clicking a different swatch button while the picker was already open editing another one).
	// That click necessarily lands outside the picker's own window, but it shouldn't be treated
	// as a dismiss-the-picker click below - it should keep the picker open, now showing the color
	// it was just switched to.
	const bool colorChangedThisFrame = (activeColorPtr != color);

	if (colorChangedThisFrame)
	{
		activeColorPtr = color;
		activeHsv = UI_RgbToHsv(float3{color->r, color->g, color->b});
	}
	else
	{
		// If RGB changed by some other means than this widget's own HSV controls
		// (e.g. the numeric fields below, or code elsewhere), resync from RGB.
		const float3 expectedRgb = UI_HsvToRgb(activeHsv);
		constexpr f32 epsilon = 1.0f / 512.0f;
		const bool changedExternally =
			Abs(expectedRgb.r - color->r) > epsilon ||
			Abs(expectedRgb.g - color->g) > epsilon ||
			Abs(expectedRgb.b - color->b) > epsilon;
		if (changedExternally)
		{
			activeHsv = UI_RgbToHsv(float3{color->r, color->g, color->b});
		}
	}

	const UIID colorPickerId = UI_MakeID(ui, "$colorpicker");
	const UIWindow &window = UI_FindOrCreateWindow(ui, colorPickerId, "Color picker");

	if ( isOpen != nullptr )
	{
		static bool wasOpen = false;

		const bool escape = ui.input.keyboard.keys[K_ESCAPE] == KEY_STATE_PRESS;
		const bool clickOutside = wasOpen && UI_IsMousePress(ui) && !UI_MouseInArea(ui, window.pos, window.size);
		const bool enter = ui.input.keyboard.keys[K_RETURN] == KEY_STATE_PRESS;
		wasOpen = *isOpen;

		if ( !colorChangedThisFrame && wasOpen && ( escape || clickOutside || enter ) )
		{
			// TODO: Revert changes (escape/clickOutside case)
			*isOpen = false;
			wasOpen = *isOpen;

			return;
		}
	}

	UI_SetNextWindowModal(ui, UIModalFlag_NoBackground);
	UI_SetNextWindowAnchor(ui, {0.5f, 0.5f});
	UI_SetNextWindowSize(ui, {234, 317});
	UI_BeginWindow(ui, colorPickerId, UIWindowFlag_Border | UIWindowFlag_Background | UIWindowFlag_Draggable | UIWindowFlag_Titlebar | UIWindowFlag_CloseButton | UIWindowFlag_ClipContents, isOpen);

	UI_MoveCursorRight(ui, padding.x);
	UI_MoveCursorDown(ui, padding.y);

	UI_BeginLayout(ui, UILayout_Horizontal);

	// Saturation/Value box (for the current hue)
	{
		const float2 svPos = UI_GetCursorPos(ui);
		UI_BeginWidget(ui, svPos, svSize);

		const float3 hueRgb = UI_HsvToRgb(float3{activeHsv.x, 1.0f, 1.0f});
		const float4 white = {1.0f, 1.0f, 1.0f, 1.0f};
		const float4 hueColor = {hueRgb.r, hueRgb.g, hueRgb.b, 1.0f};
		const float4 transparentBlack = {0.0f, 0.0f, 0.0f, 0.0f};
		const float4 opaqueBlack = {0.0f, 0.0f, 0.0f, 1.0f};
		UI_AddGradientQuad(ui, svPos, svSize, white, hueColor, white, hueColor);
		UI_AddGradientQuad(ui, svPos, svSize, transparentBlack, transparentBlack, opaqueBlack, opaqueBlack);

		static bool draggingSv = false;
		if (UI_WidgetPressed(ui)) {
			draggingSv = true;
		}
		if (draggingSv)
		{
			const float2 local = Float2(UI_MousePos(ui)) - svPos;
			activeHsv.y = Clamp(local.x / svSize.x, 0.0f, 1.0f);
			activeHsv.z = Clamp(1.0f - local.y / svSize.y, 0.0f, 1.0f);
			if (UI_IsMouseRelease(ui)) {
				draggingSv = false;
			}
		}

		const float2 markerPos = svPos + float2{ activeHsv.y * svSize.x, (1.0f - activeHsv.z) * svSize.y };
		UI_AddPickerMarker(ui, markerPos, 5.0f);

		UI_EndWidget(ui);
		UI_CursorAdvance(ui, svSize, gap);
	}

	// Hue strip
	{
		const float2 huePos = UI_GetCursorPos(ui);
		const float2 hueSize = {stripWidth, svSize.y};
		UI_BeginWidget(ui, huePos, hueSize);

		constexpr u32 hueStops = 6;
		for (u32 i = 0; i < hueStops; ++i)
		{
			const f32 t0 = (f32)i / hueStops;
			const f32 t1 = (f32)(i + 1) / hueStops;
			const float3 c0 = UI_HsvToRgb(float3{t0, 1.0f, 1.0f});
			const float3 c1 = UI_HsvToRgb(float3{t1, 1.0f, 1.0f});
			const float2 segPos = huePos + float2{0.0f, t0 * hueSize.y};
			const float2 segSize = {hueSize.x, hueSize.y / hueStops};
			const float4 col0 = {c0.r, c0.g, c0.b, 1.0f};
			const float4 col1 = {c1.r, c1.g, c1.b, 1.0f};
			UI_AddGradientQuad(ui, segPos, segSize, col0, col0, col1, col1);
		}

		static bool draggingHue = false;
		if (UI_WidgetPressed(ui)) {
			draggingHue = true;
		}
		if (draggingHue)
		{
			const float2 local = Float2(UI_MousePos(ui)) - huePos;
			activeHsv.x = Clamp(local.y / hueSize.y, 0.0f, 1.0f);
			if (UI_IsMouseRelease(ui)) {
				draggingHue = false;
			}
		}

		UI_AddStripMarker(ui, huePos, hueSize, activeHsv.x);

		UI_EndWidget(ui);
		UI_CursorAdvance(ui, hueSize, gap);
	}

	// Alpha strip (top = opaque, bottom = transparent)
	{
		const float2 alphaPos = UI_GetCursorPos(ui);
		const float2 alphaSize = {stripWidth, svSize.y};
		UI_BeginWidget(ui, alphaPos, alphaSize);

		const float3 rgb = UI_HsvToRgb(activeHsv);
		const float4 opaque = {rgb.r, rgb.g, rgb.b, 1.0f};
		const float4 transparent = {rgb.r, rgb.g, rgb.b, 0.0f};
		UI_AddGradientQuad(ui, alphaPos, alphaSize, opaque, opaque, transparent, transparent);

		static bool draggingAlpha = false;
		if (UI_WidgetPressed(ui)) {
			draggingAlpha = true;
		}
		if (draggingAlpha)
		{
			const float2 local = Float2(UI_MousePos(ui)) - alphaPos;
			color->a = Clamp(1.0f - local.y / alphaSize.y, 0.0f, 1.0f);
			if (UI_IsMouseRelease(ui)) {
				draggingAlpha = false;
			}
		}

		UI_AddStripMarker(ui, alphaPos, alphaSize, 1.0f - color->a);

		UI_EndWidget(ui);
		UI_CursorAdvance(ui, alphaSize, 0.0f);
	}

	UI_EndLayout(ui);

	// HSV is the source of truth for RGB while this panel is open (alpha is edited directly above).
	{
		const float3 rgb = UI_HsvToRgb(activeHsv);
		color->r = rgb.r;
		color->g = rgb.g;
		color->b = rgb.b;
	}

	UI_InputFloat(ui, "R", &color->r, 0.01f);
	UI_InputFloat(ui, "G", &color->g, 0.01f);
	UI_InputFloat(ui, "B", &color->b, 0.01f);
	UI_InputFloat(ui, "A", &color->a, 0.01f);

	UI_EndWindow(ui);
}


static void UI_AddMeter(UI &ui, f32 start, f32 end, const char *text)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float2 pos = UI_GetCursorPos(ui);
	const float2 size = { UI_GetContainerSize(window).x, UI_ControlHeight(ui) };

	UI_BeginWidget(ui, pos, size);

	const UIElementColor &meterColor = UI_GetElemColor(ui, UIElementMeter);
	const bool hovered = UI_WidgetHovered(ui);

	UI_PushColor(ui, meterColor.base);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	// Keep the bar visible (one pixel) even when the span rounds down to nothing
	const f32 x0 = Clamp(start, 0.0f, 1.0f);
	const f32 x1 = Clamp(end, x0, 1.0f);
	const float2 fillPos = pos + float2{Round(x0 * size.x), 0.0f};
	const float2 fillSize = { Max(Round(( x1 - x0 ) * size.x), 1.0f), size.y };

	UI_PushColor(ui, hovered ? meterColor.hovered : meterColor.active);
	UI_AddRectangle(ui, fillPos, fillSize);
	UI_PopColor(ui);

	// Text is clipped against the widget, so it can never spill out of the meter
	const float2 centeredPos = UI_AdjustTextHorizontally(ui, pos, size.x, text);
	const float2 textPos = UI_AdjustTextVertically(ui, centeredPos, size.y);
	UI_AddText(ui, textPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, size);
}

// A bar covering the [start, end] portion of the container, with the text centered
// on top of it. Both ends are normalized, so the bar can float anywhere along the
// track (a span in a timeline) instead of always growing from the left.
void UI_MeterRange(UI &ui, f32 start, f32 end, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	UI_AddMeter(ui, start, end, text);
}

// A bar growing from the left, filled up to fraction (0 to 1)
void UI_Meter(UI &ui, f32 fraction, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	UI_AddMeter(ui, 0.0f, fraction, text);
}

void UI_Histogram(UI &ui, const float *values, u32 valueCount, f32 maxValue = 1000.0f/120.0f)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 histogramWidth = UI_GetContainerSize(window).x;
	const f32 histogramHeight = 30.0f;
	const float2 histPos = UI_GetCursorPos(ui);
	const float2 histSize = {histogramWidth, histogramHeight};

	UI_BeginWidget(ui, histPos, histSize, false);

	const f32 barWidth = histogramWidth / valueCount;
	const float2 barBase = histPos + float2{0.0f, histSize.y};

	UI_PushColor(ui, ui.style.accentColor);
	for (u32 i = 0; i < valueCount; ++i)
	{
		const float heightRatio = values[i] / maxValue;
		const f32 barHeight = Max(1.0f , Min(heightRatio * histogramHeight, histogramHeight));
		const float2 barPos = barBase + float2{ i * barWidth, -barHeight };
		const float2 barSize = {barWidth - 1, barHeight};
		UI_AddRectangle(ui, barPos, barSize);
	}
	UI_PopColor(ui);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, histSize);
}

////////////////////////////////////////////////////////////////////////
// Tables
//
// A table lays out rows top to bottom and cells left to right. Cells are plain
// layout groups, so anything can go inside one:
//
//   UI_BeginTable(ui, "Entities", 3);
//   UI_TableSetupColumn(ui, "Name");
//   UI_TableSetupColumn(ui, "Type", UITableColumnSizing_Fixed, 80.0f);
//   UI_TableSetupColumn(ui, "Visible", UITableColumnSizing_Fixed, 60.0f);
//   for (u32 i = 0; i < entityCount; ++i) {
//       if ( UI_TableNextRow(ui, i == selectedEntity) ) { selectedEntity = i; }
//       UI_TableNextColumn(ui);
//       // Draw some widgets here...
//       UI_TableNextColumn(ui);
//       // Draw some other widgets here...
//       UI_TableNextColumn(ui);
//       // Draw some more widgets...
//   }
//   UI_EndTable(ui);
//
// Rows are one line tall (UI_ControlHeight, or whatever UI_BeginTable was given)
// and cell contents are clipped to their cell, so a table never grows to fit an
// oversized cell.

static UITableState &UI_FindOrCreateTableState(UI &ui, UIID tableId)
{
	for (u32 i = 0; i < ui.tableStateCount; ++i)
	{
		if ( ui.tableStates[i].id == tableId )
		{
			return ui.tableStates[i];
		}
	}

	ASSERT(ui.tableStateCount < ARRAY_COUNT(ui.tableStates));
	UITableState &state = ui.tableStates[ui.tableStateCount++];
	state = {};
	state.id = tableId;
	state.resizedColumn = -1;
	return state;
}

UITable &UI_GetCurrentTable(UI &ui)
{
	ASSERT(ui.tableStackSize > 0);
	return ui.tableStack[ui.tableStackSize-1];
}

void UI_BeginTable(UI &ui, const char *name, u32 columnCount, u32 flags = UITableFlag_Default, f32 rowHeight = 0.0f)
{
	ASSERT(ui.tableStackSize < ARRAY_COUNT(ui.tableStack));
	ASSERT(columnCount > 0 && columnCount <= UI_MAX_TABLE_COLUMNS);

	const UIWindow &window = UI_GetCurrentWindow(ui);
	const UIID id = UI_MakeID(ui, name);
	UITableState &state = UI_FindOrCreateTableState(ui, id);

	if ( state.columnCount != columnCount )
	{
		// The table changed shape, so widths dragged for the old columns mean nothing now
		state.columnCount = columnCount;
		state.resizedColumn = -1;
		for (u32 i = 0; i < ARRAY_COUNT(state.userWidths); ++i) {
			state.userWidths[i] = 0.0f;
		}
	}

	// Same reasoning as UI_GetAlignedWidgetWidth: the table takes whatever is left
	// of the container from wherever the cursor already sits.
	const f32 contentOriginX = ui.layoutGroups[window.contentLayoutGroup].pos.x;
	const f32 offset = UI_GetCursorPos(ui).x - contentOriginX;

	UITable &table = ui.tableStack[ui.tableStackSize++];
	table = {};
	table.id = id;
	table.state = &state;
	table.flags = flags;
	table.pos = UI_GetCursorPos(ui);
	table.width = Max(UI_GetContainerSize(window).x - offset, 0.0f);
	table.rowHeight = rowHeight > 0.0f ? rowHeight : UI_ControlHeight(ui);
	table.columnCount = columnCount;
	table.rowIndex = -1;
	table.columnIndex = -1;

	UI_BeginLayout(ui, UILayout_Vertical);
}

void UI_TableSetupColumn(UI &ui, const char *label, UITableColumnSizing sizing = UITableColumnSizing_Stretch, f32 size = 1.0f)
{
	UITable &table = UI_GetCurrentTable(ui);
	ASSERT(!table.resolved); // Columns are described before the header and the first row
	ASSERT(table.setupCount < table.columnCount);

	UITableColumn &column = table.columns[table.setupCount++];
	StrCopyN(column.label, label, ARRAY_COUNT(column.label) - 1);
	column.sizing = sizing;
	column.size = size;
}

// Turns the declared columns into pixel widths and offsets. Called once per frame
// by whatever comes first, the header row or the first row.
static void UI_TableResolveColumns(UI &ui, UITable &table)
{
	if ( table.resolved ) {
		return;
	}

	UITableState &state = *table.state;

	// Columns nobody described take an equal share of the leftover width
	for (u32 i = table.setupCount; i < table.columnCount; ++i)
	{
		UITableColumn &column = table.columns[i];
		column.label[0] = 0;
		column.sizing = UITableColumnSizing_Stretch;
		column.size = 1.0f;
	}
	table.setupCount = table.columnCount;

	// Advance a drag started on a previous frame before measuring anything
	if ( state.resizedColumn >= 0 )
	{
		if ( UI_IsMouseIdle(ui) )
		{
			state.resizedColumn = -1;
		}
		else
		{
			const f32 delta = (f32)( UI_MousePos(ui).x - UI_LastMouseClickPos(ui).x );
			state.userWidths[state.resizedColumn] = Max(state.widthBeforeResize + delta, ui.style.tableMinColumnWidth);
		}
	}

	// Fixed columns (declared as such, or pinned by a drag) keep their width, the
	// rest share what is left proportionally to their weight
	f32 fixedWidth = 0.0f;
	f32 totalWeight = 0.0f;
	for (u32 i = 0; i < table.columnCount; ++i)
	{
		const UITableColumn &column = table.columns[i];
		const bool fixed = state.userWidths[i] > 0.0f || column.sizing == UITableColumnSizing_Fixed;
		if ( fixed ) {
			const f32 width = state.userWidths[i] > 0.0f ? state.userWidths[i] : column.size;
			fixedWidth += Max(width, ui.style.tableMinColumnWidth);
		} else {
			totalWeight += Max(column.size, 0.0f);
		}
	}

	const f32 stretchWidth = Max(table.width - fixedWidth, 0.0f);

	f32 offset = 0.0f;
	for (u32 i = 0; i < table.columnCount; ++i)
	{
		UITableColumn &column = table.columns[i];
		const bool fixed = state.userWidths[i] > 0.0f || column.sizing == UITableColumnSizing_Fixed;
		const f32 width = fixed ?
			( state.userWidths[i] > 0.0f ? state.userWidths[i] : column.size ) :
			( totalWeight > 0.0f ? stretchWidth * Max(column.size, 0.0f) / totalWeight : 0.0f );

		column.width = Max(Round(width), ui.style.tableMinColumnWidth);
		column.offset = offset;
		offset += column.width;
	}

	table.resolved = true;
}

static void UI_TableEndCell(UI &ui, UITable &table)
{
	if ( !table.cellOpen ) {
		return;
	}

	UIWindow &window = UI_GetCurrentWindow(ui);
	window.containerSize = table.containerSizeBackup;
	window.contentLayoutGroup = table.contentLayoutGroupBackup;

	UI_EndLayout(ui, false); // The table tracks its own size, no need to grow anything
	UI_PopCursorPos(ui);
	UI_EndWidget(ui);

	table.cellOpen = false;
}

static void UI_TableEndRow(UI &ui, UITable &table)
{
	UI_TableEndCell(ui, table);

	if ( !table.rowOpen ) {
		return;
	}

	UI_EndWidget(ui);
	table.rowOpen = false;
}

bool UI_TableNextRow(UI &ui, bool selected = false)
{
	UITable &table = UI_GetCurrentTable(ui);
	UI_TableResolveColumns(ui, table);
	UI_TableEndRow(ui, table);

	table.rowIndex++;
	table.columnIndex = -1;
	table.rowPos = table.pos + float2{0.0f, table.height};
	table.height += table.rowHeight;

	const float2 rowSize = { table.width, table.rowHeight };

	// The row widget stays open until the next row, so cell text clips against it
	UI_BeginWidget(ui, table.rowPos, rowSize);
	table.rowOpen = true;

	const bool highlight = ( table.flags & UITableFlag_RowHighlight ) && UI_WidgetHovered(ui);
	const UIElementColor &rowColor = UI_GetElemColor(ui, UIElementTableRow);
	const float4 color = selected ? rowColor.active : ( highlight ? rowColor.hovered : rowColor.base );

	if ( color.a > 0.0f )
	{
		UI_PushColor(ui, color);
		UI_AddRectangle(ui, table.rowPos, rowSize);
		UI_PopColor(ui);
	}

	const bool clicked = UI_WidgetClicked(ui);

	return clicked;
}

void UI_TableNextColumn(UI &ui)
{
	UITable &table = UI_GetCurrentTable(ui);
	ASSERT(table.rowOpen); // Cells belong to a row started by UI_TableNextRow
	ASSERT(table.columnIndex + 1 < (i32)table.columnCount);

	UI_TableEndCell(ui, table);

	const UITableColumn &column = table.columns[++table.columnIndex];

	const float2 padding = UI_GetPadding(ui);
	const float2 cellPos = table.rowPos + float2{column.offset, 0.0f};
	const float2 cellSize = { column.width, table.rowHeight };

	UI_BeginWidget(ui, cellPos, cellSize, false); // The row already blocks window interaction
	UI_PushCursorPos(ui, cellPos + float2{padding.x, 0.0f});
	UI_BeginLayout(ui, UILayout_Horizontal);

	// Widgets that measure themselves against the container must fit the cell
	UIWindow &window = UI_GetCurrentWindow(ui);
	table.containerSizeBackup = window.containerSize;
	table.contentLayoutGroupBackup = window.contentLayoutGroup;
	window.containerSize = { Max(cellSize.x - 2.0f * padding.x, 0.0f), cellSize.y };
	window.contentLayoutGroup = ui.layoutGroupCount - 1;

	table.cellOpen = true;
}

void UI_EndTable(UI &ui)
{
	UITable &table = UI_GetCurrentTable(ui);
	UI_TableEndRow(ui, table);

	const float2 size = { table.width, table.height };

	UI_EndLayout(ui, false);
	UI_SetCursorPos(ui, table.pos);
	UI_CursorAdvance(ui, size);

	ASSERT(ui.tableStackSize > 0);
	ui.tableStackSize--;
}


bool UI_BeginMenuBar(UI &ui)
{
	ASSERT(ui.windowStackSize == 0);
	ui.menuBarBegan = true;

	const char *idString = "UI_BeginMenuBar";
	const UIID windowId = UI_MakeID(ui, idString);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, idString);
	UI_BeginWindow(ui, windowId, UIWindowFlag_None);

	window.pos = {0, 0};
	window.anchor = {0, 0};
	window.displacement = {0, 0};
	window.size = {(f32)ui.viewportSize.x, ui.style.menuBarHeight};

	const float2 pos = window.pos;
	const float2 size = window.size;

	UI_PushColor(ui, UIElementMenu);
	UI_AddRectangle(ui, pos, size);

	const float2 borderPos = pos + float2{0.0f, size.y};
	const float2 borderSize = float2{size.x, 1.0f};
	UI_PushColor(ui, ui.style.borderColor);
	UI_AddRectangle(ui, borderPos, borderSize);
	UI_PopColor(ui);

	UI_SetCursorPos( ui, pos + float2{ui.style.itemSpacing, 0.0f} );
	UI_BeginLayout(ui, UILayout_Horizontal);

	return true;
}

void UI_EndMenuBar(UI &ui)
{
	ASSERT(ui.menuBarBegan);
	ui.menuBarBegan = false;

	UI_EndLayout(ui);
	UI_PopColor(ui);
	UI_EndWindow(ui);
}

bool UI_BeginToolBar(UI &ui)
{
	ASSERT(ui.windowStackSize == 0);
	ui.toolBarBegan = true;

	const char *idString = "UI_BeginToolBar";
	const UIID windowId = UI_MakeID(ui, idString);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, idString);
	UI_BeginWindow(ui, windowId, UIWindowFlag_None);

	// Sits directly under the menu bar and its bottom border.
	window.pos = {0, ui.style.menuBarHeight + ui.style.borderSize.y};
	window.anchor = {0, 0};
	window.displacement = window.pos;
	window.size = {(f32)ui.viewportSize.x, ui.style.menuBarHeight};

	const float2 pos = window.pos;
	const float2 size = window.size;

	UI_PushColor(ui, UIElementMenu);
	UI_AddRectangle(ui, pos, size);

	const float2 borderPos = pos + float2{0.0f, size.y};
	const float2 borderSize = float2{size.x, 1.0f};
	UI_PushColor(ui, ui.style.borderColor);
	UI_AddRectangle(ui, borderPos, borderSize);
	UI_PopColor(ui);

	UI_SetCursorPos(ui, pos + float2{ui.style.itemSpacing, 0.0f});
	UI_BeginLayout(ui, UILayout_Horizontal);

	return true;
}

void UI_EndToolBar(UI &ui)
{
	ASSERT(ui.toolBarBegan);
	ui.toolBarBegan = false;

	UI_EndLayout(ui);
	UI_PopColor(ui);
	UI_EndWindow(ui);
}

bool UI_BeginMenu(UI &ui, const char *name, bool *isOpen = nullptr)
{
	const UIID windowId = UI_MakeID(ui, name);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, name);

	if ( ui.menuBarBegan )
	{
		constexpr float2 margin = {8.0f, 3.0f};

		const float2 textSize = UI_TextSize(ui, name);
		const float2 itemSize = textSize + 2.0f * margin;

		const float2 itemPos = UI_GetCursorPos(ui);
		const float2 textPos = itemPos + margin;

		UI_BeginWidget(ui, itemPos, itemSize);
		const bool clicked = UI_WidgetClicked(ui);
		const bool hovered = UI_WidgetHovered(ui);
		UI_PushColor(ui, UIElementMenu);
		UI_AddRectangle(ui, itemPos, itemSize);
		UI_PopColor(ui);
		UI_AddText(ui, textPos, name);
		UI_EndWidget(ui);

		UI_CursorAdvance(ui, itemSize, 0.0f);

		// Show on click
		if (clicked) {
			if ( ui.activeMenu == &window ) {
				ui.activeMenu = nullptr;
			} else {
				ui.activeMenu = &window;
			}
		}

		// Show on hovering another menu
		if (hovered) {
			if (ui.activeMenu != nullptr) {
				ui.activeMenu = &window;
			}
		}

		const float2 menuPos = itemPos + dY(itemSize);
		UI_PositionWindow(window, menuPos);
	}
	else
	{
		// Only open on the frame the caller first requests it (isOpen going false->true).
		// Once open, ui.activeMenu is the source of truth so that outside clicks / moving
		// the mouse away (handled in UI_BeginFrame) can actually close the menu instead of
		// being immediately overridden here by a stale *isOpen that was never reset.
		if ( isOpen && *isOpen && !window.wasMenuOpen ) {
			ui.activeMenu = &window;
		}
	}

	const bool showMenu = ui.activeMenu == &window;
	window.wasMenuOpen = showMenu;
	if ( showMenu )
	{
		UI_BeginWindow(ui, windowId, UIWindowFlag_None);
		UI_RaiseWindow(ui, window);

		// Add the rectangle background, save its vertices to modify the size later.
		// UI_AddRectangle can emit nothing (scissor culled, or the vertex buffer is
		// full), so only keep the pointer if the 6 vertices were really written.
		UIVertex *rectVertexPtr = ui.vertexPtr;
		const float2 tempSize = {0, 0};
		UI_AddRectangle(ui, window.pos, tempSize );
		ui.activeMenuVertexPtr = ui.vertexPtr == rectVertexPtr + 6 ? rectVertexPtr : nullptr;
	}

	if ( isOpen ) {
		*isOpen = showMenu;
	}

	return showMenu;
}

void UI_EndMenu(UI &ui)
{
	UILayoutGroup &layoutGroup = UI_GetLayoutGroup(ui);
	UIWindow &window = UI_GetCurrentWindow(ui);
	window.size = layoutGroup.size;

	// Set the size of the menu background panel (skipped if its vertices were dropped)
	if ( ui.activeMenuVertexPtr )
	{
		// Tri 1
		ui.activeMenuVertexPtr[1].position.y += window.size.y;
		ui.activeMenuVertexPtr[2].position.x += window.size.x;
		ui.activeMenuVertexPtr[2].position.y += window.size.y;
		// Tri 2
		ui.activeMenuVertexPtr[4].position.x += window.size.x;
		ui.activeMenuVertexPtr[4].position.y += window.size.y;
		ui.activeMenuVertexPtr[5].position.x += window.size.x;
	}

	UI_EndWindow(ui);
}

bool UI_BeginContextMenu(UI &ui, const char *name, bool *isOpen = nullptr)
{
	const UIID windowId = UI_MakeID(ui, name);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, name);

	if (UI_IsMousePress(ui, MOUSE_BUTTON_RIGHT) && UI_MouseInArea(ui, ui.lastWidgetPos, ui.lastWidgetSize))
	{
		ui.activeMenu = &window;
		const float2 pos = Float2(ui.input.lastMouseClickPos);
		UI_SetNextWindowDisplacement(ui, pos);
	}

	return UI_BeginMenu(ui, name, isOpen);
}

void UI_EndContextMenu(UI &ui)
{
	UI_EndMenu(ui);
}

bool UI_MenuItem(UI &ui, const char *name, bool checked = false)
{
	constexpr float2 padding = {8.0f, 4.0f};
	const float2 textSize = UI_TextSize(ui, name);
	const float2 checkSize = float2{textSize.y, textSize.y};
	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{ textSize.x, textSize.y } + dX(padding) + dX(checkSize) + 2.0f * padding;

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const UIWindow &menu = UI_GetCurrentWindow(ui);
	const float2 extWidgetSize = Max(widgetSize, dY(widgetSize) + dX(menu.size));
	const bool hovered = UI_WidgetHovered(ui, widgetPos, extWidgetSize);
	if ( hovered )
	{
		UI_PushColor(ui, UI_GetElemColor(ui, UIElementMenu).hovered);
		UI_AddRectangle(ui, widgetPos, extWidgetSize);
		UI_PopColor(ui);
	}

	const bool clicked = UI_WidgetClicked(ui, widgetPos, extWidgetSize);
	UI_EndWidget(ui);

	const float2 textPos = widgetPos + padding;
	UI_AddText(ui, textPos, name);

	if (checked)
	{
		UI_PushColor(ui, UiColorWhite);
		const float2 n0 = { 0.131, 1.0 - 0.592};
		const float2 n1 = { 0.0, 1.0 - 0.4};
		const float2 n2 = { 0.493, 1.0 - 0.052};
		const float2 n3 = { 1.0, 1.0 - 0.778};
		const float2 n4 = { 0.81, 1.0 - 0.991};
		const float2 n5 = { 0.4364, 1.0 - 0.3771};
		const float2 checkPos = widgetPos + padding + dX(textSize) + dX(padding);
		const float2 p0 = checkPos + checkSize * n0;
		const float2 p1 = checkPos + checkSize * n1;
		const float2 p2 = checkPos + checkSize * n2;
		const float2 p3 = checkPos + checkSize * n3;
		const float2 p4 = checkPos + checkSize * n4;
		const float2 p5 = checkPos + checkSize * n5;
		UI_AddTriangle(ui, p0, p1, p2, UiColorWhite );
		UI_AddTriangle(ui, p0, p2, p5, UiColorWhite );
		UI_AddTriangle(ui, p2, p3, p4, UiColorWhite );
		UI_AddTriangle(ui, p5, p2, p4, UiColorWhite );
		UI_PopColor(ui);
	}

	UI_CursorAdvance(ui, widgetSize, 0);

	if (clicked) {
		ui.activeMenu = nullptr;
	}

	return clicked;
}

// Returns true on the frame a button is pressed, writing its index into *result.
// *result is left at -1 while no button has been pressed.
bool UI_MessageBox(UI &ui, const char *caption, const char *text, const char **buttons, i32 *result)
{
	UI_SetNextWindowModal(ui);
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});
	UI_SetNextWindowSize(ui, uint2{ 350, 120 });
	UI_SetNextWindowDefaultDisplacement(ui, {0, 0});

	constexpr u32 flags = UIWindowFlag_Draggable | UIWindowFlag_Titlebar | UIWindowFlag_Border | UIWindowFlag_Background | UIWindowFlag_ClipContents;
	UI_BeginWindow(ui, caption, nullptr, flags);
	UI_Label(ui, text);

	UI_BeginLayout(ui, UILayout_Horizontal);

	*result = -1;
	i32 res = 0;
	while (*buttons)
	{
		if ( UI_Button(ui, *buttons) ) {
			*result = res;
		}
		buttons++;
		res++;
	}

	UI_EndLayout(ui);

	UI_EndWindow(ui);

	return *result != -1;
}

inline UIPayload UI_Payload(void *ptr) {
	UIPayload payload = { .ptr = ptr };
	return payload;
}

inline UIPayload UI_Payload(u32 val) {
	UIPayload payload = { .uvalue = val };
	return payload;
}

void UI_DragAndDropSource(UI &ui, const char *payloadType, UIPayload payload, ImageH imageH, float4 uvRect = {0, 0, 0, 0})
{
	// Applies to the widget that was just ended, which UI_EndWidget recorded here.
	const float2 prevWidgetPos = ui.lastWidgetPos;
	const float2 prevWidgetSize = ui.lastWidgetSize;
	const bool clicked = UI_IsMousePress(ui) && UI_WidgetHovered(ui, prevWidgetPos, prevWidgetSize);
	if (clicked)
	{
		ui.dragAndDrop.payloadType = payloadType;
		ui.dragAndDrop.payload = payload;
		ui.dragAndDrop.imageH = imageH;
		ui.dragAndDrop.uvRect = uvRect;
	}
}

bool UI_DragAndDropTarget(UI &ui, const char *payloadType)
{
	bool handleDrop = false;

	// Applies to the widget that was just ended, which UI_EndWidget recorded here.
	const float2 prevWidgetPos = ui.lastWidgetPos;
	const float2 prevWidgetSize = ui.lastWidgetSize;
	const bool hovered = UI_WidgetHovered(ui, prevWidgetPos, prevWidgetSize);
	if ( hovered && StrEq(ui.dragAndDrop.payloadType, payloadType) )
	{
		UI_PushColor(ui, ui.style.accentColor);
		UI_AddBorder(ui, prevWidgetPos, prevWidgetSize, ui.style.borderSize.x);
		UI_PopColor(ui);

		handleDrop = UI_IsMouseRelease(ui);
	}

	return handleDrop;
}

// If a drop was lost we will likely treat it globally (e.g. putting someting on the editor scene) anyways
bool UI_DragAndDropTargetLost(UI &ui, const char *payloadType)
{
	if ( UI_IsMouseIdle(ui) && !ui.hoveredWindow && ui.dragAndDrop.payload.ptr && StrEq(ui.dragAndDrop.payloadType, payloadType) )
	{
		return true;
	}

	return false;
}

UIPayload UI_DragAndDropPayload(UI &ui)
{
	return ui.dragAndDrop.payload;
}

// TODO: We should depend only on ilu_gfx.h while this is a feature in engine.cpp.
struct Graphics;
ImageH GfxCreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels);

UIStyle UI_StyleDefault()
{
	UIStyle style = {};

	style.colors[UIElementText]       = { UiColorWhite,      UiColorWhite,          UiColorWhite,          UiColorWhite };
	style.colors[UIElementBackground] = { UiColorBackground, UiColorBackground,     UiColorBackground,     UiColorBackground };
	style.colors[UIElementSection]    = { UiColorSection,    UiColorSectionHover,   UiColorSectionHover,   UiColorSection };
	style.colors[UIElementButton]     = { UiColorButton,     UiColorButtonHover,    UiColorButtonHover,    UiColorButton };
	style.colors[UIElementToggle]     = { UiColorToggle,     UiColorToggleHover,    UiColorToggleHover,    UiColorToggle };
	style.colors[UIElementInput]      = { UiColorInput,      UiColorInputHover,     UiColorInputHover,     UiColorInput };
	style.colors[UIElementBox]        = { UiColorBox,        UiColorBoxHover,       UiColorBoxHover,       UiColorBox };
	style.colors[UIElementScrollbar]  = { UiColorScrollbar,  UiColorScrollbarHover, UiColorScrollbarHover, UiColorScrollbar };
	style.colors[UIElementMenu]       = { UiColorMenu,       UiColorMenuHover,      UiColorMenuHover,      UiColorMenu };
	style.colors[UIElementCaption]    = { UiColorCaption,    UiColorCaption,        UiColorCaption,        UiColorCaptionInactive };
	style.colors[UIElementTableHeader]= { UiColorTableHeader,UiColorTableHeaderHover, UiColorTableHeaderHover, UiColorTableHeader };
	style.colors[UIElementTableRow]   = { UiColorTableRow,   UiColorTableRowHover,  UiColorTableRowSelected, UiColorTableRow };
	style.colors[UIElementMeter]      = { UiColorMeter,      UiColorMeterFillHover, UiColorMeterFill,      UiColorMeter };

	style.borderColor = UiColorBorder;
	style.accentColor = UiColorAccent;
	style.modalOverlayColor = { 0.0f, 0.0f, 0.0f, 0.8f };

	style.borderSize = { 1.0f, 1.0f };
	style.windowPadding = { 4.0f, 8.0f };
	style.framePadding = { 4.0f, 3.0f };
	style.minWindowSize = { 100.0f, 100.0f };
	style.itemSpacing = 8.0f;
	style.itemSpacingTight = 2.0f;
	style.indentWidth = 8.0f;
	style.titlebarHeight = 18.0f;
	style.menuBarHeight = 22.0f;
	style.scrollbarWidth = 4.0f;
	style.scrollSpeed = 16.0f;
	style.resizeCornerSize = 14.0f;
	style.dragClickThreshold = 3.0f;
	style.labelRatio = 0.6f;
	style.tableGripWidth = 6.0f;
	style.tableMinColumnWidth = 20.0f;

	return style;
}

// Drops every temporary override so the theme in ui.style is what widgets see.
void UI_ResetStyleOverrides(UI &ui)
{
	for (u32 i = 0; i < UIElementCount; ++i) {
		ui.colorElems[i].stackSize = 0;
	}
	ui.paddingStackSize = 0;
}

// Restores the built-in theme, discarding both runtime edits and any override
// left on the stacks. Also what a DLL hot-reload calls so palette changes made
// in UI_StyleDefault show up without restarting.
void UI_ResetStyle(UI &ui)
{
	ui.style = UI_StyleDefault();
	UI_ResetStyleOverrides(ui);
}

void UI_Initialize(UI &ui, Graphics &gfx, GraphicsDevice &gfxDev, Arena &globalArena, UIIcon *icons, u32 iconCount)
{
	ui.tempString = PushArray(globalArena, char, UI_TEMP_STRING_SIZE);

	const u32 vertexBufferSize = UI_VERTEX_BUFFER_SIZE;
	ui.vertexCountLimit = vertexBufferSize / sizeof(UIVertex);
	ui.frontendVertices = PushArray(globalArena, UIVertex, ui.vertexCountLimit);

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		ui.vertexBuffer[i] = CreateBuffer(
			gfxDev,
			vertexBufferSize,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);

		ui.backendVertices[i] = (UIVertex*)GetBufferPtr(gfxDev, ui.vertexBuffer[i]);
	}

	UI_ResetStyle(ui);

	// Bottom of the draw-colour stack; UI_PopColor refuses to pop it.
	UI_PushColor(ui, ui.style.colors[UIElementButton].base);

	struct SizedFont
	{
		const char *filename;
		i32 height;
	};

	const SizedFont fonts[] = {
		{ "editor/fonts/proggy/ProggyClean.ttf", 13 },
		{ "editor/fonts/pixelarial/PIXEARG_.ttf", 11 },
		{ "editor/fonts/refixedsys/refixedsys-mono.ttf", 15 },
		//{ "editor/fonts/kode.ttf", 17 },
	};

	Scratch scratch;

	// Load TTF font texture
	SizedFont sizedFont = {};
	FilePath fontPath = {};
	DataChunk *chunk = nullptr;
	for (u32 i = 0; i < ARRAY_COUNT(fonts); ++i)
	{
		const u32 fontIndex = ARRAY_COUNT(fonts) - i - 1;
		sizedFont = fonts[fontIndex];
		fontPath = MakePath(ProjectDir, sizedFont.filename);
		chunk = PushFile( scratch.arena, fontPath.str );
		if (chunk) {
			break;
		}
	}

	if ( !chunk )
	{
		LOG(Error, "Could not open file %s\n", fontPath.str);
		return;
	}
	const byte *fontData = chunk->bytes;

	const u32 fontAtlasWidth = 128;
	const u32 fontAtlasHeight = 128;
	byte *fontAtlasBitmap = PushArray(scratch.arena, byte, fontAtlasWidth * fontAtlasHeight);

	stbtt_fontinfo font;
	const int fontIndex = 0;
	const int fontOffset = stbtt_GetFontOffsetForIndex(fontData, fontIndex);
	const f32 pixelHeight = (f32)sizedFont.height;
	if ( stbtt_InitFont(&font, fontData, fontOffset) )
	{
		ui.fontScale = stbtt_ScaleForPixelHeight(&font, pixelHeight);
		int fontAscent, fontDescent, fontLineGap;
		stbtt_GetFontVMetrics(&font, &fontAscent, &fontDescent, &fontLineGap);
		ui.fontAscent = fontAscent * ui.fontScale;
		ui.fontDescent = fontDescent * ui.fontScale;
		ui.fontLineGap = fontLineGap * ui.fontScale;
	}

	// Begin packing
	stbtt_pack_context packContext;
	const int strideInBytes = fontAtlasWidth; // Could be 0 to indicate 'tightly packed'
	const int padding = 1;
	void *allocContext = nullptr;
	const int res = stbtt_PackBegin(&packContext, fontAtlasBitmap, fontAtlasWidth, fontAtlasHeight, strideInBytes, padding, allocContext);

	// Pack Latin1 font range
	const int firstChar = 32;
	const int charCount = 96;
	stbtt_pack_range packRange = {
		.font_size = pixelHeight,
		.first_unicode_codepoint_in_range = firstChar,
		.array_of_unicode_codepoints = nullptr,
		.num_chars = charCount,
		.chardata_for_range = ui.charData + firstChar,
	};
	const int res2 = stbtt_PackFontRanges(&packContext, fontData, fontIndex, &packRange, 1);

	// Pack custom white pixel
	stbrp_rect whitePixelRect = {
		.id = 0, // ignored by us
		.w = 1, .h = 1, // input
		// .x = 99, .y = 99, // output
		// .was_packed = false, // output
	};
	stbtt_PackFontRangesPackRects(&packContext, &whitePixelRect, 1);
	fontAtlasBitmap[strideInBytes * whitePixelRect.y + whitePixelRect.x] = 0xFF;
	ui.whitePixelUv = float2{ (whitePixelRect.x + 0.5f) / fontAtlasWidth, (whitePixelRect.y + 0.5f)/ fontAtlasHeight };

	// Pack RGBA icons
	for (u32 i = 0; i < iconCount; ++i)
	{
		UIIcon &icon = icons[i];

		// Pack custom white pixel
		stbrp_rect iconRect = {
			.id = 0, // ignored by us
			.w = icon.image.width, .h = icon.image.height, // input
			// .x = 99, .y = 99, // output
			// .was_packed = false, // output
		};
		stbtt_PackFontRangesPackRects(&packContext, &iconRect, 1);
		ASSERT(iconRect.was_packed);

		// Save packing info
		icon.pos = int2{ iconRect.x, iconRect.y };
		icon.uv = float2{ (f32)iconRect.x/fontAtlasWidth, (f32)iconRect.y/fontAtlasHeight };
		icon.uvSize = float2{ (f32)iconRect.w/fontAtlasWidth, (f32)iconRect.h/fontAtlasHeight };
	}

	ui.icons = icons;
	ui.iconCount = iconCount;

	// End packing
	stbtt_PackEnd(&packContext);

	// One channel to RGBA
	rgba *fontAtlasBitmapRGBA = PushArray(scratch.arena, rgba, fontAtlasWidth * fontAtlasHeight);

	byte *srcPtr = fontAtlasBitmap;
	rgba *dstPtr = fontAtlasBitmapRGBA;
	for (u32 i = 0; i < fontAtlasWidth * fontAtlasHeight; ++i)
	{
		*dstPtr++ = rgba{255, 255, 255, *srcPtr++};
	}

	// Blit icon RGBA pixels
	const u32 pixelSize = 4;
	for (u32 i = 0; i < iconCount; ++i)
	{
		const UIIcon &icon = icons[i];
		const u32 iconStride = icon.image.width * pixelSize;

		for (u32 y = 0; y < icon.image.height; ++y)
		{
			for (u32 x = 0; x < icon.image.width; ++x)
			{
				const rgba color = {
					icon.image.pixels[y * iconStride + x * pixelSize + 0],
					icon.image.pixels[y * iconStride + x * pixelSize + 1],
					icon.image.pixels[y * iconStride + x * pixelSize + 2],
					icon.image.pixels[y * iconStride + x * pixelSize + 3],
				};
				fontAtlasBitmapRGBA[strideInBytes * (icon.pos.y + y) + icon.pos.x + x] = color;
			}
		}
	}

	// Create texture
	ui.fontAtlasH = GfxCreateImage(gfx, "texture_font", fontAtlasWidth, fontAtlasHeight, 4, false, (byte*)fontAtlasBitmapRGBA);
	ui.fontAtlasSize = {fontAtlasWidth, fontAtlasHeight};

	UI_ResetWindowDefaults(ui);
}

void UI_BeginFrame(UI &ui)
{
	ui.frameIndex = ( ui.frameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;
	ui.vertexPtr = ui.frontendVertices;
	ui.vertexCount = 0;
	ui.vertexOverflow = false;
	ui.drawListCount = 0;
	ui.drawListStackSize = 0;

	if (UI_IsMousePressWithAnyButton(ui))
	{
		ui.input.lastMouseClickPos = UI_MousePos(ui);
	}

	// Forget which widget the previous click landed on. If the left button is
	// going down this frame, whichever widget is hovered (if any) will mark
	// itself as the pressed one again below, in UI_WidgetClicked.
	if (UI_IsMousePress(ui))
	{
		ui.widgetPressActive = false;
	}

	// Reposition windows based on its anchor
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		UI_PositionWindow(window, ui.viewportSize, window.size, window.anchor, window.displacement);
	}

	// Resize / move window interactions
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];

		if ( UI_IsMouseIdle(ui) )
		{
			window.dragging = false;
			window.resizing = false;
			window.disableWidgets = false;
		}

		// If no other widget interaction blocked the window...
		if ( !ui.avoidWindowInteraction )
		{
			if ( window.resizing )
			{
				const int2 resizeDelta = UI_MousePos(ui) - UI_LastMouseClickPos(ui);
				const int2 newWindowSize = window.sizeBeforeResize + resizeDelta;
				const float2 oldSize = window.size;
				window.size = Max(ui.style.minWindowSize, float2{(float)newWindowSize.x, (float)newWindowSize.y});
				window.displacement += window.pivot * (window.size - oldSize);
			}
			else if ( window.dragging )
			{
				const float2 oldPos = window.pos;
				window.pos += Float2(ui.input.mouse.delta);
				window.displacement += window.pos - oldPos;
			}
		}
	}

	// Update hovered window
	ui.hoveredWindow = nullptr;
	u32 hoveredWindowLayer = U32_MAX;
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];

		if ( window.visible && UI_MouseInArea(ui, window.pos, window.size) && window.layer < hoveredWindowLayer )
		{
			ui.hoveredWindow = &window;
			hoveredWindowLayer = window.layer;
		}
	}

	// Update active window
	if ( UI_IsMousePressWithAnyButton(ui) )
	{
		if ( ui.hoveredWindow && ui.hoveredWindow->visible )
		{
			if ( !( ui.hoveredWindow->flags & UIWindowFlag_NoRaise ) )
			{
				UI_RaiseWindow(ui, *ui.hoveredWindow);
				ui.activeWindow = ui.hoveredWindow;
				ui.activeWindow->dragging = ( ui.activeWindow->flags & UIWindowFlag_Draggable );
				ui.wantsInput = true;
			}
		}
		else
		{
			ui.activeWindow = nullptr;
			ui.wantsInput = false;
		}

		ui.activeWidgetId = 0;
	}

	// Deactivate menus on click outside
	if ( ui.activeMenu )
	{
		const float2 pos = ui.activeMenu->pos;
		const float2 size = ui.activeMenu->size;
		if ( UI_IsMousePress(ui) && !UI_MouseInArea(ui, pos, size)  )
		{
			ui.activeMenu = nullptr;
		}
	}

	// Deactivate menus on move mouse away
	if ( ui.activeMenu )
	{
		const float2 margin = {50, 50};
		const float2 extendedPos = ui.activeMenu->pos - margin;
		const float2 extendedSize = ui.activeMenu->size + 2.0f * margin;
		if ( !UI_MouseInArea(ui, extendedPos, extendedSize) )
		{
			ui.activeMenu = nullptr;
		}
	}

	// Reset visibility (to be determined again during this frame)
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		window.visible = false;
	}
}

void UI_EndFrame(UI &ui)
{
	// Draw drag and drop item
	if ( ui.dragAndDrop.payload.ptr )
	{
		const char *idString = "$draganddrop";
		const UIID windowId = UI_MakeID(ui, idString);
		UIWindow &window = UI_FindOrCreateWindow(ui, windowId, idString);
		window.size = float2{ 32, 32 };
		window.pos.x = UI_MousePos(ui).x - 0.5f * window.size.x;
		window.pos.y = UI_MousePos(ui).y - 0.5f * window.size.y;
		UI_RaiseWindow(ui, window);

		UI_BeginWindow(ui, windowId, UIWindowFlag_None);
		UI_Image(ui, ui.dragAndDrop.imageH, window.size, UIWidgetFlag_None, ui.dragAndDrop.uvRect);
		UI_EndWindow(ui);

		if ( UI_IsMouseIdle(ui) )
		{
			ui.dragAndDrop = {};
		}
	}

	// Modal window
	if ( ui.modalWindowStackSize )
	{
		bool backgroundNeeded = true;
		for (u32 i = 0; i < ui.modalWindowStackSize; ++i)
		{
			if ( ui.modalWindowStack[i]->modalFlags & UIModalFlag_NoBackground )
			{
				backgroundNeeded = false;
				break;
			}
		}

		const char *idString = "$modalbg";
		const UIID bgWindowId = UI_MakeID(ui, idString);

		if  ( backgroundNeeded )
		{
			const float4 overlay = ui.style.modalOverlayColor;
			UI_PushElemColor(ui, UIElementBackground, { overlay, overlay, overlay, overlay });
			UI_SetNextWindowSize(ui, ui.viewportSize);
			UI_SetNextWindowAnchorAndPivot(ui, float2{0, 0}, float2{0, 0});
			UI_BeginWindow(ui, idString, nullptr, UIWindowFlag_Background | UIWindowFlag_NoRaise );
			UI_EndWindow(ui);
			UI_PopElemColor(ui, UIElementBackground);
		}

		for (u32 i = 0; i < ui.modalWindowStackSize; ++i)
		{
			// Put background right below the topmost modal window
			if ( backgroundNeeded && i == ui.modalWindowStackSize-1 )
			{
				UIWindow &bgWindow = UI_FindWindow(ui, bgWindowId);
				UI_RaiseWindow(ui, bgWindow);
			}
			UI_RaiseWindow(ui, *ui.modalWindowStack[i]);
			UI_FocusWindow(ui, *ui.modalWindowStack[i]);
		}

		ui.modalWindowStackSize = 0;
	}

	// Create an array of draw list indices
	u32 sortedDrawListIndices[ARRAY_COUNT(ui.drawLists)];
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		sortedDrawListIndices[i] = i;
	}

	// Sort draw lists
	u32 numSwaps = U32_MAX;
	while (numSwaps > 0)
	{
		numSwaps = 0;
		for (u32 i = 1; i < ui.drawListCount; ++i)
		{
			const u32 index0 = sortedDrawListIndices[i-1];
			const u32 index1 = sortedDrawListIndices[i];
			const u32 layer0 = ui.drawLists[index0].sortKey.layer;
			const u32 layer1 = ui.drawLists[index1].sortKey.layer;
			const u32 order0 = ui.drawLists[index0].sortKey.order;
			const u32 order1 = ui.drawLists[index1].sortKey.order;
			if ( layer0 < layer1 || ( layer0 == layer1 &&  order0 > order1 ) )
			{
				sortedDrawListIndices[i-1] = index1;
				sortedDrawListIndices[i] = index0;
				numSwaps++;
			}
		}
	}

	// Copy the sorted draw lists to tmp array and back
	UIDrawList sortedDrawLists[ARRAY_COUNT(ui.drawLists)];
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		const u32 sortedDrawListIndex = sortedDrawListIndices[i];
		sortedDrawLists[i] = ui.drawLists[sortedDrawListIndex];
	}
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		ui.drawLists[i] = sortedDrawLists[i];
	}

	if ( UI_IsMouseIdle(ui) )
	{
		ui.avoidWindowInteraction = false;
	}

	if ( ui.vertexOverflow )
	{
		LOG(Warning, "UI vertexCount limit (%u) reached!\n", ui.vertexCountLimit);
	}
}

void UI_Cleanup(const UI &ui)
{
	// Nothing to do as gfx resources are cleaned up on gfx lib shutdown
}

#endif // #ifndef ILU_UI_H

# daRg Input Processing Architecture

## Overview

daRg (Dagor Reactive GUI) processes input from five device types through a unified
behavior dispatch model. The host application receives raw
HID events and forwards them into the `IGuiScene` interface.
`GuiScene` translates these into behavior callbacks, hover updates,
hotkey triggers, and navigation actions.

All input processing runs on the main (API) thread, enforced by `ApiThreadCheck`.
The one exception is `JoystickHandler::onJoystickStateChanged()`, which may be called
from a HID polling thread and queues button deltas for main-thread dispatch.

```
Host App (daguiManager.cpp)
  |
  |  HID driver callbacks
  v
IGuiScene interface (dag_guiScene.h)
  |  onMouseEvent, onTouchEvent, onKbdEvent,
  |  onJoystickBtnEvent, onVrInputEvent,
  |  setVrStickScroll, updateSpatialElements
  v
GuiScene (guiScene.cpp)
  |
  +---> Input Stack dispatch (behaviors)
  +---> Hotkey matching
  +---> Hover update (ElementTree::updateHover)
  +---> XMB / DirPad navigation
  +---> Pointer position management (MousePointerPos, GamepadCursor)
  +---> Active device tracking (activePointingDevice)
  +---> VR spatial interaction (panels)
```

---

## 1. Device Types and Input Events

### InputDevice enum (dag_inputIds.h)

| Value | Name | Source |
|-------|------|--------|
| 1 | `DEVID_KEYBOARD` | Physical keyboard |
| 2 | `DEVID_MOUSE` | Mouse or mouse-emulated |
| 3 | `DEVID_JOYSTICK` | Gamepad buttons and stick-as-cursor |
| 4 | `DEVID_TOUCH` | Touchscreen |
| 5 | `DEVID_VR` | VR controller aim rays |

Currently VR finger touch generates DEVID_TOUCH, but this will be changed to DEVID_VR.

While there is a legacy code and code written with wrong understanding of the design and
there are confusion in some places between which physical device produces events with which
InputDevice, all the pointing devices (mouse, touchscreen and VR controllers) are expected
to be unified, so that each of devices contains M number of pointers with each of these
pointers containing N number of buttons. Behaviors are mostly prepared for that and are
written with this in mind, however in daRg core it is not always like this.


### InputEvent enum (dag_inputIds.h)

| Value | Name | Used by |
|-------|------|---------|
| 1 | `INP_EV_PRESS` | All pointing/button devices |
| 2 | `INP_EV_RELEASE` | All pointing/button devices |
| 3 | `INP_EV_POINTER_MOVE` | Mouse, touch, VR aim |
| 4 | `INP_EV_MOUSE_WHEEL` | Mouse wheel |

---

## 2. The Unified Behavior Model

### Behavior class (dag_behavior.h)

Every UI element can have zero or more `Behavior` instances. Behaviors declare their
capabilities via flag bitmasks and receive input through three virtual methods:

```cpp
// Pointing devices: mouse, touch, VR, gamepad-as-mouse
int pointingEvent(ElementTree*, Element*, InputDevice device, InputEvent event,
                  int pointer_id, int btn_id, Point2 pos, int accum_res);

// Keyboard input (only delivered to kb-focused element, or global handlers)
int kbdEvent(ElementTree*, Element*, InputEvent event, int key_idx,
             bool repeat, wchar_t wc, int accum_res);

// Joystick buttons (raw gamepad events, separate from pointing)
int joystickBtnEvent(ElementTree*, Element*, const IGenJoystick*,
                     InputEvent event, int key_idx, int device_number,
                     const ButtonBits& buttons, int accum_res);
```

### Key Behavior Flags

| Flag | Hex | Meaning |
|------|-----|---------|
| `F_HANDLE_KEYBOARD` | 0x0001 | Receives `kbdEvent` when kb-focused |
| `F_HANDLE_KEYBOARD_GLOBAL` | 0x10000001 | Receives `kbdEvent` for all keys (not just when focused) |
| `F_HANDLE_MOUSE` | 0x0002 | Receives `pointingEvent` for mouse/VR/gamepad-as-mouse |
| `F_HANDLE_TOUCH` | 0x0004 | Receives `pointingEvent` for touch |
| `F_HANDLE_JOYSTICK` | 0x0008 | Receives `joystickBtnEvent` |
| `F_CAN_HANDLE_CLICKS` | 0x0010 | Used for cursor-over-clickable detection |
| `F_INTERNALLY_HANDLE_GAMEPAD_L_STICK` | 0x0020 | Suppresses gamepad cursor on L stick |
| `F_INTERNALLY_HANDLE_GAMEPAD_R_STICK` | 0x0040 | Suppresses gamepad cursor on R stick |
| `F_FOCUS_ON_CLICK` | 0x0200 | Gets keyboard focus on click release |
| `F_ALLOW_AUTO_SCROLL` | 0x0800 | Eligible for auto-scroll in XMB navigation |

### BehaviorResult flags (dag_guiConstants.h)

All event handlers return a bitmask of `BehaviorResult`:

| Flag | Value | Meaning |
|------|-------|---------|
| `R_PROCESSED` | 0x01 | Event was consumed. This is used to communicate between behaviors. |
| `R_STOPPED` | 0x02 | Stop propagation to further elements. Generally not needed, should be used in rare cases. |
| `R_REBUILD_RENDER_AND_INPUT_LISTS` | 0x04 | Element tree changed, rebuild stacks |

### The `pointingEvent` Unification

All pointing devices share the same `pointingEvent` signature. The `device` parameter
distinguishes the source, `pointer_id` identifies which pointer (finger index, VR hand,
always 0 for mouse), and `btn_id` carries button index or wheel delta:

| Device | device | pointer_id | btn_id |
|--------|--------|-----------|--------|
| Mouse click | DEVID_MOUSE | 0 | mouse button index |
| Mouse move | DEVID_MOUSE | 0 | 0 |
| Mouse wheel | DEVID_MOUSE | 0 | scroll delta |
| Gamepad cursor | DEVID_JOYSTICK | 0 | 0 |
| Touch | DEVID_TOUCH | touch_idx | 0 |
| VR aim | DEVID_VR | hand (0/1/2) | button_id |

---

## 3. Screens, Input Stacks, and Element Trees

### Screen

A `Screen` is a self-contained UI surface with its own element tree and stacks:

```cpp
class Screen {
  ElementTree etree;        // element hierarchy
  InputStack inputStack;    // interactive elements sorted by z-order
  InputStack cursorStack;   // cursor-eligible elements
  InputStack eventHandlersStack;
  Panel *panel = nullptr;   // non-null for VR panel screens
  bool isMainScreen;        // true for the primary screen (id=0)
};
```

`GuiScene` manages multiple screens via `eastl::vector_map<int, unique_ptr<Screen>>`.
One screen is the `focusedScreen` which receives most input. The main screen (id 0)
gets mouse hover processing; panel screens get VR pointer hover processing.

### InputStack

The `InputStack` is a z-ordered set of interactive elements, rebuilt when the element
tree changes:

```cpp
struct InputEntry {
  Element *elem;
  int zOrder;        // higher = closer to user
  int hierOrder;     // tree traversal order for same z
  bool inputPassive; // doesn't consume input
};

class InputStack {
  Stack stack;                // sorted: higher z-order first, then higher hier-order
  int summaryBhvFlags = 0;   // OR of all behaviors' flags in the stack
  bool isDirPadNavigable;    // any element supports dirpad navigation
};
```

Events are dispatched by iterating the stack top-to-bottom. Elements with
`F_STOP_MOUSE` flag block further propagation when hit-tested.

### Coordinate Spaces

For the main screen, input coordinates are in screen pixels. For VR panel screens,
coordinates are in panel-local pixels. `Screen::displayToLocal()` and
`Screen::localToDisplay()` convert between screen space and panel space using
camera transform and frustum projection.

---

## 4. Mouse Input Pipeline

### Entry Point: `GuiScene::onMouseEvent()`

Called by the host when the HID mouse driver fires a move, click, or wheel event.

```
Host gmcMouseMove/gmcMouseBtnDown/gmcMouseBtnUp/gmcMouseWheel
  |
  v
GuiScene::onMouseEvent(event, data, mx, my, buttons)
  |
  +-- POINTER_MOVE:
  |     skip if relative movement mode
  |     if activePointingDevice != DEVID_MOUSE:
  |       warp OS cursor to gamepadCursor.pos (no visual jump)
  |       switch to DEVID_MOUSE, discard stale event, return
  |     clear XMB focus (unless programmatic move)
  |     update mousePointerPos.pos via onMouseMoveEvent()
  |     updateGlobalHover()
  |
  +-- For each screen:
  |     displayToLocal(mx, my) -> local pos
  |     onMouseEventInternal(screen, event, DEVID_MOUSE, data, local, buttons)
  |
  +-- callScriptHandlers()
  +-- rebuildInvalidatedParts() if R_PROCESSED
```

### Internal Dispatch: `GuiScene::onMouseEventInternal()`

Handles three event categories differently:

**PRESS / RELEASE:**
1. `updateHotkeys()` — check if any hotkey combo triggered
2. `handleMouseClick()` — iterate input stack, dispatch `pointingEvent` to
   behaviors with `F_HANDLE_MOUSE`, manage keyboard focus via `F_FOCUS_ON_CLICK`

**MOUSE_WHEEL:**
- Iterate input stack but only pass wheel to elements that are ancestors of
  the first hit element (allows scroll containers behind mouse-blocking children)

**POINTER_MOVE:**
- Iterate input stack, dispatch `pointingEvent` to all `F_HANDLE_MOUSE` behaviors
- Gamepad cursor movement dispatches `POINTER_MOVE` with `DEVID_JOYSTICK` directly
  from `updateGamepadCursor()`, bypassing the HID mouse driver entirely

---

## 5. Pointer Position Management

### Dual Pointer Model

Mouse and gamepad each own independent pointer positions. A single
`activePointingDevice` (member of `GuiScene`) tracks which is live.
`activePointerPos()` returns the position of the active pointer — this is where
the visible cursor renders and where click buttons fire.

On device switch, the newly active pointer warps to the previous one's position
so the cursor doesn't visually jump:
- **Mouse POINTER_MOVE while gamepad is active**: warp OS cursor to
  `gamepadCursor.pos`, switch to `DEVID_MOUSE`, discard the stale event.
- **Gamepad stick active or button press while mouse is active**:
  `gamepadCursor.syncPosFrom(mousePointerPos)`, switch to `DEVID_JOYSTICK`.

### CursorPosState base struct (cursorState.h)

Shared position state inherited by both `MousePointer` and `GamepadCursor`:

```
pos               Current position (in screen pixels)
targetPos         Animation target (for smooth cursor movement)
nextFramePos      Deferred jump target (applied next frame)  [protected]
needToMoveOnNextFrame  Flag: nextFramePos is pending         [protected]
```

Shared methods:
- `requestNextFramePos(p)` — queue instant jump for next frame
- `requestTargetPos(p)` — set smooth animation target
- `applyNextFramePos()` — apply deferred jump: pos = nextFramePos, sync targetPos
- `hasNextFramePos()` — check if a deferred jump is pending
- `syncPosFrom(other)` — copy pos from another pointer (for device switching)
- `debugRenderState(ctx)` — render nextFramePos/targetPos debug markers

Each device subclass provides its own `moveToTarget(dt)` for smooth animation.
`MousePointer` also provides `commitToDriver()` for one-way OS cursor sync,
called once per frame after all internal state updates.

`GuiScene::activePointerState()` returns a `CursorPosState&` to the active pointer,
used by `requestActivePointerNextFramePos()` and `requestActivePointerTargetPos()`.

### MousePointer class (mousePointer.h/.cpp)

Mouse-specific state (beyond CursorPosState):

```
isSettingMousePos           Recursion guard for HID driver feedback
needsDriverSync             Dirty flag: pos changed internally, needs OS sync
mouseWasRelativeOnLastUpdate  FPS relative-mode tracking
```

### GamepadCursor class (gamepadCursor.h/.cpp)

Gamepad-specific state (beyond CursorPosState):

```
currentSector, timeInCurrentSector   Stick sector tracking for friction
dxCup, dyCup                         Sub-pixel accumulation
config                               Reference to SceneConfig
```

### Position Update Flow

```
User mouse move -> MousePointerPos::onMouseMoveEvent(p)
  sets pos = p, targetPos = p

Gamepad stick -> GamepadCursor::updateCursorPos()
  writes directly to gamepadCursor.pos (no HID driver)
  dispatches POINTER_MOVE with DEVID_JOYSTICK directly to behaviors

XMB navigation -> moveMouseCursorToElemBox()
  calls requestActivePointerNextFramePos() (jump)
  or requestActivePointerTargetPos() (animate)
```

### HID Driver Synchronization (mouse only)

`MousePointerPos::setMousePos()` writes to the HID driver via
`mouse->setPosition()`, then reads back the actual position. This read-back handles
driver clamping. The `isSettingMousePos` guard prevents the driver callback from
recursing:

```
setMousePos(p) {
  pos = floor(p);
  isSettingMousePos = true;
  mouse->setPosition(pos);     // may trigger onMouseMoveEvent callback
  isSettingMousePos = false;
  pos = mouse->getRawState();  // read back clamped value
}
```

`isProgrammaticMove()` exposes this guard to callers (e.g. to avoid clearing XMB focus
when the move was initiated by code, not by the user).

### Relative Movement Mode

When the mouse is in relative mode (FPS camera), pointer position updates are
suppressed. `syncMouseCursorLocation()` detects the transition from relative back to
absolute mode and re-syncs the HID driver position.

---

## 6. Gamepad Cursor (gamepadCursor.h/.cpp)

Converts gamepad analog stick input into cursor movement. Owns its own pointer
position (`pos`, `targetPos`) and animation state. The cursor position is updated
in-place — no HID mouse driver interaction.

### Device Switching

`GuiScene::updateGamepadCursor()` uses `isStickActive()` (lightweight deadzone check
without state mutation) to detect stick activity. If the stick is active and
`activePointingDevice != DEVID_JOYSTICK`, it calls `gamepadCursor.syncPosFrom(mousePointerPos)`
and switches the active device. Then `updateCursorPos()` computes
the new position. If the cursor moved, `POINTER_MOVE` events are dispatched directly
to all screens with `DEVID_JOYSTICK` — bypassing the HID mouse driver entirely.

Gamepad button press in `onJoystickBtnEvent()` also triggers a switch to
`DEVID_JOYSTICK`.

### Computation Pipeline

```
updateCursorPos(joy, screen, cameraTm, cameraFrustum, dt)
  |
  +-- Early exits: no joystick, relative mouse mode, stick claimed by behavior
  |
  +-- 4 iterations of moveOverElements() for sub-frame accuracy:
        |
        +-- Read stick axes, apply deadzone + nonlinearity
        +-- Calculate sector (8-way) for friction timing
        +-- Field effect: scan input stack for F_STICK_CURSOR elements
        |   compute proximity -> fieldEffectFactor (0..1)
        +-- Friction: slow cursor near interactive elements
        |   based on time-in-sector and fieldEffectFactor
        +-- Compute dx, dy with speed * friction * screen scaling
        +-- Sub-pixel accumulation (dxCup, dyCup)
        +-- Clamp to screen bounds
  |
  +-- If pos changed: targetPos = pos, return true
```

### Stick Ownership

If a behavior on the focused screen has `F_INTERNALLY_HANDLE_GAMEPAD_L_STICK` (or R),
the corresponding stick is not used for cursor movement. This allows scroll behaviors
to consume stick input directly.

### Scroll

`GamepadCursor::scroll()` reads the scroll stick (default: right thumb) and returns
a scroll delta, which `GuiScene` applies via `doJoystickScroll()`.

---

## 7. Hover System

### updateGlobalHover()

Recalculates which elements are hovered by all active pointers. Called from:

| Trigger | Location |
|---------|----------|
| update() pipeline | Once per frame via dirty flag (consolidates gamepad, animation, deferred pos) |
| onMouseEvent (MOVE) | Immediate — user moved mouse |
| onTouchEvent | Immediate — touch point moved |
| doJoystickScroll | After scroll changed element positions |
| rebuildInvalidatedParts | After element tree rebuild |
| applyInteractivityChange | After interactive flags changed |
| process(GPCM_Activate) | App regained window focus |
| applyPendingHoverUpdate | Deferred: waits for one render frame |

### How it works

```
updateGlobalHover() {
  pointers = [activePointerPos()] + touchPointers.activePointers
  focusedScreen->etree.updateHover(inputStack, pointers, stickScrollFlags)
  // updates: S_HOVER, S_TOP_HOVER state flags on elements
  // calls: elem->callHoverHandler(true/false) on transitions
  // updates: cursorOverStickScroll, cursorOverClickable observables
  validateOverlaidXmbFocus()
}
```

`ElementTree::updateHover()` iterates the input stack for each pointer, hit-testing
elements. `F_STOP_HOVER` and `F_STOP_MOUSE` block hover propagation.

### update() Pipeline Hover Consolidation

The `update()` method consolidates up to 3 hover update sources into a single call:

```cpp
bool hoverDirty = false;

// --- Phase 1: Apply deferred jumps (internal state only) ---
// Mouse: guarded by dgs_app_active (deferred until app is active)
if (activePointingDevice == DEVID_MOUSE && ::dgs_app_active)
{
  mousePointer.syncMouseCursorLocation();
  bool mouseJumped = mousePointer.hasNextFramePos();
  mousePointer.applyNextFramePos();
  if (mouseJumped) { mousePointer.markNeedsDriverSync(); hoverDirty = true; }
}
if (activePointingDevice == DEVID_JOYSTICK)
{
  bool gpJumped = gamepadCursor.hasNextFramePos();
  gamepadCursor.applyNextFramePos();
  if (gpJumped) hoverDirty = true;
}

// --- Phase 2: Device-specific movement ---
updateGamepadCursor(dt, hoverDirty);  // stick-driven pos update
if (activePointingDevice == DEVID_MOUSE && mousePointer.moveToTarget(dt))
  hoverDirty = true;
if (activePointingDevice == DEVID_JOYSTICK && gamepadCursor.moveToTarget(dt))
  hoverDirty = true;

// --- Phase 3: Sync mouse to OS driver (one-way out) ---
if (activePointingDevice == DEVID_MOUSE)
  mousePointer.commitToDriver();

if (hoverDirty) updateGlobalHover();  // ONCE per frame, after OS sync
```

### Deferred Hover Update

`requestHoverUpdate()` sets state to `WaitingForRender`. After one render pass
(`renderThreadBeforeRender` transitions it to `NeedToUpdate`), the next `update()`
calls `applyPendingHoverUpdate()` which fires `updateGlobalHover()`. This ensures
layout has been applied before recalculating hovers.

---

## 8. Keyboard Input

### Entry Point: `GuiScene::onKbdEvent()`

```
onKbdEvent(event, key_idx, shifts, repeat, wc)
  |
  +-- Click button release: if key was a "click button", fire handleMouseClick RELEASE
  |
  +-- Focused element: dispatch kbdEvent to kbFocus.focus behaviors (F_HANDLE_KEYBOARD)
  |
  +-- Global handlers: dispatch to all F_HANDLE_KEYBOARD_GLOBAL behaviors
  |
  +-- Hotkeys: if not yet processed, updateHotkeys()
  |
  +-- Click button press: if unprocessed and key is a click button,
  |   fire handleMouseClick PRESS at mouse position
  |
  +-- DirPad navigation: if kbCursorControl and arrow keys, dirPadNavigate()
```

### Keyboard Focus (KbFocus)

A single element can hold keyboard focus (`kbFocus.focus`). Focus can be captured
(`kbFocus.captureFocus`) to prevent click-outside from removing it. Focus is set on
click release for elements with `F_FOCUS_ON_CLICK`, and cleared when clicking outside.

### Service Keyboard Events

`onServiceKbdEvent()` is called before `onKbdEvent()` for system-level shortcuts
(e.g. Ctrl+Backspace to dismiss error overlay). It does not check `isInputActive()`.

---

## 9. Joystick Button Input

### Entry Point: `GuiScene::onJoystickBtnEvent()`

```
onJoystickBtnEvent(joy, event, btn_idx, device_number, buttons)
  |
  +-- Device switch: on PRESS, if activePointingDevice != DEVID_JOYSTICK,
  |   gamepadCursor.syncPosFrom(mousePointerPos), switch device
  |
  +-- DirPad release: clear repeat state
  |
  +-- Click button release: fire handleMouseClick RELEASE (DEVID_JOYSTICK)
  |
  +-- Behavior dispatch: iterate focusedScreen input stack,
  |   dispatch joystickBtnEvent to F_HANDLE_JOYSTICK behaviors
  |
  +-- Hotkeys: if unprocessed, updateHotkeys()
  |
  +-- Click button press: fire handleMouseClick PRESS (DEVID_JOYSTICK)
  |
  +-- DirPad navigation: if press + unprocessed, dirPadNavigate()
  |   Start repeat timer (dirPadRepeatDelay -> dirPadRepeatTime)
```

### Thread-Safe Joystick Handler (dag_joystick.h)

`JoystickHandler` bridges the HID polling thread and the main thread:

```
HID thread: onJoystickStateChanged(scenes, joy, ord_id)
  -> compares buttons to previous state
  -> if on main thread: dispatches immediately
  -> if off main thread: pushes ButtonBits to btnStack queue

Main thread (each update): processPendingBtnStack(scenes)
  -> pops queued ButtonBits, diffs them, calls onJoystickBtnEvent for changes
```

### Click Buttons

Both keyboard and joystick can have "click buttons" (configured in `SceneConfig::clickButtons`).
These map physical buttons to synthetic mouse clicks at the current pointer position.
`pressedClickButtons` tracks which are currently held to ensure proper release dispatch.

---

## 10. Touch Input

### Entry Point: `GuiScene::onTouchEvent()`

```
onTouchEvent(event, pointing_device, touch_idx, touch)
  |
  +-- Iterate focused screen input stack
  |   dispatch pointingEvent(DEVID_TOUCH, event, touch_idx, 0, pos)
  |   to F_HANDLE_TOUCH behaviors
  |
  +-- On POINTER_MOVE: clear XMB focus
  +-- touchPointers.updateState(event, touch_idx, pos)
  +-- updateGlobalHover()
```

`TouchPointers` tracks active touch points as a vector of `{id, pos}` pairs.
Touch events are always dispatched to the focused screen only.

---

## 11. VR Input

VR input uses a spatial interaction model with raycasting and finger-touch detection.

### Host-Side Setup (daguiManager.cpp)

Each frame, the host constructs `SpatialSceneData` containing:
- Camera transform and frustum in play space
- Hand aim rays (LeftHand, RightHand) from VR controller poses
- Generic aim ray (from mouse screen-to-world projection when not in VR)
- Index fingertip transforms for touch detection
- VR surface intersector callback

This is passed to `gui_scene->updateSpatialElements(spatialScene)`.

### Panel System

VR panels are world-space UI surfaces defined from script:

```cpp
struct PanelSpatialInfo {
  PanelAnchor anchor;       // Scene, VRSpace, Head, LeftHand, RightHand, Entity
  PanelGeometry geometry;   // Rectangle
  bool canBePointedAt;      // Ray intersection
  bool canBeTouched;        // Finger proximity
  Point3 position, angles, size;
};

struct PanelPointer {
  bool isPresent;
  Point2 pos;               // Panel-local pixel coordinates
  Cursor *cursor;
};

class Panel {
  PanelPointer pointers[3]; // LeftHand, RightHand, Generic
  TouchPointers touchPointers;
  Screen *screen;           // Panel's own Screen with element tree
};
```

### Spatial Interaction State

`updateSpatialInteractionState()` performs raycasting/proximity checks:

1. For each aim source (LeftHand, RightHand, Generic):
   - Check VR surface intersection (if configured)
   - Check each panel: cast ray or project fingertip
   - Track closest hit distance and panel index
2. Store results in `spatialInteractionState`:
   - `hitDistances[hand]` — distance to closest hit
   - `closestHitPanelIdxs[hand]` — which panel was hit (-1 = VR surface)
   - `hitPos[hand]` — UV or pixel coordinates on hit surface
   - `isHandTouchingPanel[hand]` — finger touch active

### VR Event Dispatch: `GuiScene::onVrInputEvent()`

Called per-hand for press/release/move events:

```
onVrInputEvent(event, hand, button_id)
  |
  +-- Track active hand (vrActiveHand = hand on press)
  |
  +-- Panel transition: if hand moved to different panel,
  |   release old panel's touch, deactivate input, switch activePanel
  |
  +-- VR surface hit on active hand:
  |   dispatch onMouseEventInternal(DEVID_VR) to panel's screen
  |
  +-- Panel touch (canBeTouched):
  |   onPanelTouchEvent(panel, hand, event, pos) -> pointingEvent(DEVID_TOUCH)
  |
  +-- Panel aim (canBePointedAt):
  |   onPanelMouseEvent(panel, hand, event, DEVID_VR) -> pointingEvent(DEVID_VR)
```

### VR Stick Scroll

The host calls `setVrStickScroll(hand, scroll)` each frame. During `update()`,
`updatVrStickScroll()` applies scroll deltas to the panel or main screen under each hand.

### Notes

Currently VR input works in two modes:
* Controller aim generates DEVID_VR events
* In-world finger generates DEVID_TOUCH events.
This should be unified in future - both shoud send DEVID_VR, and share the same input pointer, but use different button ids for clicks/touches.

---

## 12. Hotkeys

### HotkeyCombo

Elements can declare hotkey combinations in their script description. A `HotkeyCombo`
contains a set of `HotkeyButton` (device + button pairs) and fires when all buttons
in the combo are simultaneously pressed.

```cpp
struct HotkeyButton {
  InputDevice devId;   // DEVID_KEYBOARD, DEVID_JOYSTICK, DEVID_MOUSE
  unsigned int btnId;  // key/button index
  bool ckNot;          // negation (button must NOT be pressed)
};
```

### Hotkey Processing: `updateHotkeys()`

Called from `onMouseEventInternal` (PRESS/RELEASE), `onKbdEvent`, `onJoystickBtnEvent`,
and `process(GPCM_Activate)`.

1. Iterate all screens' input stacks
2. For each element's hotkey combos, check if the combo is active
3. Collect matching combos; if multiple match, prefer the one with more buttons
4. Clear `S_HOTKEY_ACTIVE` state on elements whose combos are subsets of the winner
5. Fire the winning combo's handler

Elements with `F_STOP_HOTKEYS` block hotkey processing for elements below them.

### Event-Based Hotkeys

`sendEvent(id, data)` allows triggering hotkeys by named event strings (e.g. from
dainput action system). Events with `:end` suffix map to release.

---

## 13. DirPad and XMB Navigation

### DirPad Navigation

Gamepad D-pad (and optionally arrow keys / stick-as-dirpad) navigates between
interactive elements spatially:

```
dirPadNavigate(dir)
  |
  +-- If XMB mode active: xmbNavigate(dir) — hierarchical navigation
  |
  +-- Otherwise: spatial search
  |   For each navigable element in input stack:
  |     calc_dir_nav_distance(pointerPos, element_bbox, dir)
  |   Move cursor to closest element in the given direction
  |   Set or clear XMB focus
```

Repeat is handled by `repeatDirPadNav(dt)` with configurable delay/interval.

### XMB Navigation

XMB (Cross-Media Bar) is a hierarchical navigation system where elements are organized
into parent-child groups:

```
XmbData {
  Element *xmbParent;
  vector<Element*> xmbChildren;
  Table nodeDesc;  // script-defined params (scrollSpeed, scrollToEdge, onLeave)
}
```

`xmbNavigate()` uses either grid-based or screen-space navigation depending on the
parent's layout flow. When leaving a group, the `onLeave` script handler can return
`XMB_STOP` to prevent exit.

When XMB focus changes, the cursor is moved to the focused element via
`moveMouseCursorToElemBox()`, which uses smooth animation (`requestTargetPos`) or
instant jump (`requestNextFramePos`). Auto-scrolling brings the focused element into
view.

---

## 14. Input Activity Control

### Two Independent Gates

```cpp
bool isInputActive() const { return isInputEnabledByHost && isInputEnabledByScript; }
```

- `isInputEnabledByHost` — set by the host via `setSceneInputActive()`. Used to
  globally disable daRg input when the game needs exclusive control.
- `isInputEnabledByScript` — set from Quirrel script. Allows scripts to temporarily
  suppress input.

When input becomes inactive, `applyInputActivityChange()` deactivates all input on
all screens (`deactivateAllInput()`) and clears pressed click buttons.

### `::dgs_app_active`

Separate from `isInputActive()`. This is the OS window focus state. Checked in:
- `updateGamepadCursor()` — don't move cursor when window is unfocused
- `MousePointerPos::setMousePos()` — don't write to HID driver when unfocused
- `MousePointerPos::update()` — don't sync cursor or apply deferred moves
- `onJoystickBtnEvent()` — guard hotkey/click/navigation processing

---

## 15. Element Flags Relevant to Input

| Flag | Hex | Effect |
|------|-----|--------|
| `F_STOP_MOUSE` | 0x0080 | Blocks mouse/pointing event propagation (and hover) when hit |
| `F_STOP_HOVER` | 0x0100 | Blocks hover propagation (but not events) when hit |
| `F_STOP_HOTKEYS` | 0x0200 | Blocks hotkey processing for elements below |
| `F_STICK_CURSOR` | 0x0400 | Gamepad cursor "sticks" near this element (field effect friction) |
| `F_JOYSTICK_SCROLL` | 0x0800 | Element accepts joystick/VR stick scroll |
| `F_SKIP_DIRPAD_NAV` | 0x1000 | Element is skipped during dirpad navigation |

---

## 16. The update() Frame Pipeline

The main per-frame update, showing all input-related operations in order:

```
GuiScene::update(dt)
  |
  1. applyPendingHoverUpdate()    // deferred hover from previous frame
  2. applyDeferredRecalLayout()   // deferred layout recalculation
  3. frpGraph->updateDeferred()   // FRP observable updates
  4. processPostedEvents()        // events posted from other threads
  5. joystickHandler.processPendingBtnStack()  // queued joystick button changes
  6. repeatDirPadNav(dt)          // D-pad repeat navigation
  |
  7. bool hoverDirty = false
  --- Phase 1: Apply deferred jumps (internal state only) ---
  8. if (mouse active && dgs_app_active):
       syncMouseCursorLocation()
       applyNextFramePos(), markNeedsDriverSync if jumped
  8b. if (gamepad active):
       gamepadCursor.applyNextFramePos()
  9. update cursorPresent observable (accounts for activePointingDevice)
  --- Phase 2: Device-specific movement ---
  10. updateGamepadCursor(dt, hoverDirty)  // stick -> cursor, direct POINTER_MOVE dispatch
  11. updatVrStickScroll(dt)       // VR stick scroll
  12. updateJoystickAxesObservables()
  13. smooth animation for active device only:
      if (mouse active && moveToTarget(dt)) hoverDirty = true
      if (gamepad active && moveToTarget(dt)) hoverDirty = true
  --- Phase 3: Sync mouse to OS driver (one-way out) ---
  13b. if (mouse active) mousePointer.commitToDriver()
  14. if (hoverDirty) updateGlobalHover()  // ONCE per frame, after OS sync
  |
  15. updateTimers(dt)
  16. callScriptHandlers()
  17. rebuildInvalidatedParts()
  18. sceneScriptHandlers.onUpdate(dt)
  19. per-screen etree.update(dt) -> behavior updates, stack rebuilds
  20. applyInteractivityChange() if stacks changed
  21. cursor update, panel updates
```

---

## 17. Key File Map

| File | Role |
|------|------|
| `publicInclude/daRg/dag_guiScene.h` | Public `IGuiScene` interface |
| `publicInclude/daRg/dag_behavior.h` | `Behavior` base class |
| `publicInclude/daRg/dag_inputIds.h` | `InputDevice`, `InputEvent` enums |
| `publicInclude/daRg/dag_guiConstants.h` | `BehaviorResult`, `Direction`, layout enums |
| `publicInclude/daRg/dag_joystick.h` | `JoystickHandler` (thread-safe button queue) |
| `publicInclude/daRg/dag_element.h` | `Element` class with flags and behaviors |
| `daRg/guiScene.h` | `GuiScene` implementation class |
| `daRg/guiScene.cpp` | Main orchestration (~4400 lines) |
| `daRg/pointerPosition.h/.cpp` | `MousePointerPos`: mouse position tracking and animation |
| `daRg/gamepadCursor.h/.cpp` | `GamepadCursor`: gamepad pointer position, stick computation, field effects |
| `daRg/screen.h` | `Screen` (element tree + stacks container) |
| `daRg/panel.h` | VR `Panel` and `PanelData` |
| `daRg/inputStack.h` | `InputStack` (z-sorted interactive elements) |
| `daRg/hotkeys.h` | `Hotkeys` and `HotkeyCombo` |
| `daRg/touchPointers.h` | `TouchPointers` active touch tracking |
| `daRg/kbFocus.h` | `KbFocus` keyboard focus management |
| `daRg/sceneConfig.h` | `SceneConfig` (gamepad, dirpad, click config) |
| `daRg/elementTree.h/.cpp` | `ElementTree::updateHover()`, `doJoystickScroll()` |
| `daRg/xmb.h/.cpp` | XMB hierarchical navigation algorithms |
| `daRg/dirPadNav.h/.cpp` | Spatial D-pad navigation |

---

## 18. Known Architectural Issues and Future Directions

### Dual Pointer Model (Resolved)

Mouse and gamepad now have independent pointer positions. `activePointingDevice`
tracks which is live, and `activePointerPos()` returns the visible cursor position.
Device switching warps the newly active pointer to the previous one's position.
The gamepad cursor dispatches `POINTER_MOVE` with `DEVID_JOYSTICK` directly —
no HID mouse driver interaction, no `isMovingMouse` flag, no Xbox synthetic events.

Remaining follow-up: when the gamepad is active, the relative mouse mode check in
`isStickActive()` / `updateCursorPos()` could be relaxed — the gamepad pointer
doesn't need the mouse in absolute mode since it doesn't interact with the HID driver.

### Duplicated Event Dispatch Loops

The "iterate input stack, dispatch to behaviors" loop appears 5-6 times across
`onMouseEventInternal` (PRESS/RELEASE, WHEEL, MOVE paths), `onTouchEvent`,
`onPanelMouseEvent`, `onPanelTouchEvent`. The core pattern is identical:

```cpp
for (const InputEntry &ie : screen->inputStack.stack) {
  if (ie.elem->hasBehaviors(FLAG)) {
    for (Behavior *bhv : ie.elem->behaviors)
      if (bhv->flags & FLAG)
        summaryRes |= bhv->pointingEvent(...);
  }
  if (ie.elem->hasFlags(F_STOP_MOUSE) && ie.elem->hitTest(pos))
    summaryRes |= R_PROCESSED | R_STOPPED;
}
```

Variations: behavior flag (`F_HANDLE_MOUSE` vs `F_HANDLE_TOUCH`), parameters, and
interleaved special logic (wheel ancestry filtering, focus management). A unified
dispatch helper could reduce this significantly.

### Touch-Screen Emulation

`emuTouchScreenWithMouse` is a HID-level flag that reroutes mouse events to touch.
When active, `mousePointerPos.pos` diverges from actual mouse position. The
`updateGlobalActiveCursor()` has a workaround that reads the HID driver directly.
This is explicitly out of scope for current refactoring.

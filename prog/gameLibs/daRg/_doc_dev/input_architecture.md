# daRg Input Processing Architecture

An overview: it leaves out details and exceptions, and the code is the
reference.

## Overview

The host gets raw HID events and gives them to the `IGuiScene` entry points:
`onMouseEvent`, `onTouchEvent`, `onKbdEvent` (after `onServiceKbdEvent`),
`onJoystickBtnEvent`, `onVrInputEvent`, `setVrStickScroll` and
`updateSpatialElements`. `GuiScene` changes them into behavior calls, hover
updates, hotkeys and navigation.

All input runs on the main (API) thread. The one exception is
`JoystickHandler` (`dag_joystick.h`), which the client owns: off the main
thread it queues the button states, and `processPendingBtnStack()` on the main
thread sends `onJoystickBtnEvent` for each change.

## Principles

**One pointing model.** All pointing devices reach behaviors through
`Behavior::pointingEvent` as (device, pointer, button). The intent: a device
has M pointers, and each pointer has N buttons. Behaviors are mostly written
for this model. The core does not always follow it, and legacy code mixes up
which physical device sends which `InputDevice` (see Known Issues).

**Dispatch never stops.** An event goes to the matching behaviors in the input
stack from top to bottom. Behaviors coordinate through the result flags of the
behaviors before them (`accum_res`): `R_PROCESSED` says that one used the
event, `R_STOPPED` that the later ones should ignore it. An element with
`F_STOP_POINTING` (script `stopMouse`) adds both when the pointer hits it. The
elements below still see the event, for example a release, so that a pressed
button can reset.

**daRg owns the pointer positions.** The mouse (`MousePointer`) and the
gamepad cursor (`GamepadCursor`) each keep a position. `activePointingDevice`
tells which one is live; the visible cursor and the click buttons use
`activePointerPos()`. When the device changes, the new pointer starts where
the old one was, so the cursor does not jump. daRg writes its own moves to the
OS cursor (normally once per frame) and reads the position back, because the
driver can clamp it. A host that shows only the OS cursor (War Thunder, where
daGUI draws it) sets `gamepadCursorMovesMouse`: then the OS cursor follows
the gamepad cursor too, as a move of daRg's own.

**A gamepad is a full pointer.** The stick moves the gamepad cursor (not
through the HID mouse), click buttons click at it, and the dpad navigates
between elements. The pad touch pad gives relative motion, so it drives the
gamepad cursor too, instead of acting as a touch screen.

**Hover is derived state.** daRg calculates hover from the pointer positions;
events do not carry it. It runs at once after an input event that moves a
pointer, in `update()` for the moves that daRg makes itself (after the OS
cursor sync, so it uses the final position), and one render later when the
hit areas move only at render (scroll to XMB focus, a pad touch pad pan, a
screen focus change). Content that a script moves by layout under a still
pointer, such as a list that scrolls by `pos`, asks for the same with
`gui_scene.requestHoverUpdate()`. A request that comes while an update is
pending waits for the next render and does not cancel the pending one.

**Input off stops dispatch, not tracking.** When input is off
(`isInputActive()`: a host flag and a script flag) or the full error details
show, the event handlers call no behaviors,
hotkeys, clicks or navigation. They still track device state: a mouse move
moves the mouse pointer as usual, and a key, touch or VR press records whether
the keyboard was used last (`keyboardInUse`, for the on-screen keyboard on
consoles). So the pointer and the device state are current when input comes
back.

**Hotkeys read the live device state.** On a button event, the combos read
the live state of their buttons, not the event. The combo with the most
buttons wins (Ctrl+S over S).

**Script runs after dispatch.** Behaviors queue their script handlers, and the
event handler calls them when dispatch is done, so script does not change the
scene under the dispatch loop. When the event was processed, most handlers
then rebuild the invalidated parts at once, so the UI does not wait for the
next `update()`.

## Caveats

- `R_STOPPED` stops nothing: no dispatch loop reads it, and only `Pannable`
  checks it. Most behaviors ignore an event when `accum_res` has
  `R_PROCESSED`, and that is what makes `stopMouse` work. The mouse wheel
  adds no stop flags at all; after the first hit element, it goes only to the
  ancestors of that element.
- An element is in the input stack only with a mouse, touch, keyboard or focus
  behavior, hotkeys, an XMB node or an input element flag (`element.cpp`,
  where the stacks are filled). `F_HANDLE_JOYSTICK` or a stick flag alone does
  not put it there, and then the flag has no effect.
- `F_INTERNALLY_HANDLE_GAMEPAD_L_STICK` (or `R`) is read from the summary of
  the focused screen's input stack: one such behavior anywhere on the screen
  stops that stick from moving the cursor.
- Joystick behaviors can get button events also when the app is in the
  background; only hotkeys, clicks and navigation check `dgs_app_active`.
- `device` and `pointer_id` depend on the source (see Pointing events): a
  keyboard click button arrives as `DEVID_KEYBOARD`, a pad touch pad tap as
  `DEVID_JOYSTICK`, a VR finger as `DEVID_TOUCH` with the hand as the pointer
  and `TS_none`.
- From the VR hands, a panel that can be touched gets only `DEVID_TOUCH`, also
  when it can be pointed at too.
- Mouse hotkeys match before behaviors; keyboard and joystick hotkeys match
  after them.
- A stick scroll clears the XMB focus (with `scrollToEdge` on the XMB parent,
  only when the focused element leaves the view).
- Some behaviors call script directly during dispatch (`ProcessPointingInput`,
  `DragAndDrop` and others). Such script changes scene state (for example the
  keyboard focus) while dispatch still runs. Queue script handlers when you
  can.

## Reference

### Pointing events

| Source | device | pointer_id | btn_id |
|--------|--------|------------|--------|
| Mouse button | DEVID_MOUSE | 0 | mouse button |
| Mouse move | DEVID_MOUSE | 0 | 0 |
| Mouse wheel | DEVID_MOUSE | 0 | wheel delta |
| Gamepad cursor move | DEVID_JOYSTICK | 0 | 0 |
| Click button, pad touch pad tap | DEVID_KEYBOARD or DEVID_JOYSTICK | 0 | 0; 1 (right button) for a two-finger tap |
| Touch screen | DEVID_TOUCH | touch index | 0 |
| VR aim | DEVID_VR | aim source (0, 1, 2) | button |
| VR finger on a panel | DEVID_TOUCH | hand | 0 |

### Behavior flags

| Flag | Gets |
|------|------|
| `F_HANDLE_MOUSE` | `pointingEvent` from mouse, gamepad cursor, click buttons, pad touch pad taps and VR aim |
| `F_HANDLE_TOUCH` | `pointingEvent` from touch screen and VR finger |
| `F_HANDLE_TOUCHPAD_PAIR` | `pointingEvent` for a finger pair on a pad touch pad |
| `F_HANDLE_KEYBOARD` | `kbdEvent` when its element has the keyboard focus |
| `F_HANDLE_KEYBOARD_GLOBAL` | `kbdEvent` for all keys |
| `F_HANDLE_JOYSTICK` | `joystickBtnEvent` |

### Screens

There is one main screen (id `MAIN_SCREEN_ID`, 999, also a script const) and
one screen for each panel (id = the panel index given to
`gui_scene.addPanel()`, so panel 0 is not the main screen).

Touch screen and joystick button events go only to the `focusedScreen`.
Keyboard, hotkeys and click buttons go to each screen that
`doesScreenAcceptInput()`: the main screen and the focused screen, or all
of them with `allScreensAcceptInput`. Mouse and gamepad cursor moves go to
all screens.

### Stick dead zone

The joystick driver cuts its own dead zone and scales the rest.
`gamepadCursorDeadZone` is an absolute deflection, so daRg cuts only the part
above the driver dead zone: the effective dead zone is the larger of the two.
The cursor, the stick scroll and `IGuiScene::getGamepadStick()` use the same
rule; code outside daRg that reads a stick (for example a map pan) can call
`getGamepadStick()` to use the same dead zone.

### Pad touch pad

A game pad touch pad (`TS_touchPad`) gives finger motion.
One finger moves the gamepad cursor and a tap clicks at it.
Two fingers give `F_HANDLE_TOUCHPAD_PAIR` behaviors pointers near the cursor.
They pan and zoom. With `padTouchpadTwoFingerTapRmb` on (off by default),
a two-finger tap clicks the right button (button 1) at the cursor.
The touch pad button cancels taps if held at first touch or pressed later.
See `padTouchpad.h` and `onPadTouchpadEvent()` for the touch pad state.
A laptop touchpad gives mouse input through Windows.

### VR

Each frame the host gives `SpatialSceneData` to `updateSpatialElements()`. It
has room for the camera, the aim rays of the hands, a generic aim ray (not
bound to a hand), the index fingertips and a VR surface intersector; a host
fills only what it has, and no host in this repo fills the generic aim.

A panel is a UI surface made from script, with its own screen, usually placed
in the world. It can be pointed at (ray hit), touched (fingertip near it), or
both. When a panel screen has the focus, daRg also projects its screen-space
cursor onto the panel as the `Internal` aim source.

A VR surface hit sends no events and `IGuiScene` has no reader for it (the host
knows it from its own `vrSurfaceIntersector`). In daRg it only hides the
panels behind the surface from that aim.

## File Map

| File | Role |
|------|------|
| `publicInclude/daRg/dag_guiScene.h` | `IGuiScene` entry points, `SpatialSceneData` |
| `publicInclude/daRg/dag_behavior.h` | `Behavior`, behavior flags |
| `publicInclude/daRg/dag_inputIds.h` | `InputDevice`, `InputEvent` |
| `publicInclude/daRg/dag_guiConstants.h` | `BehaviorResult`, `Direction` |
| `publicInclude/daRg/dag_joystick.h` | `JoystickHandler` |
| `publicInclude/daRg/dag_element.h` | `Element` flags and state flags |
| `daRg/guiScene.h/.cpp` | All entry points, dispatch, hover, navigation, `update()` |
| `daRg/cursorState.h` | `CursorPosState`, shared by both pointers |
| `daRg/mousePointer.h/.cpp` | Mouse position and HID driver sync |
| `daRg/gamepadCursor.h/.cpp` | Stick to cursor, dead zone, stick scroll |
| `daRg/padTouchpad.h` | Pad touch pad state |
| `daRg/screen.h`, `daRg/inputStack.h` | Screens and input stacks |
| `daRg/element.cpp` | Which elements go into the input stack |
| `daRg/panel.h` | Panels |
| `daRg/hotkeys.h/.cpp` | Hotkey combos |
| `daRg/touchPointers.h`, `daRg/kbFocus.h` | Active touches, keyboard focus |
| `daRg/sceneConfig.h` | Gamepad, dirpad and click button config |
| `daRg/elementTree.cpp` | `updateHover()`, `doJoystickScroll()` |
| `daRg/xmb.cpp`, `daRg/dirPadNav.cpp` | XMB and dirpad navigation |

## Known Issues and Direction

- VR uses two device ids: controller aim sends `DEVID_VR`, a finger on a panel
  sends `DEVID_TOUCH`. Both should send `DEVID_VR` with one pointer, and use
  different button ids for click and touch.
- The loop "go through the input stack, call the behaviors" is repeated in
  `handleMouseClick`, the wheel and move paths of `onMouseEventInternal`,
  `dispatchTouchEvent`, `onPanelMouseEvent` and `onPanelTouchEvent`. They
  differ in the flag, the arguments and some special logic (wheel ancestors,
  keyboard focus). One dispatch helper could replace them.

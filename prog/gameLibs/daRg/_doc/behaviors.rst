daRg behaviors
=================

Button
------
Captures press inside hitbox (with configurable touch margin),
maintains "active" state while pressed, cancels if the element moves due to scrolling,
issues click or double-click on release within tolerance, optionally focuses the element,
and fires a touch-hold callback after the hold delay.
Handles mouse, touch, joystick, and keyboard activation.

**Properties**

- ``onClick([event])`` – function.
- ``onDoubleClick([event])`` – function.
- ``eventPassThrough`` – bool. If true, does not raise "consumed" flag on propagation.
- ``focusOnClick`` – bool. Gives keyboard focus on successful click.
- ``onTouchHold([event])`` – function. Called after holding for a delay.
- ``touchHoldTime`` – float (seconds). Delay for ``onTouchHold``.

DragAndDrop
-----------
Tracks pointer press; after movement threshold or ``dragStartDelay``, enters drag,
raises visual "drag" state, floats the element (translation and Z-on-top),
continuously hunts a valid drop target (``canDrop``), and on release calls that target's ``onDrop``.
If movement stayed under click threshold, it falls back to click/double-click handling.

**Properties**

- ``dragStartDelay`` – float (seconds). Press-and-hold delay before a drag starts.
- ``dropData`` – any object/value. Arbitrary payload associated with the dragged element; passed to drop handlers.
- ``onDrop(drop_data)`` – function. Called on the drop target when a drag is released over it; receives the source's ``dropData``.
- ``canDrop(drop_data) -> bool`` – function. Optional predicate on potential targets; if false, the element is excluded as a drop target.
- ``onDragMode(on: bool, drop_data)`` – function. Called on the draggable when drag mode toggles on/off; receives current ``dropData``.
- ``onClick([event])`` – function. Called on single click if not dragging.
- ``onDoubleClick([event])`` – function. Called on double click if not dragging.

Pannable
--------
Implements scrollable viewport with one or two-finger touch dragging, mouse dragging,
kinetic scrolling on release, and overscroll feedback. Supports axis locking and scroll velocity filtering.

**Properties**

- ``panMouseButton`` – int. Mouse button to use for panning (-1 for any).
- ``onTouchBegin()`` – function. Called when touch begins.
- ``kineticScrollOnTouchEnd(velocity)`` – function. Alternative to built-in kinetic scrolling.
- ``kineticAxisLockAngle`` – float (degrees). Angle threshold for axis locking kinetic scroll.

Pannable2Touch
--------------
Two-finger simultaneous touch panning; requires both fingers to be active for movement.

**Properties**

(None)

Slider
------
Provides interactive slider/seekbar control with keyboard navigation, mouse/touch dragging,
cursor positioning by click, and optional knob element. Supports custom value ranges,
stepping, and wheel scrolling.

**Properties**

- ``min`` – float. Minimum value (default: 0).
- ``max`` – float. Maximum value (default: 100).
- ``fValue`` – float. Current value (default: 0).
- ``unit`` – float. Step size for keyboard/wheel input (default: 1).
- ``pageScroll`` – float. Page up/down step size (default: 10).
- ``orientation`` – enum (horizontal/vertical). Slider orientation.
- ``knob`` – component constructor. Child element to use as draggable knob.
- ``onChange(value)`` – function. Called when value changes.
- ``onSliderMouseMove(value)`` – function. Called during mouse hover with potential value.
- ``ignoreWheel`` – bool. If true, ignores mouse wheel events.

TextInput
---------
Single-line text input with cursor positioning, keyboard editing, clipboard support,
character masking, IME integration, and validation callbacks.

**Properties**

- ``text`` – string. Current text content.
- ``cursorPos`` – int. Current cursor position.
- ``maxChars`` – int. Maximum character limit.
- ``charMask`` – string. Allowed characters (empty = all allowed).
- ``password`` – bool. If true, displays as password field.
- ``onChange(text)`` – function. Called when text changes.
- ``onReturn()`` – function. Called on Enter key press.
- ``onEscape()`` – function. Called on Escape key press.
- ``onWrongInput()`` – function. Called when invalid input is attempted.
- ``title`` – string. IME dialog title.
- ``hint`` – string. IME dialog hint text.
- ``inputType`` – string. IME input type hint.
- ``imeNoAutoCap`` – bool. Disables IME auto-capitalization.
- ``imeNoCopy`` – bool. Disables IME copy functionality.
- ``imeOpenJoyBtn`` – string. Joystick button to open IME.

TextArea
--------
Multi-line formatted text rendering with support for color tags, custom fonts,
text styling, and inline embedded components. Handles text parsing and layout formatting.

**Properties**

- ``text`` – string. Text content (may include formatting tags).
- ``colorTable`` – table. Map of color name to color value for tag resolution.
- ``tagsTable`` – table. Map of tag name to formatting attributes (color, font, fontSize).
- ``embed`` – table. Map of tag name to component description (table or closure). See **Embedded Components** below.
- ``preformatted`` – bool/int. Controls whitespace handling and formatting flags.
- ``spacing`` – float. Character spacing.
- ``lineSpacing`` – float. Additional line spacing.
- ``parSpacing`` – float. Paragraph spacing.
- ``indent`` – float. Text indentation.
- ``hangingIndent`` – float. Hanging indent for continuation lines.
- ``maxContentWidth`` – float. Maximum text width before wrapping.
- ``valign`` – enum. Vertical alignment (top/center/bottom).
- ``monoWidth`` – float/bool. Monospace width override.

**Embedded Components**

Inline UI elements can be placed within text using self-closing tags (``<name/>``).
Each tag name must correspond to a key in the ``embed`` table.

Components are rendered inline with text and aligned on the text baseline
(i.e. bottom edge of the component sits on the baseline).

Only absolute size modes are supported: ``pixels`` (e.g. ``hdpx(N)``, ``sh(N)``)
and ``fontH`` (e.g. ``fontH(150)``).
Relative sizes (``flex``, ``pw``, ``ph``, etc.) are not allowed.
If size is not specified, it defaults to the current font height for both axes.
Unlike layout, fontH is measured by the font ascent.
Tag names are limited to 16 characters.

Example::

  let textarea = {
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    text = "Click <icon/> to continue"
    embed = {
      icon = {
        size = [fontH(100), fontH(100)]
        rendObj = ROBJ_IMAGE
        image = Picture("!ui/atlas#arrow_icon")
      }
    }
  }

TextAreaEdit
------------
Editable multi-line text area with cursor navigation, text selection, formatting support,
and IME integration. Extends TextArea with editing capabilities.

**Properties**

- ``editableText`` – EditableText object. Text editing state and content.
- ``onChange(editableText)`` – function. Called when content changes.
- ``onImeFinish(applied)`` – function. Called when IME editing completes.
- ``title`` – string. IME dialog title.
- ``hint`` – string. IME dialog hint text.
- ``inputType`` – string. IME input type.
- ``maxChars`` – int. Maximum character limit.
- ``imeNoAutoCap`` – bool. Disables auto-capitalization.
- ``imeNoCopy`` – bool. Disables copy functionality.

PieMenu
-------
Radial/pie menu control supporting mouse, touch, and gamepad stick input.
Tracks sector selection and handles click events with sector information.

**Properties**

- ``devId`` – enum (mouse/joystick/vr). Input device type.
- ``stickNo`` – int. Gamepad stick number (0 or 1).
- ``sectorsCount`` – int. Number of pie sectors.
- ``curSector`` – observable. Current selected sector index.
- ``curAngle`` – observable. Current angle selection.
- ``onClick([sector])`` – function. Called on activation.
- ``eventPassThrough`` – bool. Allows event propagation.

ComboPopup
----------
A minimal click trigger suitable for combo dropdown popups; consumes the event on touch release.

**Properties**

- ``onClick()`` – function. Fired when popup is pressed (mouse) or released (touch).

EatInput
--------
On press inside the element, flags mouse/touch input as stopped so that elements behind will discard it.
Useful as a shield.

**Properties**

(None)

BoundToArea
-----------
Before render, clamps the element's transformed bbox inside the screen's safe area by
applying a translation so it stays fully visible.

**Properties**

- ``safeAreaMargin`` – offsets object/array (HTML notation: top, right, bottom, left).

Marquee
-------
Auto-scrolls content from start to end with initial delay, optional looping, and optional fade-out phase;
preserves position across recalcs when looping.
Resets when not hovered if ``scrollOnHover`` is set.

**Properties**

- ``loop`` – bool. If true, continuously loops the scroll.
- ``threshold`` – float. If overflow is within this threshold, disables marquee.
- ``orientation`` – enum/int (horizontal or vertical). Axis to scroll.
- ``speed`` – float. Scroll speed in pixels/sec (can be array for different phases).
- ``delay`` – float or array. Initial delay and optional fade-out delay.
- ``scrollOnHover`` – bool. If true, only scrolls while hovered.

MoveResize
----------
Detects presses on edges/corners/area using an epsilon around the bbox,
sets active visual state and cursor, and drives move/resize while the pointer moves.
Cleans up on release and input deactivation.

**Properties**

- ``moveResizeModes`` – bitmask of handles (edges, corners, area).
- ``moveResizeCursors`` – map (handle -> cursor object).
- ``cursor`` – cursor object, used during drag.
- ``onMoveResizeStarted(x, y, bbox)`` – function. Called when drag starts.
- ``onMoveResize(dx, dy, dw, dh) -> {pos, size}`` – function. Called during resize/move.
- ``onMoveResizeFinished()`` – function. Called when drag stops.

MoveToArea
----------
Continuously moves the element towards the target rect if present, positions/sizes it inside
the parent relative to the target; updates each frame with smooth animation.
Hides the element if the target is empty.

**Properties**

- ``target`` – object with ``targetRect`` property (BBox2).
- ``speed`` – float. Animation speed in pixels/sec.
- ``viscosity`` – float. Animation viscosity/damping factor.

InspectPicker
-------------
Provides "element picker" UX to inspect what is under the pointer, both on hover and on click.
Supports modifier keys to include elements without render objects.

**Properties**

- ``onChange(arrayOrNull)`` – function. Called on pointer move; receives array of records (bbox + elem).
- ``onClick(arrayOrNull)`` – function. Called on click; receives array of element info.
- On inspected elements: ``skipInspection`` – bool. If true, element is ignored.

RecalcHandler
-------------
Calls a handler when layout is recalculated, useful for responding to size changes.

**Properties**

- ``onRecalcLayout([initial[, elemRef]])`` – function. Called on layout recalculation.

RtPropUpdate
------------
Real-time property updater that calls ``update()`` on attach and every before-render frame,
merges returned properties into the component, re-runs setup, and optionally recalculates layout.

**Properties**

- ``update() -> table`` – function. Returns table of script properties to update.
- ``onlyWhenParentInScreen`` – bool. Updates only if parent is visible on screen.
- ``rtAlwaysUpdate`` – bool. Writes all returned keys regardless of whether they changed.
- ``rtRecalcLayout`` – bool. Forces layout recalculation after property update.

ScrollEvent
-----------
Detects changes to scroll offset, content size, and element size;
calls handler and triggers any bound scrollHandler with updated values.

**Properties**

- ``onScroll(elemRef)`` or ``onScroll(scrollX, scrollY, contentW, contentH, elemW, elemH)`` – function. Called when scroll position or dimensions change.

WheelScroll
-----------
Handles mouse wheel events to scroll the element content, with configurable step size and orientation.

**Properties**

- ``wheelStep`` – float. Scroll amount per wheel notch (default: 0.2).
- ``orientation`` – enum (horizontal/vertical). Scroll direction (default: vertical).
- ``onWheelScroll(delta)`` – function. Called on wheel scroll with delta amount.

SwipeScroll
-----------
Implements swipe-to-scroll behavior for touch interfaces, snapping to child element boundaries.

**Properties**

- ``panMouseButton`` – int. Mouse button for panning (-1 for any).
- ``onChange(index, swipeIsActive)`` – function. Called with current child index and swipe state.

Movie
-----
Video playback behavior supporting multiple formats (AV1, Ogg) with optional audio track.

**Properties**

- ``movie`` – string. Video file path.
- ``sound`` – string. Audio file path (defaults to video_file.mp3).
- ``soundVolume`` – float. Audio volume (0.0-1.0).
- ``loop`` – bool. Whether to loop playback.
- ``enableSound`` – bool. Enable audio playback (default: true).
- ``onStart()`` – function. Called when playback starts.
- ``onFinish()`` – function. Called when playback ends.
- ``onError()`` – function. Called on playback error.

OverlayTransparency
------------------
Manages element opacity based on overlap with higher-priority elements,
creating visual layering effects and center-based opacity gradients.

**Properties**

- ``priority`` – float. Element priority for overlay calculations.
- ``data.priorityOffset`` – float. Additional priority offset.
- ``data.opacityCenterMinMult`` – float. Minimum opacity multiplier for center-distance fading.
- ``data.opacityCenterRelativeDist`` – float. Distance threshold for center-based opacity.

Parallax
--------
Creates parallax scrolling effect based on mouse position relative to screen center.

**Properties**

- ``parallaxK`` – float. Parallax effect strength multiplier.

SmoothScrollStack
----------------
Animates stack children to smoothly flow into position with configurable speed and viscosity.

**Properties**

- ``speed`` – float. Animation speed (default: 10.0).
- ``viscosity`` – float. Animation viscosity for gap prevention (default: 0.025).
- ``orientation`` – enum (horizontal/vertical). Stack direction.

TransitionSize
--------------
Smoothly animates element size changes with configurable speed and axis control.

**Properties**

- ``orientation`` – enum (horizontal/vertical/both). Which axes to animate.
- ``speed`` – float. Animation speed in pixels/sec (default: 100.0).

TrackMouse
----------
Tracks mouse movement and wheel events, calling handlers for movement and scroll.
Includes Xbox-specific workarounds for missing mouse move events.

**Properties**

- ``onMouseMove(x, y)`` – function. Called on mouse movement.
- ``onMouseWheel(delta, x, y)`` – function. Called on mouse wheel events.
- ``eventPassThrough`` – bool. Allows event propagation.

ProcessPointingInput
--------------------
Low-level behavior for custom pointing device input processing, providing direct access
to press/release/move events with device information and modifier keys.

**Properties**

- ``onPointerPress(event)`` – function. Called on pointer press.
- ``onPointerRelease(event)`` – function. Called on pointer release.
- ``onPointerMove(event)`` – function. Called on pointer movement.

ProcessGesture
--------------
Gesture recognition behavior supporting pinch, rotate, and drag gestures with configurable
distance limits and phase callbacks.

**Properties**

- ``onGestureBegin(event)`` – function. Called when gesture starts.
- ``onGestureActive(event)`` – function. Called during active gesture.
- ``onGestureEnd(event)`` – function. Called when gesture ends.
- ``gestureDragDistanceMax`` – float. Maximum distance for drag gestures.
- ``gesturePinchDistanceMin`` – float. Minimum distance for pinch gestures.
- ``gestureRotateDistanceMin`` – float. Minimum distance for rotation gestures.

FpsBar
------
Real-time FPS and frame time display with color-coded performance indicators.
Updates text and color each frame based on current performance metrics.

**Properties**

(None readable)
- ``color`` – color. Display color (auto-updated by the behavior based on performance).

LatencyBar
----------
Displays input latency metrics and low-latency rendering information.

**Properties**

(None)

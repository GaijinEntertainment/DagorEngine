daRg Render Objects
===================

ROBJ_SOLID
----------
Renders a solid rectangle of a given color.

Properties:

* ``color`` — Color, fill color (default 0xFFFFFFFF).
* ``brightness`` — float, brightness modifier (0.0—1.0; default 1.0).

ROBJ_DEBUG
----------
Renders a debug rectangle with a 1px frame and diagonal cross-lines.

Properties:

* ``color`` — Color, stroke color (default 0xFFFFFFFF).
* ``brightness`` — float, brightness modifier (0.0—1.0; default 1.0).

ROBJ_IMAGE
----------
Renders a picture.

Properties:

* ``image`` — Picture, main image.
* ``fallbackImage`` — Picture, used when ``image`` fails/empty.
* ``color`` — Color, tint (default 0xFFFFFFFF).
* ``keepAspect`` — bool or enum ``KEEP_ASPECT_*``; default ``KEEP_ASPECT_NONE``.
* ``imageHalign`` / ``imageValign`` — enum ``ALIGN_*`` (default center).
* ``flipX`` / ``flipY`` — bool, flip texture coords.
* ``picSaturate`` — float, saturation factor (0—1; default 1.0).
* ``imageAffectsLayout`` — bool, if true the element size influences layout.
* ``brightness`` — float, brightness modifier (0—1; default 1.0).

ROBJ_VECTOR_CANVAS
------------------
Immediate-mode vector drawing; either run a ``draw(api, rect)`` function or execute a ``commands`` array (VECTOR_* opcodes).

Properties:

* ``color`` — Color, stroke color (default 0xFFFFFFFF).
* ``fillColor`` — Color, fill color (default 0xFFFFFFFF).
* ``lineWidth`` — float, stroke width (default 2).
* ``draw`` — function(api, rect), scripted draw.
* ``commands`` — array of VECTOR_* commands.
* ``brightness`` — float, brightness modifier (0—1; default 1.0).

ROBJ_9RECT
----------
Nine-slice scalable image renderer.

Properties:

* ``image`` — Picture, required.
* ``color`` — Color, modulation (default 0xFFFFFFFF).
* ``texOffs`` — array[4], texture slice offsets [top, right, bottom, left].
* ``screenOffs`` — array[4], on-screen slice offsets [top, right, bottom, left].
* ``brightness`` — float, brightness modifier (0—1; default 1.0).

ROBJ_PROGRESS_LINEAR
--------------------
Horizontal progress bar; negative values fill from the right.

Properties:

* ``fValue`` — float, progress (-1.0…1.0).
* ``bgColor`` — Color, background bar color (default 0x64646464).
* ``fgColor`` — Color, foreground bar color (default 0xFFFFFFFF).
* ``brightness`` — float, brightness modifier (0—1; default 1.0).

ROBJ_PROGRESS_CIRCULAR
----------------------
Circular progress indicator with optional radial image overlay.

Properties:

* ``fValue`` — float, magnitude 0.0—1.0 (sign selects direction).
* ``image`` — Picture, optional mask/overlay.
* ``fallbackImage`` — Picture, optional fallback if ``image`` invalid.
* ``bgColor`` — Color, background arc (default 0x64646464).
* ``fgColor`` — Color, foreground arc (default 0xFFFFFFFF).
* ``brightness`` — float, brightness modifier (0—1; default 1.0).

ROBJ_TEXT
---------
Renders single-line text. Requires the element's ``text`` value; supports font, effects and overflow controls.

Properties:

* ``font`` — Font id or ``Font.<name>``; defaults to scene default font.
* ``color`` — Color, text color; defaults to scene default.
* ``fontFx`` — enum FFT_* (text effect), default ``FFT_NONE``.
* ``fxColor`` — Color, effect color (default 0x78787878).
* ``fxFactor`` — int, effect factor (default 48).
* ``fxOffsX`` / ``fxOffsY`` — int, effect offsets (default 0).
* ``overflowX`` — enum TOVERFLOW_* (e.g., ``TOVERFLOW_CLIP``).
* ``ellipsis`` — bool, append ellipsis on overflow (default true).
* ``spacing`` — int, extra character spacing (default 0).
* ``monoWidth`` — number, fixed character width.
* ``password`` — null/bool/char; masks text with given char or "*".
* ``brightness`` — float, brightness modifier (0—1; default 1.0).
* ``fontSize`` — float, font size.
* ``fontTex`` — Picture, font texture.
* ``fontTexSu`` / ``fontTexSv`` — int, font texture UV coordinates (default 32).
* ``fontTexBov`` — int, font texture baseline offset vertical (default 0).

ROBJ_INSCRIPTION
----------------
Renders single-line text laid out as a precomputed inscription. Same properties as ``ROBJ_TEXT`` plus:

Properties:

* All properties from ``ROBJ_TEXT``
* Renders text as inscription in separate texture, providing better font effects but using more memory.

Note: Text rendering differs slightly in size compared to ``ROBJ_TEXT`` for the same fontSize.

ROBJ_TEXTAREA
-------------
Multi-line editable text area with scrolling and cursor rendering. Uses the same properties as ``ROBJ_TEXT`` plus the following.

Properties:

* ``overflowY`` — enum TOVERFLOW_* for vertical overflow (default ``TOVERFLOW_CLIP``).
* ``lowLineCount`` — int, minimum line count for alignment (default 0).
* ``lowLineCountAlign`` — enum alignment when there are few lines (default ``PLACE_DEFAULT``).
* ``spacing`` — int, extra character spacing.
* ``preformatted`` - bool or combination of the following flags:

  * ``FMT_NO_WRAP`` - disable auto wrapping of lines.
  * ``FMT_KEEP_SPACES`` - don't merge multiple spaces into one.
  * ``FMT_IGNORE_TAGS`` - keep tags as-is instead of interpreting them.
  * ``FMT_HIDE_ELLIPSIS`` - do not render ellipsis for overflowed text.
  * ``FMT_AS_IS`` - all of the above combined (equivalent to ``true``).

ROBJ_BOX
--------
Filled rectangle with optional image, border and rounded corners.

Properties:

* ``borderWidth`` — float or array[4] [top, right, bottom, left].
* ``borderRadius`` — float or array[4] [top-left, top-right, bottom-right, bottom-left].
* ``borderColor`` — Color, border color.
* ``fillColor`` — Color, fill (default transparent).
* ``image`` / ``fallbackImage`` — Picture(s), optional content.
* ``keepAspect`` — enum/bool; image aspect policy.
* ``imageHalign`` / ``imageValign`` — enum ``ALIGN_*`` (default center).
* ``flipX`` / ``flipY`` — bool, flip image.
* ``picSaturate`` — float, image saturation (0—1; default 1.0).
* ``brightness`` — float, brightness modifier (0—1; default 1.0).

ROBJ_FRAME
----------
Draws a rectangular frame; optionally fills the interior.

Properties:

* ``color`` — Color, border color.
* ``fillColor`` — Color, interior fill (default transparent).
* ``brightness`` — float, brightness modifier (0—1; default 1.0).
* ``borderWidth`` — float or array[4] in CSS order [top, right, bottom, left]; negatives clamped to 0.

ROBJ_MASK
---------
Applies a clipping mask so children render only through the mask.

Properties:

* ``image`` — Picture used as mask.

ROBJ_MOVIE
----------
Renders video frames from a movie player (requires ``behavior = Behaviors.Movie``).
Aspect handling and saturation similar to images.

Properties:

* ``movie`` — string, path to video file.
* ``color`` — Color, tint (default 0xFFFFFFFF).
* ``keepAspect`` — bool or enum ``KEEP_ASPECT_*`` (default ``KEEP_ASPECT_NONE``).
* ``picSaturate`` — float, 0—1 saturation (default 1.0).
* ``imageHalign`` / ``imageValign`` — enum ``ALIGN_*`` (default center).
* ``brightness`` — float, brightness modifier (0—1; default 1.0).
* ``loop`` — bool, whether to loop the video.
* ``enableSound`` — bool, enable sound playback (default true).
* ``sound`` — string, sound file path (defaults to "<video_file>.mp3").
* ``soundVolume`` — float, sound volume (default 1.0).

Event Handlers:

* ``onStart`` — called when playback starts.
* ``onFinish`` — called when playback finishes.
* ``onError`` — called on playback error.

ROBJ_DAS_CANVAS
---------------
Custom drawing canvas implemented in daslang. Property surface is defined by the das script.

Properties:

* ``script`` — DasScript instance.
* ``drawFunc`` — string, name of draw function in script.
* ``setupFunc`` — string, name of optional setup function in script (optional).

Function Signatures:

* Draw function: ``(var ctx: GuiContext&; rdata: ElemRenderData& const; rstate: RenderState& const; [data: <user's struct> &])``
* Setup function: ``(props: Properties&; var storage: <user's struct> &)``

ROBJ_WORLD_BLUR
---------------
Blurs the 3D world behind the UI and uses it as the object's image; you can tint it and overlay a fill color.

Properties:

* ``color`` — Color, tint applied to blurred scene (default 0xFFFFFFFF).
* ``fillColor`` — Color, overlay above blurred scene (default transparent).
* ``borderWidth`` — not set by default; if absent, border is zeroed.
* ``borderRadius`` — float or array[4] [top-left, top-right, bottom-right, bottom-left].
* ``saturateFactor`` — float, saturation factor (0—1; default 1.0).

ROBJ_WORLD_BLUR_PANEL
---------------------
Blurs both world and UI content and uses it as the object's image; same color and fill semantics as ``ROBJ_WORLD_BLUR``.

Properties:

* ``color`` — Color, tint applied to blurred content (default 0xFFFFFFFF).
* ``fillColor`` — Color, overlay above blurred content (default transparent).
* ``borderWidth`` — border width specification.
* ``borderRadius`` — float or array[4] [top-left, top-right, bottom-right, bottom-left].
* ``saturateFactor`` — float, saturation factor (0—1; default 1.0).

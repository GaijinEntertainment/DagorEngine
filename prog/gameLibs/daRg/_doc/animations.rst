daRg Animation System
=====================

Overview
--------

daRg provides an animation system that enables smooth, declarative
animations for UI elements. The system supports two primary mechanisms:

* **Animations** - Explicit, trigger-based animations that play on element
  appearance, disappearance, or custom events
* **Transitions** - Automatic interpolation when element properties change

Animations are defined declaratively in element descriptions and executed
by the C++ rendering engine for optimal performance.


Animatable properties
---------------------

The following properties can be animated using the ``AnimProp`` enum:

**Colors**

* ``AnimProp.color`` - Main element color (text, image tint)
* ``AnimProp.bgColor`` - Background color
* ``AnimProp.fgColor`` - Foreground color
* ``AnimProp.fillColor`` - Fill color (for boxes, frames)
* ``AnimProp.borderColor`` - Border color

**Rendering**

* ``AnimProp.opacity`` - Element opacity (0.0 - 1.0)
* ``AnimProp.picSaturate`` - Image saturation (0.0 - 1.0)
* ``AnimProp.brightness`` - Brightness modifier (0.0 - 1.0)

**Transformations**

* ``AnimProp.rotate`` - Rotation angle in degrees
* ``AnimProp.scale`` - Scale factor as [x, y]
* ``AnimProp.translate`` - Translation offset as [x, y]

**Other**

* ``AnimProp.fValue`` - Generic float value (used by progress bars, sliders)

.. note::

   Transform animations (``rotate``, ``scale``, ``translate``) require the
   element to have a ``transform`` property defined, even if empty:
   ``transform = true`` or ``transform = {}``


Animations
----------

Basic structure
~~~~~~~~~~~~~~~

Animations are defined as an array of animation descriptors on an element:

.. code-block:: quirrel

   {
     rendObj = ROBJ_SOLID
     size = sh(10)
     color = Color(220, 180, 120)
     transform = const {}  // Required field for transform animations

     animations = const [
       { prop=AnimProp.opacity, from=0, to=1, duration=0.3, play=true }
       { prop=AnimProp.scale, from=[0.8, 0.8], to=[1, 1], duration=0.3, play=true }
     ]
   }


Animation properties
~~~~~~~~~~~~~~~~~~~~

**Required**

* ``prop`` - The property to animate (from ``AnimProp`` enum)
* ``duration`` - Animation duration in seconds

**Value range**

* ``from`` - Starting value (type depends on property)
* ``to`` - Ending value (type depends on property)

If ``from`` or ``to`` is omitted, the current element property value is used.

**Playback control**

* ``play`` - If ``true``, animation plays when element appears
* ``playFadeOut`` - If ``true``, animation plays when element is removed
* ``loop`` - If ``true``, animation repeats indefinitely
* ``delay`` - Delay before animation starts (seconds)
* ``loopPause`` - Pause between loop iterations (seconds)
* ``globalTimer`` - If ``true``, synchronizes with global animation clock

**Easing and Timing**

* ``easing`` - Easing function (see `Easing functions`_)

**Triggers and Callbacks**

* ``trigger`` - Named trigger that starts this animation
* ``onEnter`` - Called when animation starts (or trigger name to start next)
* ``onStart`` - Called after delay, when animation actually begins playing
* ``onExit`` - Called when animation ends (or trigger name to start next)
* ``onFinish`` - Called when animation completes naturally
* ``onAbort`` - Called when animation is skipped or interrupted
* ``sound`` - Sound to play: ``{start = "sound_name", stop = "sound_name"}``


Appearance and disappearance animations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``play`` for entrance animations and ``playFadeOut`` for exit animations:

.. code-block:: quirrel

   {
     key = "my_panel"
     transform = true
     clipChildren = true

     animations = const [
       // Entrance: fade in and scale up
       { prop=AnimProp.opacity, from=0, to=1, duration=0.15, play=true, easing=OutCubic }
       { prop=AnimProp.scale, from=[0.9, 0.9], to=[1, 1], duration=0.15, play=true, easing=OutCubic }

       // Exit: fade out and scale down
       { prop=AnimProp.opacity, from=1, to=0, duration=0.12, playFadeOut=true, easing=OutCubic }
       { prop=AnimProp.scale, from=[1, 1], to=[0.9, 0.9], duration=0.12, playFadeOut=true, easing=OutCubic }
     ]
   }

.. important::

   When using ``playFadeOut``, add a ``key`` property may be needed to
   help daRg track elements correctly.


Trigger-based animations
~~~~~~~~~~~~~~~~~~~~~~~~

Create complex animation sequences with named triggers:

.. code-block:: quirrel

   let item = {
     size = sh(15)
     rendObj = ROBJ_SOLID
     color = Color(220, 180, 120)
     transform = const { pivot = [0.5, 0.5] }

     animations = const [
       // Initial animation plays on appearance
       { prop=AnimProp.scale, from=[0,0], to=[1,1], duration=0.5, play=true,
         easing=OutCubic, trigger="initial" }

       // Chain to next animation via onExit
       { prop=AnimProp.rotate, from=-45, to=0, duration=0.5, play=true,
         easing=OutCubic, trigger="initial", onExit="pulse" }

       // Triggered by previous animation's onExit
       { prop=AnimProp.scale, from=[1,1], to=[1.1,1.1], duration=0.3,
         easing=InOutCubic, trigger="pulse", onExit="pulse_back" }

       { prop=AnimProp.scale, from=[1.1,1.1], to=[1,1], duration=0.3,
         easing=InOutCubic, trigger="pulse_back", onFinish=@() vlog("Done!") }
     ]
   }


Controlling animations from script
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use these functions to control animations programmatically:

.. code-block:: quirrel

   // Start an animation by its trigger name
   anim_start("pulse")

   // Request animation to stop (finishes current iteration)
   anim_request_stop("pulse")

   // Skip animation immediately (jumps to end state)
   anim_skip("pulse")

   // Skip only the delay, start animation immediately
   anim_skip_delay("pulse")

Example with event-driven animation:

.. code-block:: quirrel

   let pulseAnim = {}  // Object reference as trigger

   let animatedButton = {
     behavior = Behaviors.Button
     onClick = @() anim_start(pulseAnim)

     transform = true
     animations = const [
       { prop=AnimProp.scale, from=[1,1], to=[1.2,1.2], duration=0.15,
         easing=OutCubic, trigger=pulseAnim, onExit=pulseAnim + "_back" }
       { prop=AnimProp.scale, from=[1.2,1.2], to=[1,1], duration=0.15,
         easing=OutCubic, trigger=pulseAnim + "_back" }
     ]
   }


Looping animations
~~~~~~~~~~~~~~~~~~

Create continuous animations with the ``loop`` property:

.. code-block:: quirrel

   // Simple continuous rotation
   { prop=AnimProp.rotate, from=0, to=360, duration=2, play=true, loop=true, easing=Linear }

   // Pulsing with pause between pulses
   { prop=AnimProp.scale, from=[1,1], to=[1.1,1.1], duration=0.5,
     play=true, loop=true, loopPause=1.0, easing=InOutSine }

   // Color cycling
   { prop=AnimProp.color, from=Color(255,255,255), to=Color(255,0,0),
     duration=1, play=true, loop=true, easing=CosineFull }


Global timer synchronization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When multiple elements need synchronized looped animations, use ``globalTimer``:

.. code-block:: quirrel

   // Without globalTimer: each element animates independently
   // depending on time they were created
   let block1 = { animations = const [{ prop=AnimProp.opacity, from=0.5, to=1,
     duration=1, play=true, loop=true, easing=CosineFull }] }

   // With globalTimer: all elements pulse in sync
   let block2 = { animations = const [{ prop=AnimProp.opacity, from=0.5, to=1,
     duration=1, play=true, loop=true, easing=CosineFull, globalTimer=true }] }


Transitions
-----------

Transitions automatically animate property changes when element state changes.
Unlike animations, transitions don't require explicit from/to values - they
interpolate between the previous and new property values.


Basic transitions
~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   let stateFlags = Watched(0)

   let button = @() {
     watch = stateFlags
     onElemState = @(s) stateFlags.set(s)
     behavior = Behaviors.Button

     rendObj = ROBJ_BOX
     fillColor = (stateFlags.get() & S_HOVER) ? Color(100,150,200) : Color(60,60,60)
     size = const [sh(20), sh(8)]

     // Animate fillColor changes over 0.25 seconds
     transitions = const [
       { prop=AnimProp.fillColor, duration=0.25, easing=OutCubic }
     ]
   }


Multiple transitions
~~~~~~~~~~~~~~~~~~~~

Animate several properties simultaneously:

.. code-block:: quirrel

   let button = watchElemState(@(sf) {
     rendObj = ROBJ_BOX
     fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
     opacity = (sf & S_HOVER) ? 1.0 : 0.7

     transform = {
       scale = (sf & S_HOVER) ? const [1.05, 1.05] : const [1, 1]
     }

     transitions = const [
       { prop=AnimProp.fillColor, duration=0.25, easing=OutCubic }
       { prop=AnimProp.opacity, duration=0.3, easing=OutCubic }
       { prop=AnimProp.scale, duration=0.2, easing=InOutCubic }
     ]
   })


Dynamic duration
~~~~~~~~~~~~~~~~

Transition duration be changed dynamically, it is re-read when component is rebuilt:

.. code-block:: quirrel

   let button = @() {
     watch = stateFlags
     let sf = stateFlags.get()

     fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)

     // Slower animation when pressing, faster when releasing
     transitions = [
       { prop=AnimProp.fillColor, duration = (sf & S_ACTIVE) ? 0.5 : 0.15 }
     ]
   }


TransitAll - applying to all properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``TransitAll`` to apply the same transition configuration to all animatable
properties of an element at once:

.. code-block:: quirrel

   let button = watchElemState(@(sf) {
     rendObj = ROBJ_BOX
     fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
     opacity = (sf & S_HOVER) ? 1.0 : 0.7

     // Apply same transition to all supported properties
     transitions = TransitAll({ duration=0.25, easing=OutCubic })
   })

``TransitAll`` automatically creates transitions for all properties that the
element's render object supports (colors, opacity, transforms if ``transform``
is defined, etc.).


Transition callbacks
~~~~~~~~~~~~~~~~~~~~

Transitions support lifecycle callbacks similar to animations (except ``onStart``):

.. code-block:: quirrel

   transitions = [
     { prop=AnimProp.fillColor, duration=0.3,
       onEnter = @() vlog("Transition started"),
       onFinish = @() vlog("Transition completed"),
       onAbort = @() vlog("Transition interrupted"),
       onExit = @() vlog("Transition ended")
     }
   ]


Easing functions
----------------

Easing functions control the rate of change during animation. They transform
a linear progress value (0 to 1) into a curved progression.


Standard easing
~~~~~~~~~~~~~~~

**Linear**

* ``Linear`` - Constant speed, no acceleration

**Quadratic** (power of 2)

* ``InQuad`` - Accelerate from zero
* ``OutQuad`` - Decelerate to zero
* ``InOutQuad`` - Accelerate then decelerate

**Cubic** (power of 3)

* ``InCubic`` - Stronger acceleration
* ``OutCubic`` - Stronger deceleration (recommended for UI)
* ``InOutCubic`` - Smooth acceleration and deceleration

**Quartic** (power of 4)

* ``InQuart``, ``OutQuart``, ``InOutQuart``

**Quintic** (power of 5)

* ``InQuintic``, ``OutQuintic``, ``InOutQuintic``

**Sinusoidal**

* ``InSine``, ``OutSine``, ``InOutSine`` - Gentle, natural motion

**Circular**

* ``InCirc``, ``OutCirc``, ``InOutCirc`` - Circular arc motion

**Exponential**

* ``InExp``, ``OutExp``, ``InOutExp`` - Dramatic acceleration/deceleration

**Elastic**

* ``InElastic``, ``OutElastic``, ``InOutElastic`` - Damped sine wave oscillation

**Back** (overshoot)

* ``InBack``, ``OutBack``, ``InOutBack`` - Overshoots target then settles

**Bounce**

* ``InBounce``, ``OutBounce``, ``InOutBounce`` - Bouncing ball effect


Special easing
~~~~~~~~~~~~~~

* ``InOutBezier`` - Smoothstep curve (Hermite interpolation)
* ``CosineFull`` - Full cosine wave (0 to 1 and back, good for looping)
* ``InStep`` - Stays at 0 until halfway, then jumps to 1
* ``OutStep`` - Stays at 1 until halfway, then drops to 0
* ``Discrete8`` - Quantized to 8 steps / positions
* ``Blink`` - Quick fade in, slow fade out
* ``DoubleBlink`` - Two pulses per cycle
* ``BlinkSin``, ``BlinkCos`` - Blinking with oscillation overlay
* ``Shake4``, ``Shake6`` - Shake/vibrate effects (4 or 6 oscillations)


Custom easing functions
~~~~~~~~~~~~~~~~~~~~~~~

Define custom easing as a function that takes progress (0-1) and returns
modified progress (0-1):

.. code-block:: quirrel

   // Custom delayed fade-in
   const function delayedFadeIn(t) {
     if (t < 0.3)
       return 0
     let s = (t - 0.3) / 0.7
     return s * s  // Quadratic ease after delay
   }

   animations = const [
     { prop=AnimProp.opacity, from=0, to=1, duration=1.0,
       play=true, easing=delayedFadeIn }
   ]

   // Overshoot bounce-back
   let overshoot = @(t) t < 0.7
     ? t / 0.7 * 1.2
     : 1.2 - (t - 0.7) / 0.3 * 0.2


Example patterns
----------------

Modal window animation
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   function modalWindow(content) {
     return {
       key = "modal"
       size = flex()
       rendObj = ROBJ_SOLID
       color = Color(0, 0, 0, 150)
       halign = ALIGN_CENTER
       valign = ALIGN_CENTER

       transform = true
       animations = const [
         { prop=AnimProp.opacity, from=0, to=1, duration=0.2, play=true, easing=OutCubic }
         { prop=AnimProp.opacity, from=1, to=0, duration=0.15, playFadeOut=true, easing=OutCubic }
       ]

       children = {
         rendObj = ROBJ_BOX
         fillColor = Color(40, 40, 50)
         transform = true
         animations = const [
           { prop=AnimProp.scale, from=[0.95, 0.95], to=[1, 1], duration=0.2, play=true, easing=OutCubic }
           { prop=AnimProp.scale, from=[1, 1], to=[0.95, 0.95], duration=0.15, playFadeOut=true, easing=OutCubic }
         ]
         children = content
       }
     }
   }


Dropdown/popup animation
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   function dropdown(items, dropUp = false) {
     return {
       key = "dropdown"
       flow = FLOW_VERTICAL
       clipChildren = true

       transform = const {
         pivot = [0.5, dropUp ? 1 : 0]  // Scale from top or bottom
       }

       animations = const [
         { prop=AnimProp.opacity, from=0, to=1, duration=0.12, play=true, easing=OutQuad }
         { prop=AnimProp.scale, from=[1, 0], to=[1, 1], duration=0.12, play=true, easing=OutQuad }
         { prop=AnimProp.opacity, from=1, to=0, duration=0.1, playFadeOut=true }
         { prop=AnimProp.scale, from=[1, 1], to=[1, 0], duration=0.1, playFadeOut=true, easing=OutQuad }
       ]

       children = items
     }
   }


Staggered list animation
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   function animatedListItem(item, index) {
     let staggerDelay = index * 0.05

     return {
       key = item.id
       transform = true
       animations = const [
         { prop=AnimProp.translate, from=[sh(-5), 0], to=[0, 0],
           duration=0.3, play=true, delay=staggerDelay, easing=OutCubic }
         { prop=AnimProp.opacity, from=0, to=1,
           duration=0.25, play=true, delay=staggerDelay, easing=OutCubic }
       ]
       children = itemContent(item)
     }
   }

   function animatedList(items) {
     return {
       flow = FLOW_VERTICAL
       children = items.map(@(item, idx) animatedListItem(item, idx))
     }
   }


Character-by-character text animation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   import "utf8" as utf8

   function animatedText(text) {
     let ut = utf8(text)
     let chars = []
     for (local i = 1; i <= ut.charCount(); i++)
       chars.append(ut.slice(i - 1, i))

     return {
       flow = FLOW_HORIZONTAL
       children = chars.map(@(ch, i) {
         rendObj = ROBJ_TEXT
         text = ch
         key = {}
         transform = true
         animations = const [
           { prop=AnimProp.translate, from=[0, -sh(5)], to=[0, 0],
             duration=0.2, play=true, delay=i*0.04, easing=OutCubic }
           { prop=AnimProp.opacity, from=0, to=1,
             duration=0.15, play=true, delay=i*0.04, easing=OutCubic }
         ]
       })
     }
   }


Hover scale effect with transition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   let hoverCard = watchElemState(@(sf) {
     rendObj = ROBJ_BOX
     fillColor = Color(50, 50, 60)
     size = [sh(20), sh(15)]

     transform = {
       scale = (sf & S_HOVER) ? [1.05, 1.05] : [1, 1]
     }

     transitions = const [
       { prop=AnimProp.scale, duration=0.2, easing=OutCubic }
     ]

     children = cardContent
   })


Loading spinner
~~~~~~~~~~~~~~~

.. code-block:: quirrel

   let spinner = {
     rendObj = ROBJ_IMAGE
     size = sh(5)
     image = Picture("ui/spinner.svg")
     transform = true

     animations = const [
       { prop=AnimProp.rotate, from=0, to=360, duration=1,
         play=true, loop=true, easing=Linear }
     ]
   }


Pulsing indicator
~~~~~~~~~~~~~~~~~

.. code-block:: quirrel

   let pulsingDot = {
     rendObj = ROBJ_SOLID
     size = sh(2)
     color = Color(100, 200, 100)
     transform = true

     animations = const [
       { prop=AnimProp.scale, from=[1, 1], to=[1.3, 1.3], duration=0.8,
         play=true, loop=true, easing=CosineFull }
       { prop=AnimProp.opacity, from=1, to=0.5, duration=0.8,
         play=true, loop=true, easing=CosineFull }
     ]
   }


Tips
----

1. **Prefer transforms over layout-affecting properties** - Animating
   ``scale``, ``rotate``, ``translate``, and ``opacity`` is more efficient
   than animating properties that trigger layout recalculation.

2. **Use ``key`` on animated elements** - Keys help daRg track elements
   correctly, especially for fade-out animations.


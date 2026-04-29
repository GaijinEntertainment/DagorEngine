daRg Layout System
==================

Overview
--------

daRg uses a flexible layout model inspired by CSS Flexbox. Elements can size
themselves based on content, fill available space, align relative to parent
containers, or use fixed positioning.

Every element has a bounding box defined by its position and size. The layout
system determines these values based on the element's properties and its
relationship to parent and sibling elements.


Size specification
------------------

The ``size`` property defines an element's dimensions. It accepts various formats:

**Array format** (most common):

.. code-block:: quirrel

   size = [width, height]

**Single value** (applies to both dimensions):

.. code-block:: quirrel

   size = 100        // 100x100 pixels
   size = sh(10)     // 10% of screen height for both dimensions
   size = flex()     // fill available space in both dimensions
   size = SIZE_TO_CONTENT  // fit to children in both dimensions

Note: when using arrays, mark them with ``const`` where possible to avoid dynamically recreating
these arrays each time.

Size modes
~~~~~~~~~~

**Pixels** - Fixed size in pixels:

.. code-block:: quirrel

   size = [200, 100]           // 200px wide, 100px tall
   size = const [200, 100]     // compile-time constant, preferred, more efficient (doesn't recreate the array)

**Screen-relative functions**:

* ``sw(percent)`` - Percentage of screen width
* ``sh(percent)`` - Percentage of screen height
* ``hdpx(pixels)`` - DPI-aware pixels scaled to 1080p baseline (float)
* ``hdpxi(pixels)`` - Same as ``hdpx`` but returns integer
* ``fsh(percent)`` - Aspect-ratio-safe screen height (uses ``sh`` on wide screens,
  falls back to ``sw(0.75 * percent)`` on narrow screens to prevent overflow).
  May be overriden for project-specific needs.

These functions return values recalculated to pixels.

.. code-block:: quirrel

   size = [sw(50), sh(25)]     // 50% screen width, 25% screen height
   size = hdpx(100)            // 100px at 1080p, scales with resolution
   size = fsh(10)              // 10% of screen height, safe for narrow displays

**Parent-relative functions** (should not be used unless there are no other options):

* ``pw(percent)`` - Percentage of parent width
* ``ph(percent)`` - Percentage of parent height

.. code-block:: quirrel

   size = [pw(100), ph(50)]    // full parent width, half parent height

**Content-based**:

* ``SIZE_TO_CONTENT`` - Size to fit children

.. code-block:: quirrel

   size = SIZE_TO_CONTENT              // both dimensions
   size = [sw(50), SIZE_TO_CONTENT]    // fixed width, height fits content

**Flexible**:

* ``flex([weight])`` - Fill remaining space, optionally weighted

.. code-block:: quirrel

   size = flex()               // fill all available space
   size = [flex(), sh(10)]     // flexible width, fixed height
   size = [flex(2), flex(1)]   // 2:1 ratio when multiple flex children

**Font-relative**:

* ``fontH(percent)`` - Size relative to current font height

.. code-block:: quirrel

   size = [sw(50), fontH(200)]   // two lines of text height (200% of font height)


Convenience shortcuts
~~~~~~~~~~~~~~~~~~~~~

The standard library provides shortcuts for common patterns:

.. code-block:: quirrel

   FLEX_H = [flex(), SIZE_TO_CONTENT]  // flexible width, content height
   FLEX_V = [SIZE_TO_CONTENT, flex()]  // content width, flexible height
   FLEX = flex()                       // fill both dimensions

   // Functions for weighted flex shortcuts
   flex_h(weight)  // returns [flex(weight), SIZE_TO_CONTENT]
   flex_v(weight)  // returns [SIZE_TO_CONTENT, flex(weight)]

.. code-block:: quirrel

   // Using weighted flex shortcuts
   {
     flow = FLOW_HORIZONTAL
     children = [
       { size = flex_h(2), /* ... */ }  // 2/3 of width, content height
       { size = flex_h(1), /* ... */ }  // 1/3 of width, content height
     ]
   }


Flow layout
-----------

The ``flow`` property controls how children are arranged:

* ``FLOW_PARENT_RELATIVE`` (default) - Children positioned relative to parent
* ``FLOW_HORIZONTAL`` - Children arranged left-to-right
* ``FLOW_VERTICAL`` - Children arranged top-to-bottom

.. code-block:: quirrel

   {
     flow = FLOW_HORIZONTAL
     children = [child1, child2, child3]  // arranged horizontally
   }

When ``flow`` is set, children are laid out sequentially. Without flow (or with
``FLOW_PARENT_RELATIVE``), children are positioned independently and may overlap.


Alignment
---------

Content alignment (halign, valign)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Controls how children are aligned within the parent's content area:

* ``halign`` - Horizontal alignment: ``ALIGN_LEFT``, ``ALIGN_CENTER``, ``ALIGN_RIGHT``
* ``valign`` - Vertical alignment: ``ALIGN_TOP``, ``ALIGN_CENTER``, ``ALIGN_BOTTOM``

.. code-block:: quirrel

   {
     size = const [sw(50), sh(50)]
     halign = ALIGN_CENTER
     valign = ALIGN_CENTER
     children = childElement  // centered in parent
   }

With ``flow`` set, alignment affects the cross-axis and distribution:

.. code-block:: quirrel

   {
     flow = FLOW_HORIZONTAL
     halign = ALIGN_CENTER    // centers the row of children horizontally
     valign = ALIGN_CENTER    // vertically centers each child in the row
     children = [child1, child2, child3]
   }


Self placement (hplace, vplace)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Controls where an element positions itself within its parent, independent of
siblings:

* ``hplace`` - Horizontal placement: ``ALIGN_LEFT``, ``ALIGN_CENTER``, ``ALIGN_RIGHT``
* ``vplace`` - Vertical placement: ``ALIGN_TOP``, ``ALIGN_CENTER``, ``ALIGN_BOTTOM``

.. code-block:: quirrel

   {
     size = sh(20)
     hplace = ALIGN_CENTER
     vplace = ALIGN_BOTTOM
     // element centered horizontally, at bottom of parent
   }

Note: ``hplace``/``vplace`` are ignored for children when the parent has
``flow`` set in that axis direction.


Margin and padding
------------------

Spacing properties accept pixels or screen-relative functions (``sh``, ``sw``,
``hdpx``, ``fsh``, ``fontH``). Parent-relative functions (``pw``, ``ph``) are
**not supported** for spacing.

**Padding** - Inner spacing between element border and its children:

.. code-block:: quirrel

   padding = 10                           // all sides (pixels)
   padding = hdpx(10)                     // DPI-aware pixels
   padding = [sh(1), sw(2)]               // vertical, horizontal
   padding = [10, 20, 30, 40]             // top, right, bottom, left (CSS order)

**Margin** - Outer spacing around the element:

.. code-block:: quirrel

   margin = 10                            // all sides
   margin = [10, 20]                      // vertical, horizontal
   margin = [10, 20, 30, 40]              // top, right, bottom, left

In flow layouts, adjacent margins collapse - the larger margin wins:

.. code-block:: quirrel

   // First child margin-right: 20, second child margin-left: 30
   // Actual gap between them: max(20, 30) = 30

Negative margins are supported and can create overlapping effects:

.. code-block:: quirrel

   margin = -10  // element overlaps neighbors by 10px


Gap
---

The ``gap`` property sets spacing between children in flow layouts. Like
margin/padding, it accepts pixels and screen-relative functions but not
``pw``/``ph``.

**Numeric gap**:

.. code-block:: quirrel

   {
     flow = FLOW_HORIZONTAL
     gap = sh(2)              // 2% screen height between each child
     children = [child1, child2, child3]
   }

**Component gap** - A full component can serve as the gap, allowing for
decorative separators:

.. code-block:: quirrel

   {
     flow = FLOW_VERTICAL
     gap = {
       size = const [flex(), hdpx(1)]
       rendObj = ROBJ_SOLID
       color = 0xFF333333
     }
     children = [item1, item2, item3]
   }

**Negative gap** - Creates overlapping children:

.. code-block:: quirrel

   {
     flow = FLOW_VERTICAL
     gap = -hdpx(10)          // children overlap by 10px
     children = [card1, card2, card3]
   }

Note: ``null`` children in the children array do not create gaps. A component
affects layout (and creates gaps) if it has any ``rendObj``, explicit ``size``,
or ``children``.


Size constraints
----------------

Limit element dimensions with min/max constraints:

* ``minWidth``, ``minHeight`` - Minimum dimensions
* ``maxWidth``, ``maxHeight`` - Maximum dimensions

.. code-block:: quirrel

   {
     size = flex()
     minWidth = hdpx(200)
     maxWidth = hdpx(800)
     minHeight = SIZE_TO_CONTENT  // at least as tall as content
   }

Constraints accept the same size specifications as ``size``:

.. code-block:: quirrel

   {
     size = FLEX_H
     minWidth = SIZE_TO_CONTENT  // expand to fit content, but can grow
     maxWidth = sw(50)           // but never more than half screen
   }


Positioning
-----------

The ``pos`` property sets an element's position offset:

.. code-block:: quirrel

   {
     pos = [100, 50]           // offset 100px right, 50px down from layout position
   }

Position accepts the same size specifications:

.. code-block:: quirrel

   pos = [pw(50), ph(50)]      // offset by half parent size
   pos = [sw(10), sh(5)]       // screen-relative offset

Position is applied after layout calculation, so it shifts the element from
where it would normally be placed.


Children sorting
----------------

The ``sortChildren`` property reorders children based on their ``sortOrder``
values during layout:

.. code-block:: quirrel

   {
     sortChildren = true
     children = [
       { sortOrder = 3, /* ... */ }   // laid out and rendered last
       { sortOrder = 1, /* ... */ }   // laid out and rendered first
       { sortOrder = 2, /* ... */ }   // laid out and rendered second
     ]
   }

Higher ``sortOrder`` values are processed later. This affects both layout order
(in flow containers) and rendering order. Useful for dynamically reordering
items while keeping declarations in logical order.


Image layout
------------

Images have special layout considerations:

**keepAspect** - Controls aspect ratio preservation:

* ``KEEP_ASPECT_NONE`` - Stretch to fill (default)
* ``KEEP_ASPECT_FIT`` - Scale to fit, may have empty space
* ``KEEP_ASPECT_FILL`` - Scale to fill, may crop

This determines how the image is placed inside the element.
The element itself is laid out as usual.

.. code-block:: quirrel

   {
     size = const [hdpx(200), hdpx(100)]
     rendObj = ROBJ_IMAGE
     image = Picture("ui/image.png")
     keepAspect = KEEP_ASPECT_FIT
     imageHalign = ALIGN_CENTER    // horizontal alignment within bounds
     imageValign = ALIGN_CENTER    // vertical alignment within bounds
   }

**imageAffectsLayout** - When true, image dimensions influence element sizing:

.. code-block:: quirrel

   {
     size = SIZE_TO_CONTENT
     rendObj = ROBJ_IMAGE
     image = Picture("ui/icon.png")
     imageAffectsLayout = true     // element sizes to image
   }


Transform
---------

The ``transform`` property applies visual transformations that do not affect
layout calculations. Transforms are applied during rendering only.

.. code-block:: quirrel

   {
     transform = const {
       pivot = [0.5, 0.5]      // transform origin (0-1 range, default center)
       rotate = 45             // rotation in degrees
       scale = [1.2, 1.2]      // scale factors
       translate = [10, 20]    // translation offset in pixels
     }
     // ...
   }

Transform properties:

* ``pivot`` - Transform origin as [x, y] in 0-1 range (default [0.5, 0.5] = center)
* ``rotate`` - Rotation angle in degrees
* ``scale`` - Scale factors as [x, y] or single value for uniform scale
* ``translate`` - Translation offset in pixels as [x, y]

All fields are optional. If not specified, default values are used.

Transforms are purely visual - the layout system still uses the untransformed
bounding box. Use transforms for animations, hover effects, and visual polish.

If default transformation is sufficient, but it needs to be animated,
a mandatory ``transform`` field is required. In this case it can be initialized
with ``true`` or ``const {}``. Without it animations has nothing to be applied to.

Z-Order control
---------------

The ``zOrder`` property controls render ordering independent of tree structure:

.. code-block:: quirrel

   {
     children = [
       { zOrder = 1, /* renders behind */ }
       { zOrder = 10, /* renders on top */ }
     ]
   }

Higher ``zOrder`` values render on top. Unlike ``sortChildren``/``sortOrder``,
``zOrder`` affects only rendering - layout order remains unchanged. Use
``zOrder`` when elements should visually overlap but maintain their layout
positions.


Scrolling
---------

Elements can be scrollable containers with content larger than their bounds.

**Basic scroll setup**:

.. code-block:: quirrel

   {
     size = const [sw(50), sh(50)]
     clipChildren = true
     behavior = Behaviors.WheelScroll  // or Behaviors.Pannable
     children = {
       size = const [flex(), SIZE_TO_CONTENT]
       flow = FLOW_VERTICAL
       children = longContentList
     }
   }

**Initial scroll offset**:

.. code-block:: quirrel

   scrollOffsX = 100   // initial horizontal scroll position
   scrollOffsY = 200   // initial vertical scroll position

**ScrollHandler** - For programmatic scroll control and monitoring:

.. code-block:: quirrel

   let scrollHandler = ScrollHandler()

   {
     scrollHandler = scrollHandler
     // ...
   }

   // Later: scrollHandler.scrollToX(100) or scrollHandler.scrollToChildren(elem)


Subpixel positioning
--------------------

By default, element positions are rounded to whole pixels for crisp rendering.
Enable ``subPixel`` for smooth animations:

.. code-block:: quirrel

   {
     subPixel = true    // allow fractional pixel positions
     // useful for smooth animations, gradual movements
   }

Subpixel positioning is inherited from parent by default. Use it when element
positions need to animate smoothly without "snapping" to pixels.


Clipping
--------

The ``clipChildren`` property clips child content to the element bounds:

.. code-block:: quirrel

   {
     size = const [hdpx(200), hdpx(100)]
     clipChildren = true
     children = {
       size = const [hdpx(300), hdpx(200)]  // larger than parent, will be clipped
       // ...
     }
   }

Essential for scroll containers and overflow handling.


Layout calculation order
------------------------

The layout system processes elements in this order:

1. **Fixed sizes** - Calculate pixel, screen-relative, and parent-relative sizes
2. **Content sizes** - Calculate SIZE_TO_CONTENT dimensions bottom-up
3. **Flex distribution** - Distribute remaining space among flex children
4. **Constraints** - Apply min/max limits
5. **Alignment** - Position children according to alignment properties
6. **Screen positions** - Calculate absolute screen coordinates


Common patterns
---------------

**Centered content**:

.. code-block:: quirrel

   {
     size = flex()
     halign = ALIGN_CENTER
     valign = ALIGN_CENTER
     children = content
   }

**Header-content-footer layout**:

.. code-block:: quirrel

   {
     size = flex()
     flow = FLOW_VERTICAL
     children = [
       { size = [flex(), hdpx(60)], /* header */ }
       { size = flex(), /* content fills remaining space */ }
       { size = [flex(), hdpx(40)], /* footer */ }
     ]
   }

**Equal-width columns**:

.. code-block:: quirrel

   {
     size = flex()
     flow = FLOW_HORIZONTAL
     gap = hdpx(10)
     children = [
       { size = flex(), /* column 1 */ }
       { size = flex(), /* column 2 */ }
       { size = flex(), /* column 3 */ }
     ]
   }

**Weighted columns**:

.. code-block:: quirrel

   {
     flow = FLOW_HORIZONTAL
     children = [
       { size = const [flex(1), flex()], /* 1/3 width */ }
       { size = const [flex(2), flex()], /* 2/3 width */ }
     ]
   }

**Minimum content width with flex**:

.. code-block:: quirrel

   {
     size = FLEX_H
     minWidth = SIZE_TO_CONTENT  // won't shrink below content size
     children = { rendObj = ROBJ_TEXT, text = "Don't shrink me" }
   }

**Overlay positioning**:

.. code-block:: quirrel

   {
     size = flex()
     children = [
       baseContent
       {
         hplace = ALIGN_RIGHT
         vplace = ALIGN_TOP
         pos = [-hdpx(10), hdpx(10)]  // inset from corner
         children = badge
       }
     ]
   }


Element identity and keys
-------------------------

The ``key`` property provides stable element identity across rebuilds:

.. code-block:: quirrel

   {
     flow = FLOW_VERTICAL
     children = items.map(@(item) {
       key = item.id          // stable identity for animations and state
       // ...
     })
   }

Without ``key``, daRg matches elements by position and structure. When list
items can be reordered, added, or removed, ``key`` ensures animations and
internal state follow the correct elements.


Zero-size positioning anchors
-----------------------------

Elements with ``size = 0`` can serve as positioning anchors for their children.
The anchor itself occupies no space in layout but provides a reference point
for child positioning:

.. code-block:: quirrel

   {
     size = 0
     pos = const [sw(50), sh(50)]       // position at screen center
     halign = ALIGN_CENTER
     valign = ALIGN_CENTER
     children = content           // content centered on anchor point
   }

This pattern is useful for:

* Placing elements at calculated coordinates (e.g., circular layouts)
* Creating overlay markers at specific positions
* Positioning tooltips or popups relative to a point

.. code-block:: quirrel

   // Circular layout example
   let radius = hdpx(100)
   let angle = PI / 4
   {
     size = 0
     pos = [cos(angle) * radius, sin(angle) * radius]
     halign = ALIGN_CENTER
     valign = ALIGN_CENTER
     children = itemAtAngle
   }


Manually calculating component sizes
------------------------------------

Use ``calc_comp_size(component)`` to manually calculate component dimensions.
It returns an array of two numbers: [width, height]
It is slow (the function creates a separate scene for the given components
and calculates its layout) and should no be used in real time.

Use ``calc_str_box()`` function to calculate the size of rendered text string.
It can take two forms ``calc_str_box(text, [params])`` or ``calc_str_box(params)``.
``text`` is a string.
``params`` is a table or a function returning a table (can be a component desciption).
If ``text`` is not specified it is read from ``params`` table.
From ``params`` table the following fields are read:
``fontId``, ``fontSize``, ``fontHt``, ``spacing``, ``monoWidth``, ``padding``
(all optional).


Debugging Tips
--------------

* Use ``ROBJ_FRAME`` with ``borderWidth`` to visualize element boundaries
* ``ROBJ_DEBUG`` renders a box with diagonal lines for debugging
* ``darg.dbgframe <mode>`` console commands sets debug rendering of element frames.
  ``mode`` values:

   * ``0`` - debug render off
   * ``1`` or ``create`` - elements are assigned new random color on create
   * ``2`` or ``update`` - elements are assigned new random color on rebuild
* Remember that ``SIZE_TO_CONTENT`` requires children to have calculable sizes

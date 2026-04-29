.. _tutorial_dasStbImage_drawing_and_blending:

==========================================
STBIMAGE-05 — Drawing and Alpha Blending
==========================================

.. index::
    single: Tutorial; fill_rect
    single: Tutorial; blit_alpha
    single: Tutorial; Alpha Blending
    single: Tutorial; dasStbImage

This tutorial covers drawing primitives and alpha blending on images:
filling rectangles with solid colors, and blending a 1-channel alpha
source onto an RGBA destination.

Filling Rectangles (RGBA)
=========================

``Image.fill_rect(x, y, w, h, color)`` fills a rectangle with a packed
RGBA ``uint`` color value.  The image must be 4-channel uint8.
Coordinates that extend beyond the image bounds are clipped automatically:

.. code-block:: das

   var img = make_image(64, 64, 4)
   // Pack RGBA into a single uint: R in low byte, A in high byte
   let red = uint(255) | (uint(60) << 8u) | (uint(60) << 16u) | (uint(255) << 24u)
   img.fill_rect(0, 0, 64, 64, red)
   // Partial fill — clipped to bounds
   img.fill_rect(50, 50, 30, 30, 0xFF00FF00u)

Filling Rectangles (Greyscale)
==============================

For 1-channel uint8 images, ``fill_rect`` takes a ``uint8`` value:

.. code-block:: das

   var grey = make_image(32, 32, 1)
   grey.fill_rect(0, 0, 32, 32, uint8(40))    // dark background
   grey.fill_rect(4, 4, 24, 24, uint8(200))   // bright square

Alpha Blending
==============

``Image.blit_alpha(src, sx, sy, dx, dy, w, h, r, g, b)`` blends a
1-channel alpha source onto a 4-channel RGBA destination.  Each source
pixel is the alpha value for blending the color ``(r, g, b)`` onto the
destination.

The blend formula per channel is::

   out = (alpha * color + (255 - alpha) * dest + 128) / 255

Alpha channel of the destination is updated as
``min(dest_alpha + src_alpha, 255)``.

This is the fundamental building block for software text rendering:
font atlases are 1-channel images where each pixel stores glyph coverage.

.. code-block:: das

   var dst = make_image(64, 64, 4)
   dst.fill_rect(0, 0, 64, 64, 0xFFB4B4B4u)  // light grey background
   // Create a 1-channel alpha source (gradient)
   var src = make_image(32, 32, 1)
   for (y in range(32)) {
       for (x in range(32)) {
           src.bytes[y * 32 + x] = uint8(x * 255 / 31)
       }
   }
   // Blend in red — left side transparent, right side opaque
   dst.blit_alpha(src, 0, 0, 16, 16, 32, 32, 255, 0, 0)

Both source and destination regions are clipped to their respective image
bounds, so out-of-range coordinates are safe.

Compositing
===========

Combining ``fill_rect`` and ``blit_alpha`` to build up a layered image
is a common pattern in UI rendering:

.. code-block:: das

   var canvas = make_image(64, 48, 4)
   // Background
   canvas.fill_rect(0, 0, 64, 48, pack_rgba(30, 30, 50, 255))
   // Button background
   canvas.fill_rect(8, 12, 48, 24, pack_rgba(60, 120, 200, 255))
   // Border (1px lines)
   let border = pack_rgba(200, 220, 255, 255)
   canvas.fill_rect(8, 12, 48, 1, border)   // top
   canvas.fill_rect(8, 35, 48, 1, border)   // bottom
   canvas.fill_rect(8, 12, 1, 24, border)   // left
   canvas.fill_rect(55, 12, 1, 24, border)  // right
   // Alpha-blended icon
   canvas.blit_alpha(icon, 0, 0, 14, 18, 12, 12, 255, 255, 255)

.. seealso::

   Full source: :download:`tutorials/dasStbImage/05_drawing_and_blending.das <../../../../tutorials/dasStbImage/05_drawing_and_blending.das>`

   Previous tutorial: :ref:`tutorial_dasStbImage_pixel_access`

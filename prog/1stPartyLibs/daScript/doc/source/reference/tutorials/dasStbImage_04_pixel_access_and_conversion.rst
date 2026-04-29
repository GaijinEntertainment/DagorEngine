.. _tutorial_dasStbImage_pixel_access:

==================================================
STBIMAGE-04 — Pixel Access and Format Conversion
==================================================

.. index::
    single: Tutorial; Pixel Access
    single: Tutorial; Image Conversion
    single: Tutorial; with_pixels
    single: Tutorial; to_channels
    single: Tutorial; to_bpc
    single: Tutorial; dasStbImage

This tutorial covers reading and modifying pixel data, row-level access,
and converting between channel counts and bit depths.

Reading Pixels
==============

``with_pixels()`` gives a temporary mutable array view of pixel data.
Since there are three overloads (uint8, float, uint16), you must specify
the block parameter type explicitly:

.. code-block:: das

   img |> with_pixels() <| $(var pixels : array<uint8>#) {
       let r = int(pixels[0])
       let g = int(pixels[1])
       print("R={r} G={g}\n")
   }

For float and 16-bit access, use ``with_pixels_f()`` and
``with_pixels_16()`` — these require the image to have the matching
bpc (4 for float, 2 for uint16).

Row-level Access
================

``with_row(y, block)`` gives access to a single row — more efficient
when you only need one row at a time:

.. code-block:: das

   img |> with_row(4) <| $(var row : array<uint8>#) {
       print("Row 4 has {length(row)} bytes\n")
   }

``with_row_f()`` provides float access (requires bpc=4).
``with_row_16()`` provides uint16 access (requires bpc=2).

Modifying Pixels
================

The pixel arrays are mutable — changes are written directly back to the
image data:

.. code-block:: das

   img |> with_pixels() <| $(var pixels : array<uint8>#) {
       for (i in range(width * height)) {
           pixels[i * 4 + 0] = uint8(255 - int(pixels[i * 4 + 0]))
       }
   }

Channel Conversion
==================

``Image.to_channels(n)`` returns a new image with the specified number
of channels:

==========  ===========  ==========================================
From        To           Behavior
==========  ===========  ==========================================
4 (RGBA)    3 (RGB)      Drop alpha
3 (RGB)     4 (RGBA)     Add alpha = 255
3 or 4      1 (grey)     ITU-R BT.601 luminance
1 (grey)    3 or 4       Grey replicated to RGB(A)
1 or 4      2 (grey+A)   Add/keep alpha
2 (grey+A)  1 or 4       Extract/expand
==========  ===========  ==========================================

.. code-block:: das

   var inscope rgb <- rgba_img.to_channels(3)      // drop alpha
   var inscope grey <- rgb.to_channels(1)          // to greyscale
   var inscope back <- grey.to_channels(4)         // grey → RGBA

BPC Conversion
==============

``Image.to_bpc(n)`` converts bytes-per-component:

=====  ========  =======================
bpc    Type      Range
=====  ========  =======================
1      uint8     0–255
2      uint16    0–65535
4      float     0.0–1.0
=====  ========  =======================

Conversions preserve visual values: ``uint8(255)`` → ``uint16(65535)`` →
``float(1.0)``:

.. code-block:: das

   var inscope hdr <- img.to_bpc(4)        // uint8 → float
   var inscope img16 <- hdr.to_bpc(2)      // float → uint16
   var inscope back <- img16.to_bpc(1)     // uint16 → uint8

Combined Conversions
====================

Channel and BPC conversions can be chained.  Convert BPC first for
higher precision, then channels:

.. code-block:: das

   // RGBA uint8 → RGB float (HDR pipeline input)
   var inscope hdr_rgb <- img.to_bpc(4).to_channels(3)

   // Greyscale uint16 (scientific imaging)
   var inscope grey16 <- img.to_channels(1).to_bpc(2)

.. seealso::

   Full source: :download:`tutorials/dasStbImage/04_pixel_access_and_conversion.das <../../../../tutorials/dasStbImage/04_pixel_access_and_conversion.das>`

   Next tutorial: :ref:`tutorial_dasStbImage_drawing_and_blending`

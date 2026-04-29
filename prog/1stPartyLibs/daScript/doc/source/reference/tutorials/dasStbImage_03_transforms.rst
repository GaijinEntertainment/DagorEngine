.. _tutorial_dasStbImage_transforms:

===================================
STBIMAGE-03 — Image Transforms
===================================

.. index::
    single: Tutorial; Image Resize
    single: Tutorial; Image Flip
    single: Tutorial; Image Crop
    single: Tutorial; Image Blit
    single: Tutorial; dasStbImage

This tutorial covers resizing, flipping, cropping, and compositing images.

Resizing
========

``Image.resize(w, h)`` returns a new resized image using the default
filter.  You can also specify a filter from ``stbir_filter``:

.. code-block:: das

   var inscope small <- img.resize(16, 16)
   var inscope sharp <- img.resize(128, 128, stbir_filter.STBIR_FILTER_POINT_SAMPLE)
   var inscope smooth <- img.resize(128, 128, stbir_filter.STBIR_FILTER_MITCHELL)

Available filters: ``STBIR_FILTER_DEFAULT``, ``STBIR_FILTER_BOX``,
``STBIR_FILTER_TRIANGLE``, ``STBIR_FILTER_CUBICBSPLINE``,
``STBIR_FILTER_CATMULLROM``, ``STBIR_FILTER_MITCHELL``,
``STBIR_FILTER_POINT_SAMPLE``.

Flipping
========

``flip_vertical()`` and ``flip_horizontal()`` return new flipped images.
The original is not modified:

.. code-block:: das

   var inscope flipped_v <- img.flip_vertical()
   var inscope flipped_h <- img.flip_horizontal()

Cropping
========

``Image.crop(x, y, w, h)`` extracts a rectangular sub-region.
Coordinates are clamped to image bounds:

.. code-block:: das

   var inscope cropped <- img.crop(24, 24, 16, 16)
   // Requesting beyond bounds is safe — the result is clamped
   var inscope clamped <- img.crop(50, 50, 32, 32)

Blitting
========

``Image.blit(source, x, y)`` copies source pixels onto the image at
position (x, y).  Both images must have the same number of channels.
Pixels outside the destination bounds are clipped:

.. code-block:: das

   var inscope canvas <- make_solid(64, 64, ...)
   var inscope overlay <- make_solid(16, 16, ...)
   canvas.blit(overlay, 8, 8)

Creating Blank Canvases
=======================

``make_image(width, height, channels, bpc)`` creates a zero-filled image.
``bpc`` defaults to 1 (uint8):

.. code-block:: das

   var rgba = make_image(100, 100, 4)       // uint8 RGBA
   var hdr = make_image(100, 100, 3, 4)     // float RGB
   var grey = make_image(100, 100, 1)       // uint8 greyscale
   var img16 = make_image(100, 100, 4, 2)   // uint16 RGBA

.. seealso::

   Full source: :download:`tutorials/dasStbImage/03_transforms.das <../../../../tutorials/dasStbImage/03_transforms.das>`

   Next tutorial: :ref:`tutorial_dasStbImage_pixel_access`

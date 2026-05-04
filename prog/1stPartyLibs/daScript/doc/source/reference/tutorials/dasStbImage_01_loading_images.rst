.. _tutorial_dasStbImage_loading_images:

===========================================
STBIMAGE-01 — Loading and Inspecting Images
===========================================

.. index::
    single: Tutorial; Images
    single: Tutorial; Image Loading
    single: Tutorial; dasStbImage
    single: Tutorial; stbimage_boost

This tutorial covers loading images from files, querying image properties,
and format detection using the ``stbimage_boost`` module.

Setup
=====

Import the ``stbimage_boost`` module::

   require stbimage/stbimage_boost

Loading with Error Handling
===========================

``Image.load()`` returns a ``tuple<bool; string>``.  On success the first
element is ``true`` and the error string is empty:

.. code-block:: das

   var inscope img : Image
   let (ok, error) = img.load("photo.png")
   if (!ok) {
       print("Failed: {error}\n")
       return
   }
   print("{img.width}x{img.height}, {img.channels} channels\n")

Image Properties
================

Once loaded, an ``Image`` exposes several query methods:

============================  =============================================
Method                        Description
============================  =============================================
``valid()``                   ``true`` if width > 0 and height > 0
``stride()``                  Bytes per row (width * channels * bpc)
``has_alpha()``               ``true`` if channels is 2 or 4
``pixel_size()``              Bytes per pixel (channels * bpc)
``is_hdr()``                  ``true`` if bpc == 4 (float data)
``data_size()``               Total bytes of pixel data
============================  =============================================

HDR and 16-bit Loading
======================

``load_hdr()`` loads float data (bpc=4).  ``load_16()`` loads 16-bit data
(bpc=2).  Both follow the same ``tuple<bool; string>`` error pattern:

.. code-block:: das

   var inscope img16 : Image
   let (ok, err) = img16.load_16("photo.png")

Image Info Without Loading
==========================

``image_info()`` reads just the header — much faster than loading pixel data:

.. code-block:: das

   let info = image_info("photo.png")
   print("{info.w}x{info.h}, {info.comp} channels\n")

Convenience Loading
===================

``load_image()`` is a one-liner that panics on failure — good for scripts:

.. code-block:: das

   var inscope img <- load_image("photo.png")

A variant with an error out-parameter is also available:

.. code-block:: das

   var error : string
   var inscope img <- load_image("photo.png", error)

Format Detection
================

``is_hdr()`` and ``is_16_bit()`` check file format without loading pixels.
Memory variants (``is_hdr_from_memory``, ``is_16_bit_from_memory``) also
exist:

.. code-block:: das

   print("HDR: {is_hdr(path)}, 16-bit: {is_16_bit(path)}\n")

.. seealso::

   Full source: :download:`tutorials/dasStbImage/01_loading_images.das <../../../../tutorials/dasStbImage/01_loading_images.das>`

   Next tutorial: :ref:`tutorial_dasStbImage_saving_and_encoding`

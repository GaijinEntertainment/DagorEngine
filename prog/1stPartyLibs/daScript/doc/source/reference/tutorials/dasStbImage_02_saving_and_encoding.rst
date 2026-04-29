.. _tutorial_dasStbImage_saving_and_encoding:

=============================================
STBIMAGE-02 — Saving and Encoding Images
=============================================

.. index::
    single: Tutorial; Image Saving
    single: Tutorial; Image Encoding
    single: Tutorial; JPEG Quality
    single: Tutorial; dasStbImage

This tutorial covers saving images to files, encoding to in-memory byte
arrays, loading from memory, and round-trip verification.

Saving to File
==============

``Image.save(path, quality)`` saves to file.  The format is determined by
the file extension (``.png``, ``.bmp``, ``.tga``, ``.jpg``, ``.hdr``).
The quality parameter only matters for JPEG (1–100, default 90):

.. code-block:: das

   let (ok, error) = img.save("output.png")
   let (ok2, _) = img.save("output.jpg", 75)  // JPEG quality 75

JPEG Quality
============

Lower quality values produce smaller files with more compression artifacts:

.. code-block:: das

   for (q in fixed_array(10, 50, 90)) {
       var buf : array<uint8>
       img.encode("jpg", buf, q)
       print("Quality {q}: {length(buf)} bytes\n")
   }

Encoding to Memory
==================

``Image.encode(format, buf, quality)`` writes compressed image data into a
byte array.  The format is specified without a dot: ``"png"``, ``"bmp"``,
``"tga"``, ``"jpg"``:

.. code-block:: das

   var buf : array<uint8>
   let (ok, error) = img.encode("png", buf)
   print("Encoded {length(buf)} bytes\n")

Loading from Memory
===================

``Image.load_from_memory(buf)`` decodes an image from a byte array.
Also available: ``load_hdr_from_memory()``, ``load_16_from_memory()``:

.. code-block:: das

   var inscope img2 : Image
   let (ok, error) = img2.load_from_memory(png_buf)

Format detection works from memory too:

.. code-block:: das

   print("HDR: {is_hdr_from_memory(buf)}\n")
   print("16-bit: {is_16_bit_from_memory(buf)}\n")

Round-trip Verification
=======================

Lossless formats (PNG, BMP, TGA) preserve pixel values exactly through
an encode/decode cycle:

.. code-block:: das

   var buf : array<uint8>
   original.encode("png", buf)
   var inscope decoded : Image
   decoded.load_from_memory(buf)
   // decoded pixels == original pixels

.. seealso::

   Full source: :download:`tutorials/dasStbImage/02_saving_and_encoding.das <../../../../tutorials/dasStbImage/02_saving_and_encoding.das>`

   Next tutorial: :ref:`tutorial_dasStbImage_transforms`

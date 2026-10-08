Compute Shaders
=================================================

Overview
--------

The compute shader API provides two classes for dispatching GPU compute work:

- :cpp:class:`ComputeShaderElement` -- low-level compute shader element with full control
  over parameter binding, dispatch modes, and GPU pipeline selection.
- :cpp:class:`ComputeShader` -- lightweight RAII wrapper for the common case of
  creating a shader by name and dispatching work.

.. warning::

   Avoid using :cpp:class:`ComputeShaderElement` directly. It returns a raw
   pointer from :cpp:func:`new_compute_shader` that must be manually deleted,
   making it easy to leak memory. Prefer :cpp:class:`ComputeShader` which
   manages lifetime automatically via ``eastl::unique_ptr``.

Usage
-----

Creating a compute shader
~~~~~~~~~~~~~~~~~~~~~~~~~

Use the :cpp:class:`ComputeShader` RAII wrapper:

.. code-block:: cpp

   #include <shaders/dag_computeShaders.h>

   ComputeShader cs("my_compute_shader");
   if (cs)
     cs.dispatchThreads(width, height, 1);

Setting parameters
~~~~~~~~~~~~~~~~~~

Shader parameters are set via variable IDs obtained from
:cpp:func:`get_shader_variable_id`:

.. code-block:: cpp

   int varId = get_shader_variable_id("some_param");
   cs->set_color4_param(varId, Color4(1, 0, 0, 1));
   cs->set_texture_param(texVarId, textureId);

Dispatching work
~~~~~~~~~~~~~~~~

There are three dispatch modes:

- **By thread groups** -- :cpp:func:`ComputeShaderElement::dispatch` dispatches
  the specified number of thread groups directly.
- **By total threads** -- :cpp:func:`ComputeShaderElement::dispatchThreads`
  takes the desired total thread count and automatically divides by the shader's
  thread group sizes (rounding up).
- **Indirect** -- :cpp:func:`ComputeShaderElement::dispatch_indirect` reads
  dispatch arguments from a GPU buffer. The buffer must contain 3 ``uint32_t``
  values (``groupCountX``, ``groupCountY``, ``groupCountZ``) at the given byte
  offset. The buffer must be created with ``SBCF_BIND_UNORDERED`` and
  ``SBCF_MISC_DRAWINDIRECT`` flags (e.g. via ``d3d::buffers::create_ua_indirect``
  with ``Indirect::Dispatch``). When storing multiple dispatch entries in a single
  buffer, the byte offset should be a multiple of ``DISPATCH_INDIRECT_BUFFER_SIZE``
  (12 bytes).

.. code-block:: cpp

   // Dispatch by thread groups
   cs->dispatch(groupsX, groupsY, groupsZ);

   // Dispatch by total thread count (auto-divides by thread group size)
   cs->dispatchThreads(totalX, totalY, totalZ);

   // Indirect dispatch from a GPU buffer
   cs->dispatch_indirect(argsBuffer, byteOffset);

API Reference
-------------

.. doxygenfile:: dag_computeShaders.h
  :project: shaders

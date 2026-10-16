.. _render-passes:

=============
Render passes
=============

A render pass declaration describes the structure of a pass: its targets and the role of each target.
Shaders opt into a declared pass and name its targets in their output struct. The compiler derives
every platform number (color slot, Vulkan input attachment index, Metal color index, emulated ``T``
register, color export format) from the declaration, so shader authors write only names.

Most passes need only the syntax in :ref:`simple-render-passes`. Each section after it adds one
capability: shaders that serve several passes, passes that run with several format sets, MSAA,
subpasses with pixel-local reads for tile-based GPUs, and shading-rate attachments.
:ref:`render-pass-general-rules` holds the rules that apply to all declarations.

.. _simple-render-passes:

Simple render passes
--------------------

A pass is declared with ``render_pass``. Its ``color()`` and ``depth()`` lists declare the targets in
order of first appearance, and each entry carries its format:

.. code-block:: text

    render_pass forward
    {
      color(scene = argb16f, velocity = rg16f);
      depth(scene_depth = d32);
    }

- ``color(A, B, ...)`` lists the color targets, at most 8. The list order is the color slot order,
  except on Metal, where the slot is the target index; write the ``color()`` list first so the two
  agree. A color target carries a color format.
- ``depth(T)`` binds the depth target, which carries a depth format.
- ``sampled_depth(T)`` binds the depth target read-only while shaders sample it as a texture, for
  example soft particles that read the scene depth. No part of the pass writes such a target.

Formats: ``argb8``, ``rgba8``, ``abgr8``, ``argb8_srgb``, ``rgba8_srgb``, ``abgr8_srgb``,
``a2rgb10``, ``a2bgr10``, ``r11g11b10f``, ``argb16``, ``argb16s``, ``argb16f``, ``argb16ui``,
``argb32f``, ``argb32ui``, ``a1rgb5``, ``a4rgb4``, ``rgb565``, ``rg8``, ``rg8s``, ``rg16``,
``rg16f``, ``rg32f``, ``rg32ui``, ``a8``, ``l16``, ``r8``, ``r8ui``, ``r16f``, ``r16ui``, ``r32f``,
``r32ui``, ``r32si``, ``d16``, ``d24``, ``d32``, ``d32s8``.

Shader opt-in
~~~~~~~~~~~~~

A shader opts into a pass with a ``pass`` statement, at most one per variant:

.. code-block:: text

    shader forward_opaque
    {
      pass forward;
      ...
    }

The name resolves against the declarations above the shader, in the same unit or in an earlier
include, so declare the pass before the shaders that opt into it. The one subpass of a ``render_pass``
is named ``main``: ``pass forward.main;`` is the same opt-in, and the compiler reports the subpass as
``forward.main``. The statement belongs to a shader; a ``block`` cannot declare it. The opt-in adds no variant axis (see :ref:`tbdr-render-passes` for the
one exception). The dump stores the opt-in, and in dev builds the runtime checks it against the active
pass at draw.

Output struct
~~~~~~~~~~~~~

A shader that writes color declares its pixel shader output struct in the ``(ps)`` block, with target
names as semantics. The compiler resolves each field to its platform slot and puts the plain HLSL
struct in front of the shader source:

.. code-block:: text

    (ps) {
      FORWARD_OUTPUT@ps_output
      {
        float4 color : scene;
        float2 motion : velocity;
      }
    }
    hlsl(ps) {
      FORWARD_OUTPUT main_ps(VsOutput input)
      {
        FORWARD_OUTPUT ret;
        ret.color = ...;
        ret.motion = ...;
        return ret;
      }
    }

The struct is resolved per variant against that variant's opt-in. A variant that does not opt in
writes a hand-written struct in its ``hlsl(ps)`` block instead, and a ``block`` cannot declare one.

- Explicit ``SV_TargetN`` semantics are rejected. ``SV_Depth`` and other system values pass through.
  An ``SV_Coverage`` field does not combine with ``alpha_to_coverage``: D3D drops the state while
  Vulkan and Metal combine both masks.
- The compiled output signature must equal the declared struct. A field's HLSL class (float, int or
  uint) and component count are checked against the target format.
- On embedded platforms that ship GPU ISA in the shader bindump, the export format of each color
  output (the data format of the pixel shader export instructions, see the `AMD RDNA 3.5 ISA reference
  <https://docs.amd.com/v/u/en-US/rdna35_instruction_set_architecture>`_) is fixed when the shader is
  compiled. The compiler derives it from the target formats, so an opted-in variant sets no export
  format by hand.

For dual-source blending, the ``src1`` modifier makes a field the second blend source of a target:
``float4 blend : src1 scene;`` beside the plain field for ``scene``. The pass must bind exactly one
color target, and ``src1`` does not combine with ``optional``. The second source writes color slot 0
only; on Metal the slot is the target index, so declare that target before every other target of the
pass.

Depth and render state
~~~~~~~~~~~~~~~~~~~~~~

The compiler checks the static render state of an opted-in variant against the pass:

- A shader that writes ``SV_Depth`` needs a writable ``depth()`` target.
- A read-only depth binding (``sampled_depth``, or the read-only depth of
  :ref:`tbdr-render-passes`) needs ``z_write = false``.
- ``z_write`` on a pass with no depth target is a warning.
- Dual-source blending (a blend factor ``sc1``, ``isc1``, ``sa1`` or ``isa1``) needs a ``src1`` field.
- A ``src1`` field that no blend factor reads is a warning.

.. _pass-polymorphic-shaders:

Pass-polymorphic shaders
------------------------

One compiled variant can serve several passes. A variant opts into a set of passes with ``any_of``:

.. code-block:: text

    render_pass forward_lowres
    {
      color(scene = argb16f);
      depth(scene_depth = d32);
    }

    shader forward_opaque
    {
      pass any_of(forward, forward_lowres);
      ...
    }

A set that several shaders share is declared once as a subpass group:

.. code-block:: text

    subpass forward_any = any_of(forward, forward_lowres);

    shader forward_opaque
    {
      pass forward_any;
      ...
    }

A member is a pass with one subpass (``forward``, or ``forward.main``), a ``PASS.SUBPASS`` of a pass
with several (see :ref:`tbdr-render-passes`), or another group, each declared before the group.

A target that only some members bind needs ``optional`` on its output struct field. The field is
dropped in variants whose pass does not bind the target, and ``RP_OUTPUT_HAS(FIELD)`` from
``hardware_defines.dshl`` is 1 where the field exists:

.. code-block:: text

    (ps) {
      FORWARD_OUTPUT@ps_output
      {
        float4 color : scene;
        float2 motion : optional velocity;
      }
    }
    hlsl(ps) {
      FORWARD_OUTPUT main_ps(VsOutput input)
      {
        FORWARD_OUTPUT ret;
        ret.color = ...;
        #if RP_OUTPUT_HAS(motion)
        ret.motion = ...;
        #endif
        return ret;
      }
    }

The name after ``optional`` must be a target that some pass declared above the shader declares, in any
branch of its body. The per-field leaves are emitted with the pixel shader source alone, so in a
variant that declares no ``(ps)`` output struct, and in every other stage, ``RP_OUTPUT_HAS`` is a
preprocessor error. In a group variant the field stays where the group contract has a slot for the
target, so ``RP_OUTPUT_HAS(FIELD)`` is 1 there too; the members that do not bind the target discard
the write.

The group is validated where it is declared:

- One slot cannot carry two target names across the members, and the members must agree on the
  depth binding kind (writable, read-only, ``sampled_depth``).
- Every color target of the group contract has one HLSL class (float, int or uint) across the members
  and across the supported layouts of each member, because one compiled field has one class.
- A target that members bind at different slots stays legal, but leaves the group contract. An output
  struct field that names it is an error; a shader that writes it opts into a concrete pass instead.

A group variant is accepted in any member and with no pass active. So is a variant that opts into a
pass with one subpass and no reads: it writes the same slots on plain render targets.

.. _format-polymorphic-passes:

Format-polymorphic passes
-------------------------

A pass whose targets take a format that the game selects at run time, for example an HDR scene format
that depends on the device, names framebuffer layouts instead of inline formats. A
``framebuffer_layout`` gives each target name a format, and ``any_of`` names a set of layouts:

.. code-block:: text

    framebuffer_layout forward_fb_hdr16
    {
      scene = argb16f;
      velocity = rg16f;
      scene_depth = d32;
    }

    framebuffer_layout forward_fb_hdr11
    {
      scene = r11g11b10f;
      velocity = rg16f;
      scene_depth = d32;
    }

    framebuffer_layout forward_fb = any_of(forward_fb_hdr16, forward_fb_hdr11);

    render_pass forward
    {
      framebuffer(forward_fb);
      color(scene, velocity);
      depth(scene_depth);
    }

- ``framebuffer(LAYOUT, ...)`` names the layouts the pass supports. A pass declares its formats in
  exactly one of two forms: a ``framebuffer()`` statement, or a format on every target. A pass that
  has both is an error.
- A layout is known once its declaration precedes the pass, so declare layouts first.
- Every pass target must be an entry of every supported layout. A layout entry that the pass does not
  bind is legal.
- A layout body accepts ``if (hardware.X) { ... } else { ... }`` like a pass body.

One shape then serves every format set, and one shader variant serves every layout: the compiler
checks each output field against the target format in every supported layout. The runtime creates a
pass instance for one of the supported layouts.

.. _msaa-render-passes:

MSAA
----

A sample count is written ``xN`` after a format, inline or in a framebuffer layout. The sample count
is a layout fact and never reaches a shader, so MSAA adds no shader variant.

In a simple pass, ``resolve(DST)`` on an entry declares a multisampled target and its resolve
destination together:

.. code-block:: text

    render_pass forward_msaa
    {
      color(scene_ms = argb8 x4 resolve(scene = argb8));
      depth(depth_ms = d24 x4 resolve(depth = d24));
    }

- ``resolve(DST)`` resolves the target into ``DST`` at the end of the pass. ``DST`` is a
  single-sampled target with no other role, and it carries the format of the source in every
  supported layout.
- The color targets and the depth target of one pass (one subpass, in :ref:`tbdr-render-passes`)
  have the same sample count in every supported layout.
- In a pass with ``target`` statements, ``multisampled target NAME;`` declares a target that is bound
  with more than one sample. A layout entry with more than one sample requires the qualifier, and a
  qualified target needs more than one sample in every supported layout. There ``resolve(DST)`` is
  written on the ``target`` statement and ``DST`` is declared by its own ``target`` statement.
- A multisampled target is read per sample with ``pixelLocalLoadSample`` (see
  :ref:`tbdr-render-passes`).

.. _tbdr-render-passes:

TBDR devices
------------

On tile-based GPUs a later subpass can read what an earlier subpass of the same pass wrote, from tile
memory and without a trip through main memory. A pass with several subpasses is declared with
``multi_render_pass``:

.. code-block:: text

    multi_render_pass deferred
    {
      allow_split;
      framebuffer(deferred_fb);

      target gbuf0;
      target gbuf1;
      target result;
      target zbuf;
      if (hardware.metal) { target zbuf_value; }

      subpass fill
      {
        color(gbuf0, gbuf1);
        if (hardware.metal) { color(zbuf_value); }
        depth(zbuf);
      }
      subpass resolve
      {
        color(result);
        depth(zbuf);
        if (hardware.metal) { read(gbuf0, gbuf1, zbuf_value); }
        else { read(gbuf0, gbuf1, zbuf); }
      }
    }

Metal fetches color attachments only, so the shape above writes the depth value to a color target
there and reads that target instead of the depth.

Statements of a ``multi_render_pass`` body:

- ``target NAME;`` declares a target. ``target NAME = FORMAT [xN] [resolve(DST)];`` also gives the
  format, the sample count and a resolve destination, in place of a ``framebuffer()`` statement.
- ``framebuffer(LAYOUT, ...);`` and ``allow_split;`` apply to the whole pass.
- ``subpass NAME { ... }`` declares a subpass. Its statements are ``color(...)``, ``depth(T)``,
  ``sampled_depth(T)``, ``read(...)`` and ``vrs(T)``, and they name targets only.
- ``if (hardware.X) { ... } else { ... }`` selects statements per target platform.

A ``subpass`` block under ``render_pass``, or a ``multi_render_pass`` without one, is an error that
names the other keyword.

Subpass roles, beyond those of :ref:`simple-render-passes`:

- ``read(A, B, ...)`` lists the input attachments, at most 8 per subpass. The list order is the
  attachment ordinal order. A subpass cannot read a target it writes as color.
- A ``depth()`` target that is also in the subpass ``read()`` list binds read-only, and the shaders
  drawn in that subpass must set ``z_write = false``. Metal cannot read a depth target; write the depth
  value to a color target there.

On Metal the color slot is the target declaration index, not the ``color()`` list position. Declare
the targets that every shape shares first, so a target added in a ``hardware`` branch does not move the
others. A read target is declared among the first 8 targets, because Metal fetches the input
attachment by target index.

A shader opts into one subpass with ``pass PASS.SUBPASS;``. A subpass group can name subpasses of
several passes: ``subpass gbuffer_writers = any_of(deferred.fill, deferred_msaa.fill);``.

Subpass reads
~~~~~~~~~~~~~

A ``@subpass_read`` binding in the ``(ps)`` block reads an attachment of the opted-in subpass.
``pixelLocalLoad`` and ``pixelLocalLoadSample`` from ``hardware_defines.dshl`` read it:

.. code-block:: text

    (ps) {
      g0@subpass_read = gbuf0;             // a target of the opted-in pass
      g1@subpass_read = deferred.gbuf1;    // the same, with the pass named
      zd@subpass_read = zbuf_ms;           // declared `multisampled target zbuf_ms;`
    }
    hlsl(ps) {
      float4 a = pixelLocalLoad(g0, input.tc);
      float4 b = pixelLocalLoad(g1, input.tc);
      float d = pixelLocalLoadSample(zd, input.tc, sampleIndex).r;
    }

A ``block`` cannot declare the binding, and it cannot be an array. It compiles to an input attachment
where subpasses are native (Vulkan, iOS Metal) and to an ordinary pixel shader texture elsewhere, at a
register the render pass binds. The read is a float read on every backend, so the target carries a
float-class format (float, unorm, snorm or a depth format) in every supported layout; a read of a
``r32ui`` or ``r32si`` target is an error that names the layout.

In a subpass group the read lists share one common prefix, compared by target name, by target index
and by the ``multisampled`` qualifier. A group variant reads only that prefix, and the members of a
reading group must agree on ``allow_split``.

Split execution
~~~~~~~~~~~~~~~

``allow_split;`` permits execution without native subpasses: one ``set_render_target`` per subpass,
with the reads bound as ordinary textures. The runtime picks this mode on a device whose native
subpass reads cannot be trusted. On a backend with native subpasses, a variant that opts into a
subpass of an ``allow_split`` pass whose read list is not empty gains the ``render_pass_split_mode``
interval as a variant dimension, whether or not the shader itself declares a read, because both
lowerings of the reads must exist. This is the one opt-in that adds a variant axis.

.. _vrs-render-passes:

Variable rate shading
---------------------

``vrs(T)`` names the shading rate attachment of a pass (or of a subpass), an ``r8ui`` target:

.. code-block:: text

    render_pass forward_vrs
    {
      color(scene = argb16f);
      depth(scene_depth = d32);
      vrs(shading_rate = r8ui);
    }

The target has no other role and no shader sees it. On a device without shading rate attachments the
runtime ignores it and the pass renders at full rate. On Metal every subpass names the same ``vrs``
target or none, and the ``vrs`` target is declared after every color target.

.. _render-pass-general-rules:

General rules
-------------

- ``render_pass``, ``multi_render_pass``, ``framebuffer_layout``, ``subpass`` and ``pass`` are reserved
  words. A source that uses one of them as an identifier does not parse.
- Passes and subpass groups share one namespace, so a pass and a group cannot have the same name.
  Framebuffer layouts have a namespace of their own. The same declaration in two compilation units
  must match field for field, so declare each of them once in a shared include.
- A target name that begins with ``sv_`` is reserved for system values, and ``optional`` and ``src1``
  are reserved as output struct field modifiers.
- In ``if (hardware.X)`` conditions only ``hardware.*``, ``true`` and ``false`` are allowed.

.. _render-pass-runtime:

Runtime
-------

A declared shape becomes a live pass through ``ShaderRenderPass``
(``prog/dagorInclude/shaders/dag_shaderRenderPass.h``). ``find_id`` resolves a pass name from the loaded
dump, a ``Pass`` bakes one instance from the shape, one of its framebuffer layouts and the caller's
boundary plan (``Actions``: how each target's contents cross the pass boundary), and the instance is
driven with the targets bound by name (``Targets``) through ``begin``/``next_subpass``/``end``, the RAII
``Scope``, or the dev-only ``Marker`` for code that keeps plain ``set_render_target``. Where the device
cannot run the reads natively, an ``allow_split`` shape runs as one plain render target set per subpass
with the reads as textures. In dev builds every opted-in variant is checked against the active pass at
draw.

A pass makes no state transitions for its targets after it ends. The caller issues the barriers that
the next readers of a target need.

API reference
^^^^^^^^^^^^^

.. doxygenfile:: dag_shaderRenderPass.h
  :project: shaders

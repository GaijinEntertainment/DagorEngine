
.. _stdlib_apply_in_context:

================================
Cross-context evaluation helpers
================================

.. include:: detail/apply_in_context.rst

The apply_in_context module exposes single [apply_in_context] annotation.

All functions and symbols are in "apply_in_context" module, use require to get access to it. ::

    require daslib/apply_in_context


++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-apply_in_context-apply_in_context:

.. das:attribute:: apply_in_context

[apply_in_context] function annotation.
Function is modified, so that it is called in the debug agent context, specified in the annotation.
If specified context is not insalled, panic is called.

For example::
 [apply_in_context(opengl_cache)]
 def public cache_font(name:string implicit) : Font?
     ...
 ...
 let font = cache_font("Arial") // call invoked in the "opengl_cache" debug agent context



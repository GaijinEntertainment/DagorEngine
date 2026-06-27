.. _stream_output:

=============
Stream output
=============

Pragma ``stream_output`` can be used to define stream output layout for the shader.
It declares stream output slot, components for a certain semantic via the following syntax:

.. code-block:: c

    #pragma stream_output (slot) (semantic) (components)

A pipeline can stream output several semantics. Add several pragmas to define this.
Semantics order in the target buffer corresponds with the pragmas order.

Let's look at an example with stream output for SV_POSITION.xy and COLOR.yz to buffer bound to slot 0:

.. code-block:: c

    #pragma stream_output 0 SV_POSITION XY
    #pragma stream_output 0 COLOR YZ

.. warning::
    Stream output requires to ship DXIL on Xbox which will increase bindump size and pipelines creation time.
    Minimize amount of pipelines with stream output since it is very expensive on Xbox!

.. note::
    Only one stream output entry per semantic is allowed due to Xbox limitations.
    Stream output for every semantic must be defined with one pragma.
    Only consecutive components are supported for streamout.
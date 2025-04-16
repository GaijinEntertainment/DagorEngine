.. _global-constants:

****************
Global constants
****************

DSHL supports some built-in global constants, which you can grab in the preshader :ref:`preshader` and then use in ``hlsl`` blocks.

List of constants:

- ``globtm`` -- ``float4x4`` world-view-projection matrix
- ``projtm`` -- ``float4x4`` projection matrix
- ``viewprojtm`` -- ``float4x4`` view-projection matrix
- ``local_view_x, local_view_y, local_view_z, local_view_pos`` -- 1..4 columns of inverse view matrix, in ``float3`` format
- ``world_local_x, world_local_y, world_local_z, world_local_pos`` -- 1..4 columns of world transform matrix, in ``float3`` format

Usage example:

.. code-block:: c

  shader example_shader
  {
    // grabbing global constants via preshader
    (vs)
    {
      viewprojtm@f44 = viewprojtm;
      world_local_x@f3 = world_local_x;
      world_local_y@f3 = world_local_y;
      world_local_z@f3 = world_local_z;
      world_local_pos@f3 = world_local_pos;
    }

    hlsl(vs)
    {
      float4 example_vs(float4 pos: POSITION0)
      {
        // using global constants in hlsl
        float3 worldPos = float3(pos.x * world_local_x + pos.y * world_local_y + pos.z * world_local_z + world_local_pos);
        return mulPointTm(worldPos, viewprojtm);
      }
    }

  }

.. warning::
  Matrix constants ``globtm, projtm, viewprojtm`` are only available for ``(vs)`` preshader.

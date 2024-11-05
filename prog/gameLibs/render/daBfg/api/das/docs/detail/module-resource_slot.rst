Storing in :ref:`das+ecs <stdlib_resource_slot_ecs>` is more convenient.

However, we can store :ref:`NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` outside ecs with `daBfg.resource_slot` module.

Unfortunately, NodeHandleWithSlotsAccess can't be stored in local
das variable, because das doesn't call destructors for local types.
If you really need to store handle outside ecs, you have to store
handle in heap or make binding for storing it on cpp-side.

Also hot-reload will work automatically for nodes inside
:ref:`ecs <stdlib_resource_slot_ecs>`.

.. code-block:: das

  require daBfg.resource_slot

  some_cpp_binding <| $(var handle : NodeHandleWithSlotsAccess &)
    some_struct.handle <- root() |> registerAccess("node_name", [[SlotActions update <- [{auto Update("slot_name", "texture_name", 100)}] ]]) <| @(slots_state; var registry : Registry)

      return <- @ <|
        // some render code


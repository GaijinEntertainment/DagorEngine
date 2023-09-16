Access to component value by name. For example::

    create_entity <| @ ( eid, cmp )
        cmp.pos := float3(i)    // same as cmp |> set("pos",float3(i))

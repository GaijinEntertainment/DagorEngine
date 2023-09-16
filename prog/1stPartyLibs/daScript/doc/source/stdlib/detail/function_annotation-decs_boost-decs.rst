This macro converts a function into a DECS pass stage query. Possible arguments are `stage`, 'REQUIRE', and `REQUIRE_NOT`.
It has all other properties of a `query` (like ability to operate on templates). For example::

    [decs(stage=update_ai, REQUIRE=ai_turret)]
        def update_ai ( eid:EntityId; var turret:Turret; pos:float3 )
            ...

In the example above a query is added to the `update_ai` stage. The query also requires that each entity passed to it has an `ai_turret` property.

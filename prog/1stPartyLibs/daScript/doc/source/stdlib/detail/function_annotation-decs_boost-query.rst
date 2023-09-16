This macro implmenets 'query` functionality. There are 2 types of queries:
    * query(...) - returns a list of entities matching the query
    * query(eid) - returns a single entity matching the eid
For example::

    query() <| $ ( eid:EntityId; pos, vel : float3 )
        print("[{eid}] pos={pos} vel={vel}\n")

The query above will print all entities with position and velocity.
Here is another example::

    query(kaboom) <| $ ( var pos:float3&; vel:float3; col:uint=13u )
        pos += vel
The query above will add the velocity to the position of an entity with eid kaboom.

Query can have `REQUIRE` and `REQUIRE_NOT` clauses::

    var average : float3
    query <| $ [REQUIRE(tank)] ( pos:float3 )
        average += pos

The query above will add `pos` components of all entities, which also have a `tank` component.

Additionally queries can atuomaticall expand components of entities. For example::

    [decs_template(prefix="particle")]
    struct Particle
        pos, vel : float3
    ...
    query <| $ ( var q : Particle )
        q.pos += q.vel                  // this is actually particlepos += particlevel


In the example above structure q : Particle does not exist as a variable. Instead it is expanded into accessing individual componentes of the entity.
REQURE section of the query is automatically filled with all components of the template.
If template prefix is not specified, prefix is taken from the name of the template (would be "Particle_").
Specifying empty prefix `[decs_template(prefix)]` will result in no prefix beeing added.

Note: apart from tagging structure as a template, the macro also generates `apply_decs_template` and `remove_decs_template` functions.
`apply_decs_template` is used to add template to an entity, and `remove_decs_template` is used to remove all components of the template from the entity::

    for i in range(3)
        create_entity <| @ ( eid, cmp )
            apply_decs_template(cmp, [[Particle pos=float3(i), vel=float3(i+1)]])

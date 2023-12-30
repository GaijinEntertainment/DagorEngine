
.. _stdlib_decs_boost:

======================
Boost package for DECS
======================

.. include:: detail/decs_boost.rst

The DECS_BOOST module implements queries, stages, and templates for the DECS.
Under normal circumstances this is the main require module for DECS.

All functions and symbols are in "decs_boost" module, use require to get access to it. ::

    require daslib/desc_boost


++++++++++++
Type aliases
++++++++++++

.. _alias-ItCheck:

.. das:attribute:: ItCheck is a variant type

+---+------+
+yes+string+
+---+------+
+no +bool  +
+---+------+


DECS prefix check.

++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-decs_boost-REQUIRE:

.. das:attribute:: REQUIRE

This annotation provides list of required components for entity. 

.. _handle-decs_boost-REQUIRE_NOT:

.. das:attribute:: REQUIRE_NOT

This annotation provides list of components, which are required to not be part of the entity. 

.. _handle-decs_boost-decs:

.. das:attribute:: decs

This macro converts a function into a DECS pass stage query. Possible arguments are `stage`, 'REQUIRE', and `REQUIRE_NOT`.
It has all other properties of a `query` (like ability to operate on templates). For example::

    [decs(stage=update_ai, REQUIRE=ai_turret)]
        def update_ai ( eid:EntityId; var turret:Turret; pos:float3 )
            ...

In the example above a query is added to the `update_ai` stage. The query also requires that each entity passed to it has an `ai_turret` property.

+++++++++++
Call macros
+++++++++++

.. _call-macro-decs_boost-query:

.. das:attribute:: query

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

.. _call-macro-decs_boost-find_query:

.. das:attribute:: find_query

This macro implmenets 'find_query` functionality.
It is similar to `query` in most ways, with the main differences being:
    * there is no eid-based find query
    * the find_query stops once the first match is found
For example::

    let found = find_query <| $ ( pos,dim:float3; obstacle:Obstacle )
    if !obstacle.wall
        return false
    let aabb = [[AABB min=pos-dim*0.5, max=pos+dim*0.5 ]]
    if is_intersecting(ray, aabb, 0.1, dist)
        return true

In the example above the find_query will return `true` once the first intesection is found.
Note: if return is missing, or end of find_query block is reached - its assumed that find_query did not find anything, and will return false.

++++++++++++++++
Structure macros
++++++++++++++++

.. _handle-decs_boost-decs_template:

.. das:attribute:: decs_template

This macro creates a template for the given structure.
`apply_decs_template` and `remove_decs_template` functions are generated for the structure type.



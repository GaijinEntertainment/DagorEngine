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

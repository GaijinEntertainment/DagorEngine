require UnitTest
require math

def square(x)
    return x*x

[export]
def find_best_squad_pos(wish_pos : float3; leader_tm : float3x4; agent_pos : float3;
                        offset : float3; num_iter : int; leader_min_dist : float) : float3
    var wishPosition = wish_pos
    for iter in range( num_iter)
        project_to_nearest_navmesh_point(wishPosition, 0.5)
        if length_sq(wishPosition - leader_tm[3]) < square(leader_min_dist)
            wishPosition = leader_tm * (float3(1 << (iter + 1)) * offset)
        else
            break
    project_to_nearest_navmesh_point(wishPosition, 0.5)
    return wishPosition

[export]
def test()
    var pos  = float3(1,2,3)
    project_to_nearest_navmesh_point(pos, 1.0)
    assert(pos==float3(2,3,4))
    return true

require ecs
require Dacoll
require math

[generic]
def traceray_normalized(pos, dir : float3; var t : float&; var out_norm : float3&) : bool
  return Dacoll::traceray_normalized(pos, dir, t, out_norm, Dacoll::ETF_DEFAULT)

[generic]
def traceray_normalized_coll_type(pos, dir : float3; var t : float&; var out_norm : float3&; var out_coll_type : int&) : bool
  return Dacoll::traceray_normalized_coll_type(pos, dir, t, out_norm, Dacoll::ETF_DEFAULT, out_coll_type)

[generic]
def sphere_cast(from, to : float3; rad : float; var out : ShapeQueryOutput; cast_mat_id : int = -1)
  return _builtin_sphere_cast(from, to, rad, out, cast_mat_id)

[generic]
def add_dynamic_collision(props : DataBlock |#) : CollisionObject
  return Dacoll::add_dynamic_collision(props, /*userPtr*/ null, /*is_player*/ false, /*add_to_world*/ true, DEFAULT_DYN_COLLISION_MASK)

[generic]
def test_collision_ri(collobj : CollisionObject; box : BBox3) : bool
  return Dacoll::test_collision_ri(collobj, box) <| $ [unused_argument(contacts)] (contacts : array<CollisionContactData>#) {}

[generic]
def test_collision_ri_ex(collobj : CollisionObject; box : BBox3; mat_id : int) : bool
  return Dacoll::test_collision_ri_ex(collobj, box, mat_id) <| $ [unused_argument(contacts)] (contacts : array<CollisionContactData>#) {}

[generic]
def test_collision_frt(collobj : CollisionObject; mat_id = -1) : bool
  return Dacoll::test_collision_frt(collobj, mat_id) <| $ [unused_argument(contacts)] (contacts : array<CollisionContactData>#) {}

[generic]
def traceht_lmesh(pos : float3) : float
  let pos2d = pos.xz
  return traceht_lmesh(pos2d)
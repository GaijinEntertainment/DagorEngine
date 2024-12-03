options no_aot
require ecs
require ecs.verify_common
require ecs.verify_macro
require soundEvent


[req_template]
struct SoundLandmeshQueries
  is_watched_sound : bool
  transform : float3x4
  sound_landmesh_queries_vars__speedSq : float
  sound_landmesh_queries_vars__isOnGround : bool
  sound_landmesh_queries__biomes : Object
  sound_landmesh_queries__lmeshDiff : float
  sound_landmesh_queries__farawayThreshold : float
  sound_landmesh_queries__heroSpeedToQueryInterval : float4
  sound_landmesh_queries__enemySpeedToQueryInterval : float4
  sound_landmesh_queries__heroEnemyInAirInterval : float2
  sound_landmesh_queries__lastQueryTime : float
  sound_landmesh_queries__biomeQueryId : int
  sound_landmesh_queries__puddleQueryId : int
  sound_landmesh_queries__biomeToMaterial : IPoint2List
  sound_landmesh_queries__biomePhysMatId : int
  sound_landmesh_queries__biomeOverridesWater : bool
  sound_landmesh_queries__puddleQueryToDepth : float4
  sound_landmesh_queries__puddleDepth : float

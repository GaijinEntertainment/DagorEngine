options no_aot
require ecs
require ecs.verify_common
require ecs.verify_macro
require soundEvent
require DngPhysObj


[req_template]
struct PhysObjCollisionSoundRequiredComponents
  phys_obj_net_phys : PhysObjActor
  transform : float3x4


[req_template]
struct PhysObjCollisionSoundTemplate
  phys_obj_collision_sound__threshold : float2
  phys_obj_collision_sound__path : string
  phys_obj_collision_sound__numSounds : int
  phys_obj_collision_sound__enabled : bool
  phys_obj_collision_sound__offset : float3
  phys_obj_collision_sound__interval : float
  phys_obj_collision_sound__omegaMul : float
  phys_obj_collision_sound__maxRange : float
  phys_obj_collision_sound__time : float
  phys_obj_collision_sound__maxImpulse : float
  phys_obj_collision_sound__event : SoundEvent

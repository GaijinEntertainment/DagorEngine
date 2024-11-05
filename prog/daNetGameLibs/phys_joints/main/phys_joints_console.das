options no_aot

require ecs
require DagorConsole

[console_cmd(name="phys_joints.create")]
def create_joint(first : uint; second : uint; fx, fy, fz, sx, sy, sz : float)
  query(EntityId(first)) <| $ [es] (transform : float3x4)
    let firstPos = float3(fx, fy, fz)
    let fpWorld = transform * firstPos
    query(EntityId(second)) <| $ [es] (transform : float3x4)
      let secondPos = float3(sx, sy, sz)
      let spWorld = transform * secondPos
      createEntity("phys_joint_pt+rope_phys") <| $(var init)
        set(init, "phys_joint__firstBody", EntityId(first))
        set(init, "phys_joint__secondBody", EntityId(second))
        set(init, "phys_joint__firstBodyAnchor", firstPos)
        set(init, "phys_joint__secondBodyAnchor", secondPos)
        set(init, "rope_phys__start", fpWorld)
        set(init, "rope_phys__end", spWorld)


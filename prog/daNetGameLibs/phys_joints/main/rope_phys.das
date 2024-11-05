require ecs
require GamePhys
require math.base
require DagorDebug3D
require DagorMath
require Dacoll

[es(on_appear)]
def init_rope_phys(evt : Event; rope_phys__numPoints : int; rope_phys__length : float;
                   rope_phys__start, rope_phys__end : float3;
                   var rope_phys__segmentLen : float&; var spline_gen_geometry__points : Point3List&; var rope_phys__prevPos : Point3List&)
  rope_phys__segmentLen = rope_phys__length / float(rope_phys__numPoints - 1)
  resize(spline_gen_geometry__points, rope_phys__numPoints)
  resize(rope_phys__prevPos, rope_phys__numPoints)
  for i in range(rope_phys__numPoints)
    let p = lerp(rope_phys__start, rope_phys__end, float3(float(i) / float(rope_phys__numPoints - 1)))
    spline_gen_geometry__points[i] = p
    rope_phys__prevPos[i] = p

[es(no_order)]
def update_rope_phys(info : UpdateStageInfoAct; rope_phys__segmentLen : float;
                     var spline_gen_geometry__points : Point3List&; var rope_phys__prevPos : Point3List&;
                     rope_phys__lastFixed : bool = true)
  let grav = gravity()
  let viscMult = 0.98f
  let lastIndexToUpdate = length(spline_gen_geometry__points) - (rope_phys__lastFixed ? 1 : 0)
  for i in range(1, lastIndexToUpdate)
    let prevPos = spline_gen_geometry__points[i]
    let accel = float3(0f, -grav, 0f)
    spline_gen_geometry__points[i] += (spline_gen_geometry__points[i] - rope_phys__prevPos[i]) * viscMult + accel * square(info.dt) * 0.5f

    let traceDist = 2f
    let threshold = 0.1f
    let addHt = traceDist * 0.5f
    let upPos = spline_gen_geometry__points[i] + float3(0f, addHt, 0f)
    var t = traceDist
    tracedown_normalized(upPos, t, ETF_DEFAULT)
    if t < addHt + threshold
      spline_gen_geometry__points[i].y = upPos.y - t + threshold

    rope_phys__prevPos[i] = prevPos

  // TODO: multiray traces for performance!!!

  for _ in range(5)
    for i in range(0, length(spline_gen_geometry__points) - 1)
      let curPos = spline_gen_geometry__points[i]
      let nextPos = spline_gen_geometry__points[i + 1]
      let dirToNext = nextPos - curPos
      let distToNext = length(dirToNext)
      let diff = rope_phys__segmentLen - distToNext
      let curFixed = i == 0
      let nextFixed = i + 1 == length(spline_gen_geometry__points) - 1 && rope_phys__lastFixed
      if !curFixed
        spline_gen_geometry__points[i] -= (nextFixed ? 1f : 0.5f) * diff / distToNext * dirToNext
      if !nextFixed
        spline_gen_geometry__points[i + 1] += (curFixed ? 1f : 0.5f) * diff / distToNext * dirToNext

[es(before=update_rope_phys, after=after_net_phys_sync)]
def update_rope_fixed_points_from_joints(info : UpdateStageInfoAct; var spline_gen_geometry__points : Point3List&;
                                         phys_joint__firstBody : EntityId;
                                         phys_joint__secondBody : EntityId;
                                         phys_joint__firstBodyAnchor : float3;
                                         phys_joint__secondBodyAnchor : float3)
  query(phys_joint__firstBody) <| $ [es] (transform : float3x4)
    let p1 = transform * phys_joint__firstBodyAnchor
    var secondExists = false
    query(phys_joint__secondBody) <| $ [es] (transform : float3x4)
      let p2 = transform * phys_joint__secondBodyAnchor
      spline_gen_geometry__points[0] = p1
      spline_gen_geometry__points[length(spline_gen_geometry__points) - 1] = p2
      secondExists = true
    if !secondExists
      let p2 = phys_joint__secondBodyAnchor
      spline_gen_geometry__points[0] = p1
      spline_gen_geometry__points[length(spline_gen_geometry__points) - 1] = p2

[es(before=update_rope_phys, after=after_net_phys_sync, REQUIRE_NOT=phys_joint__secondBody)]
def update_detached_rope_fixed_point_from_joint(info : UpdateStageInfoAct; var spline_gen_geometry__points : Point3List&;
                                                phys_joint__firstBody : EntityId;
                                                phys_joint__firstBodyAnchor : float3)
  var bodyExists = false
  query(phys_joint__firstBody) <| $ [es] (transform : float3x4)
    bodyExists = true
    spline_gen_geometry__points[0] = transform * phys_joint__firstBodyAnchor
  if !bodyExists
    spline_gen_geometry__points[0] = phys_joint__firstBodyAnchor

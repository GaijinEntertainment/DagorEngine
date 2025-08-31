options no_aot
require ecs
require ecs.ecs_template
require DagorConsole
require AnimV20
require CollRes
require GeomNodeTree
require DngCamera
require RendInst
require Dacoll
require PhysMat
require RendInst
require DagorDebug3D
require DagorSystem
require DagorFiles
require math.base
require strings

enum CollresDrawAllMode
  NONE
  DEFAULT
  NORMALS
  BBOX
  MODES_COUNT


[ecs_template]
struct collres_draw_all_debug
  drawCollisionResourceMode : int
  drawCollisionResourceDist : float


[console_cmd(name="collres.draw_all", hint="draws collision geometry near camera, 0 - NONE, 1 - COLLIDERS, 2 - COLLIDERS and NORMALS, 3 - BBOX")]
def console_draw_coll_res(mode : int = 1;
                          dist : float = 16.0)
  if mode < 0 || mode >= int(CollresDrawAllMode.MODES_COUNT)
    console_print("Unknown draw mode for collres.draw_all {mode}")
    return

  let found = find_query() <| $ [es] (eid : EntityId;
                                      var drawCollisionResourceMode : int&;
                                      var drawCollisionResourceDist : float&)
    let isEqual = (mode == int(drawCollisionResourceMode) && dist == drawCollisionResourceDist)
    if isEqual || mode == int(CollresDrawAllMode.NONE) || dist <= 0.0
      destroyEntity(eid)
    else
      drawCollisionResourceMode = mode
      drawCollisionResourceDist = dist
    return true
  if !found && mode != int(CollresDrawAllMode.NONE) && dist > 0.0
    createEntity("collres_draw_all_debug") <| $(var init : ComponentsInitializer)
      init |> set("drawCollisionResourceMode", mode)
      init |> set("drawCollisionResourceDist", dist)


def draw_node_names(collres : CollisionResource;
                    transform : float3x4;
                    animchar : AnimcharBaseComponent const?;
                    color : E3DCOLOR)
  let numNodes = collres_get_nodesCount(collres)
  for i in range(numNodes)
    let node = collres_get_node(collres, i)
    var nodeTm : float3x4
    collres_get_collision_node_tm(collres, i, transform, animchar != null ? animchar.nodeTree : null, nodeTm)
    let name = node |> collnode_get_name()
    add_debug_text_mark(nodeTm[3], name, -1, 0f, color)

def draw_node_collision_geometry(collres : CollisionResource;
                                 transform : float3x4;
                                 animchar : AnimcharBaseComponent const?;
                                 color : E3DCOLOR;
                                 cam_dir : float3;
                                 draw_normals : bool = false)
  let numNodes = collres_get_nodesCount(collres)
  for i in range(numNodes)
    let node = collres_get_node(collres, i)
    var nodeTm : float3x4
    collres_get_collision_node_tm(collres, i, transform, animchar != null ? animchar.nodeTree : null, nodeTm)
    set_cached_debug_lines_wtm(transform)
    if node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_MESH
      let camDirNodeSpace = draw_normals ? rotate(inverse(nodeTm), cam_dir) : float3()
      set_cached_debug_lines_wtm(nodeTm)
      get_collnode_geom(node) <| $(indices; vertices)
        for idx in range(length(indices) / 3)
          let v0 = vertices[int(indices[idx * 3 + 0])].xyz
          let v1 = vertices[int(indices[idx * 3 + 1])].xyz
          let v2 = vertices[int(indices[idx * 3 + 2])].xyz
          draw_cached_debug_line(v0, v1, color)
          draw_cached_debug_line(v1, v2, color)
          draw_cached_debug_line(v2, v0, color)

          if draw_normals
            let normalVec = cross(v1 - v0, v2 - v0)
            let normalVecLenSq = length_sq(normalVec)
            if normalVecLenSq > FLT_EPSILON
              let midPoint = (v0 + v1 + v2) / 3.0
              let normal = normalVec / sqrt(normalVecLenSq)
              let tangent = get_normal(normal)
              let bitangent = cross(normal, tangent)
              let arrowRad = 0.005
              let normalDrawLen = 0.2
              let arrowLen = normalDrawLen * 0.1
              let normalPoint = midPoint + normal * normalDrawLen

              let arrowPoint0 = normalPoint + tangent * arrowRad - normal * arrowLen
              let arrowPoint1 = normalPoint - tangent * arrowRad - normal * arrowLen
              let arrowPoint2 = normalPoint + bitangent * arrowRad - normal * arrowLen
              let arrowPoint3 = normalPoint - bitangent * arrowRad - normal * arrowLen

              let normalColor = dot(camDirNodeSpace, normal) < 0.0 ? E3DCOLOR(0xD51C77FF) : E3DCOLOR(0x44FF0C0C)

              draw_cached_debug_line(midPoint, normalPoint, normalColor)
              draw_cached_debug_line(normalPoint, arrowPoint0, normalColor)
              draw_cached_debug_line(normalPoint, arrowPoint1, normalColor)
              draw_cached_debug_line(normalPoint, arrowPoint2, normalColor)
              draw_cached_debug_line(normalPoint, arrowPoint3, normalColor)

    elif node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_BOX
      draw_cached_debug_box(node.modelBBox, color)
    elif node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_SPHERE
      draw_cached_debug_sphere(node.boundingSphere.c, node.boundingSphere.r, color, 12)
    elif node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_CAPSULE
      draw_cached_debug_capsule_w(node.capsule, color)


def for_each_collres_near_camera(func_cb; cam_dist : float = 16.0)
  query() <| $ [es] (transform : float3x4; camera__active : bool)
    if !camera__active
      return
    let camPos = transform[3]
    let camDist = cam_dist
    begin_draw_cached_debug_lines(true, true, false)
    query() <| $ [es] (transform : float3x4; collres : CollisionResource; animchar : AnimcharBaseComponent const?)
      if length_sq(transform[3] - camPos) >= square(camDist)
        return
      let color = E3DCOLOR(0xFF00FF00)
      func_cb |> invoke(collres, transform, animchar, color)

    rendinst_foreachInBox(BBox3(BSphere3(camPos, camDist)), int(GatherRiTypeFlags.RiGenAndExtra)) <| $(desc : RendInstDesc; tm : float3x4; is_tm : bool)
      if length_sq(tm[3] - camPos) >= square(camDist)
        return
      let collres = getRiGenCollisionResource(desc)
      if collres == null
        return
      let color = E3DCOLOR(is_tm ? 0xFF00FF00 : 0xFFFF0000)
      func_cb |> invoke(*collres, tm, null, color)
      let pos = tm[3]
      add_debug_text_mark(pos, "{getRIGenResName(desc)}", -1, 0., color)

    end_draw_cached_debug_lines()


[es(tag=(render, dev), no_order)]
def debug_dynamic_collision(info : UpdateStageInfoRenderDebug;
                            drawCollisionResourceMode : int;
                            drawCollisionResourceDist : float)
  find_query() <| $ [es] (transform aka camera_transform : float3x4;
                          camera__active : bool)
    if !camera__active
      return false
    let drawNodeCollisionGeometry <- $ <| (collres : CollisionResource; tm : float3x4; animchar : AnimcharBaseComponent const ?; color : E3DCOLOR)
      if drawCollisionResourceMode == int(CollresDrawAllMode.BBOX)
        set_cached_debug_lines_wtm(tm)
        draw_cached_debug_box(collres.boundingBox, E3DCOLOR(0xFFFFFFFF))
        set_cached_debug_lines_wtm(IDENT_TM)
      else
        let drawNormals = (drawCollisionResourceMode == int(CollresDrawAllMode.NORMALS))
        draw_node_collision_geometry(collres, tm, animchar, color, camera_transform[2], drawNormals)
    for_each_collres_near_camera(drawNodeCollisionGeometry, drawCollisionResourceDist)
    return true


[ecs_template]
struct collres_names_debug
  drawCollresNames : bool

[console_cmd(name="collres.draw_names", hint="draws collres node names and positions near camera")]
def toggle_draw_coll_res_names()
  var exists = false
  query() <| $ [es(REQUIRE=drawCollresNames)] (eid : EntityId)
    destroyEntity(eid)
    exists = true
  if !exists
    createEntity("collres_names_debug")

[es(tag=(render, dev), REQUIRE=drawCollresNames, no_order)]
def draw_collres_node_names(info : UpdateStageInfoRenderDebug)
  for_each_collres_near_camera(@@draw_node_names)


[ecs_template]
struct collision_ruler_debug
  collisionRulerDebug : bool

def get_node_type_name(node_type : CollisionResourceNodeType)
  if node_type == CollisionResourceNodeType.COLLISION_NODE_TYPE_MESH
    return "mesh"
  if node_type == CollisionResourceNodeType.COLLISION_NODE_TYPE_POINTS
    return "points"
  if node_type == CollisionResourceNodeType.COLLISION_NODE_TYPE_BOX
    return "box"
  if node_type == CollisionResourceNodeType.COLLISION_NODE_TYPE_SPHERE
    return "sphere"
  if node_type == CollisionResourceNodeType.COLLISION_NODE_TYPE_CAPSULE
    return "capsule"
  if node_type == CollisionResourceNodeType.COLLISION_NODE_TYPE_CONVEX
    return "convex"
  return "unknown"

[console_cmd(name="collres.ruler")]
def draw_collres_ruler()
  let found = find_query() <| $ [es(REQUIRE=collisionRulerDebug)] (eid : EntityId)
    destroyEntity(eid)
    return true
  if !found
    createEntity("collision_ruler_debug")

def format_scale(flags : uint8; tm : float3x4)
  if (uint(flags) & uint(CollisionNodeFlag.IDENT)) != 0u
    return "identity"
  if (uint(flags) & uint(CollisionNodeFlag.TRANSLATE)) != 0u
    return "translate"
  if (uint(flags) & uint(CollisionNodeFlag.ORTHONORMALIZED)) != 0u
    return "orthonormalized"
  if (uint(flags) & uint(CollisionNodeFlag.ORTHOUNIFORM)) != 0u
    return "orthouniform (scale {length(tm[0]):.1f})"
  return "scaled: {length(tm[0]):.3f} | {length(tm[1]):.3f} | {length(tm[2]):.3f}"

def format_scale(tm : float3x4)
  var flags = uint8(0)
  let len0sq = length_sq(tm[0])
  let len1sq = length_sq(tm[1])
  let len2sq = length_sq(tm[2])
  if abs(dot(tm[0], tm[1])) < 1e-3f && abs(dot(tm[0], tm[2])) < 1e-3f && abs(dot(tm[1], tm[2])) < 1e-3f
    if abs(len1sq - len0sq) < len0sq * 1e-3f && abs(len2sq - len0sq) < len0sq * 1e-3f
      flags = abs(len0sq - 1.f) < 1e-3f ? uint8(CollisionNodeFlag.ORTHONORMALIZED) : uint8(CollisionNodeFlag.ORTHOUNIFORM)
  return format_scale(flags, tm)

def get_node_info(node) : string
  var geomInfo = "; type: {get_node_type_name(node.nodeType)}"
  if node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_MESH || node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_CONVEX
    get_collnode_geom(node) <| $ [unused_argument(vertices)] (indices; vertices)
      geomInfo += " ({length(indices) / 3} trs)"
  if node.geomNodeId != -1
    geomInfo += "; geomNodeId: {node.geomNodeId}"
  else
    geomInfo += "; " + format_scale(node.flags, node.tm)

  var behFlags = ""
  if (uint(node.behaviorFlags) & uint(BehaviorFlag.TRACEABLE)) != 0u
    behFlags += (behFlags != "" ? "+" : "") + "trace"
  if (uint(node.behaviorFlags) & uint(BehaviorFlag.PHYS_COLLIDABLE)) != 0u
    behFlags += (behFlags != "" ? "+" : "") + "phys"
  if behFlags != ""
    geomInfo += "; beh: {behFlags}"

  return "node {node.nodeIndex}: {node |> collnode_get_name()}" + geomInfo

def get_grid_info(collres : CollisionResource; flag : uint8)
  var width : int3
  var leafSize : float3
  if collres |> collres_getGridSize(flag, width, leafSize)
    return "{width.x}x{width.y}x{width.z} leaf {max(leafSize.x, max(leafSize.y, leafSize.z)):.2f}"
  return "no"

def draw_collres_node_debug(name : string; cam_itm, transform : float3x4; animchar : AnimcharBaseComponent const?;
                            dist : float; best_node_idx : int; intersection_pos : float3; collres : CollisionResource)
  let markOffsetScale = dist * 0.22
  let markPos = intersection_pos + cam_itm[0] * markOffsetScale + cam_itm[1] * markOffsetScale * 0.75
  let node = collres |> collres_get_node(best_node_idx)
  let nodeTree = animchar != null ? animchar.nodeTree : null
  var ntm : float3x4
  collres |> collres_get_collision_node_tm(int(node.nodeIndex), transform, nodeTree, ntm)
  set_cached_debug_lines_wtm(ntm)
  draw_cached_debug_box(node.modelBBox, E3DCOLOR(0xFFBF00FF))
  set_cached_debug_lines_wtm(transform)
  draw_cached_debug_sphere(collres.boundingSphereCenter, collres.boundingSphereRad, E3DCOLOR(0xFFBBBB00), 24)
  set_cached_debug_lines_wtm(IDENT_TM)
  var meshCnt, ptCnt, boxCnt, sphCnt, capCnt, cvxCnt : int
  var traceableTriCnt, collidableTriCnt : int
  var traceableCnt, collidableCnt, geomCnt : int
  let nodesCnt = collres |> collres_get_nodesCount()
  var scaledCnt = 0
  var notUniformCnt = 0
  var noteId = -1
  var noteType = 0
  for i in range(nodesCnt)
    let inode = collres |> collres_get_node(i)
    let isTraceable = (uint(inode.behaviorFlags) & uint(BehaviorFlag.TRACEABLE)) != 0u
    let isCollidable = (uint(inode.behaviorFlags) & uint(BehaviorFlag.PHYS_COLLIDABLE)) != 0u
    if inode.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_MESH
      meshCnt++
      if (uint(inode.flags) & uint(CollisionNodeFlag.ORTHONORMALIZED)) == 0u
        scaledCnt++
        if noteType <= 1 && int(inode.nodeIndex) != best_node_idx
          noteType = 1
          noteId = int(inode.nodeIndex)
      if (uint(inode.flags) & (uint(CollisionNodeFlag.ORTHONORMALIZED) | uint(CollisionNodeFlag.ORTHOUNIFORM))) == 0u
        notUniformCnt++
        if noteType <= 2 && int(inode.nodeIndex) != best_node_idx
          noteType = 2
          noteId = int(inode.nodeIndex)
      get_collnode_geom(inode) <| $ [unused_argument(vertices)] (indices; vertices)
        let tri = length(indices) / 3
        if isTraceable
          traceableTriCnt += tri
        if isCollidable
          collidableTriCnt += tri
    if inode.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_POINTS
      ptCnt++
    if inode.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_BOX
      boxCnt++
    if inode.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_SPHERE
      sphCnt++
    if inode.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_CAPSULE
      capCnt++
    if inode.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_CONVEX
      cvxCnt++
    if isTraceable
      traceableCnt++
    if isCollidable
      collidableCnt++
    if (inode.geomNodeId != -1)
      geomCnt++
  var nodesText = "Total nodes: {nodesCnt}"
  if meshCnt != 0
    nodesText += "; meshes: {meshCnt}"
    var meshInfo : string
    if scaledCnt != 0
      meshInfo += (meshInfo != "" ? "; " : "") + "scaled: {scaledCnt}"
    if notUniformCnt != 0
      meshInfo += (meshInfo != "" ? "; " : "") + "not uniform: {notUniformCnt}"
    if meshInfo != ""
      nodesText += " ({meshInfo})"
  if ptCnt != 0
    nodesText += "; pt: {ptCnt}"
  if boxCnt != 0
    nodesText += "; box: {boxCnt}"
  if sphCnt != 0
    nodesText += "; sph: {sphCnt}"
  if capCnt != 0
    nodesText += "; cap: {capCnt}"
  if cvxCnt != 0
    nodesText += "; cvx: {cvxCnt}"
  if traceableCnt != 0
    nodesText += "; traceable: {traceableCnt} ({traceableTriCnt} trs)"
  if collidableCnt != 0
    nodesText += "; phys_collidable: {collidableCnt} ({collidableTriCnt} trs)"
  if geomCnt != 0
    nodesText += "; geom_attached: {geomCnt}"
  add_debug_text_mark(markPos, name, -1, 0.0, E3DCOLOR(0xFF8FFF8F))
  add_debug_text_mark(markPos, "dist {dist:.2f}m; bsph_rad: {collres.boundingSphereRad:.3f}; node_mat: {node.physMatId} ({get_material_name(get_material(int(node.physMatId)))}); tm " + format_scale(transform), -1, 1.2, E3DCOLOR(0xFF8FFF8F))
  add_debug_text_mark(markPos, "Selected " + get_node_info(node), -1, 2.4, E3DCOLOR(0xFF8FFF8F))
  add_debug_text_mark(markPos, nodesText, -1, 3.6, E3DCOLOR(0xFF8FFF8F))
  if noteType != 0
    add_debug_text_mark(markPos, "Last scaled " + get_node_info(collres |> collres_get_node(noteId)), -1, 4.8, E3DCOLOR(0xFF8FFF8F))
  add_debug_text_mark(markPos, "Grids: trace={get_grid_info(collres, uint8(BehaviorFlag.TRACEABLE))} coll={get_grid_info(collres, uint8(BehaviorFlag.PHYS_COLLIDABLE))}", -1, noteType != 0 ? 6.0 : 4.8, E3DCOLOR(0xFF8FFF8F))

[es(tag=(render, dev), no_order, REQUIRE=collisionRulerDebug)]
def draw_collision_ruler_es(evt : UpdateStageInfoRenderDebug)
  let camItm = get_TMatrix(get_cur_cam_entity(), "transform")
  if camItm == null
    return
  let cameraPos = (*camItm)[3] - float3(0.0, 0.05, -0.075)
  let cameraDir = (*camItm)[2]

  var bestT = 4000.0
  var bestEid = INVALID_ENTITY_ID
  var bestNodeIdx = -1
  var hitNormal : float3
  ecs::query() <| $ [es] (eid : EntityId; transform : float3x4; collres : CollisionResource; animchar : AnimcharBaseComponent const?)
    let nodeTree = animchar != null ? animchar.nodeTree : null
    collres |> collres_traceray(transform, nodeTree, cameraPos, cameraDir, bestT, true, uint8(BehaviorFlag.TRACEABLE)) <| $(is_hit : bool; isects : CollResIntersectionsType#)
      if is_hit
        bestT = isects[0].intersectionT
        bestEid = eid
        bestNodeIdx = int(isects[0].collisionNodeId)
        hitNormal = isects[0].normal

  var riDesc = RendInstDesc()
  var norm : float3
  var matId = PHYSMAT_INVALID
  let res = traceray_normalized(cameraPos, cameraDir, bestT, matId, norm, ETF_RI | ETF_RI_TREES | ETF_RI_PHYS, riDesc, -1)
  let intersectionPos = cameraPos + cameraDir * bestT
  if res && riDesc.isValid
    let collres = getRiGenCollisionResource(riDesc)
    if collres != null
      let riTm = getRIGenMatrix(riDesc)
      bestT += 0.01
      let behFlags = fixed_array<uint8>(uint8(BehaviorFlag.TRACEABLE), uint8(BehaviorFlag.PHYS_COLLIDABLE))
      for behFlag in behFlags
        var traced = false
        *collres |> collres_traceray(riTm, null, cameraPos, cameraDir, bestT, true, behFlag) <| $(is_hit : bool; isects : CollResIntersectionsType#)
          traced = is_hit
          if is_hit
            bestT = isects[0].intersectionT
            bestEid = INVALID_ENTITY_ID
            bestNodeIdx = int(isects[0].collisionNodeId)
            hitNormal = isects[0].normal
            let nameType = riDesc.isRiExtra ? "RiExtra: " : "RendInst: "
            let handleStr = riDesc.isRiExtra ? " {riDesc.riExtraHandle}" : ""
            draw_collres_node_debug(nameType + getRIGenResName(riDesc) + handleStr, *camItm, riTm, null, bestT, bestNodeIdx, intersectionPos, *collres)
        if traced
          break

  begin_draw_cached_debug_lines(true, false, false)
  ecs::query(bestEid) <| $ [es] (transform : float3x4; collres : CollisionResource; animchar : AnimcharBaseComponent const?)
    draw_collres_node_debug("Entity: {getEntityTemplateName(bestEid)}", *camItm, transform, animchar, bestT, bestNodeIdx, intersectionPos, collres)

  draw_cached_debug_line(cameraPos, intersectionPos, E3DCOLOR(0xFFFF2020))
  if res || bestEid != INVALID_ENTITY_ID
    let rad = pow(bestT / 300.0, 0.8)
    draw_cached_debug_sphere(intersectionPos, rad, E3DCOLOR(0xFFFFFF00), 24)

    let normalPoint = intersectionPos + hitNormal * rad * 5.0
    draw_cached_debug_line(intersectionPos, normalPoint, E3DCOLOR(0xFFFFFF00))
  end_draw_cached_debug_lines()

[console_cmd(name="collres.dump_vertices", hint="or aimed rendInst will be used")]
def collres_dump_vertices(ri_name : string = "")
  var riType = -1
  var riName = ri_name
  if !empty(ri_name)
    riType = get_rigen_extra_res_idx(ri_name)
  else
    let camItm = get_TMatrix(get_cur_cam_entity(), "transform")
    if camItm == null
      return
    let cameraPos = (*camItm)[3] - float3(0.0, 0.05, -0.075)
    let cameraDir = (*camItm)[2]
    var t = 4000.0
    var riDesc = RendInstDesc()
    var norm : float3
    var matId = PHYSMAT_INVALID
    if traceray_normalized(cameraPos, cameraDir, t, matId, norm, ETF_RI | ETF_RI_TREES | ETF_RI_PHYS, riDesc, -1)
      riType = riDesc.pool
      riName = getRIGenResName(riDesc)
  if riType < 0
    console_print("Invalid rendInst name or not aimed to target")
    return
  let riCollres = get_ri_gen_extra_collres(riType)
  if riCollres != null
    let fileName = riName + "_coll.obj"
    df_open(fileName, DF_WRITE) <| $(file)
      let nodeCount = collres_get_nodesCount(*riCollres)
      df_write(file, "### This file contains {nodeCount} mesh nodes data ###\n")
      df_write(file, "### Vertices ###\n")
      for nodeIdx in range(nodeCount)
        let node = *riCollres |> collres_get_node(nodeIdx)
        let isCollidable = (uint(node.behaviorFlags) & uint(BehaviorFlag.PHYS_COLLIDABLE)) != 0u
        if !isCollidable
          continue
        df_write(file, "# Node '{node |> collnode_get_name()}'\n")
        get_collnode_geom(node) <| $ [unused_argument(indices)] (indices; vertices)
          if file != null
            for v in vertices
              df_write(file, "v {v.x} {v.y} {v.z}\n")
      df_write(file, "\n### Indices ###\n")
      var indexOffset = 1
      for nodeIdx in range(nodeCount)
        let node = *riCollres |> collres_get_node(nodeIdx)
        let isCollidable = (uint(node.behaviorFlags) & uint(BehaviorFlag.PHYS_COLLIDABLE)) != 0u
        if !isCollidable
          continue
        df_write(file, "# Node '{node |> collnode_get_name()}' Offset +{indexOffset}\n")
        get_collnode_geom(node) <| $(indices; vertices)
          if file != null
            for i in range(length(indices) / 3)
              df_write(file, "f {int(indices[i*3+0]) + indexOffset} {int(indices[i*3+1]) + indexOffset} {int(indices[i*3+2]) + indexOffset}\n")
          indexOffset += length(vertices)
    console_print("Dumped to {fileName} (Wavefront obj format)")

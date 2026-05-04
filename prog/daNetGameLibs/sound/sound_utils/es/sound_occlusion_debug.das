options no_aot
require ecs
require ecs.common
require ecs.safe
require ecs.ecs_template
require DagorConsole
require RendInst
require Dacoll
require CollRes
require PhysMat
require soundSystem
require math.color
require math.base
require DagorDebug3D
require DagorDataBlock
require DagorSystem

def get_cur_cam_entity()
  var camEid = INVALID_ENTITY_ID
  find_query() <| $ [es] (eid : EntityId; camera__active : bool)
    camEid = eid
    return camera__active
  return camEid

def trace_ri(var intersection_out : float3&; var bestT : float&; var sel_ri_name : das_string&; debug_dist : float) : string
  let camItm = get_TMatrix(get_cur_cam_entity(), "transform")
  if camItm == null
    return ""
  let cameraPos = (*camItm)[3]
  let cameraDir = (*camItm)[2]
  bestT = debug_dist
  var riDesc = RendInstDesc()
  var norm : float3
  var matId = PHYSMAT_INVALID
  let flags = ETF_RI | ETF_RI_TREES | ETF_RI_PHYS
  let res = traceray_normalized(cameraPos, cameraDir, bestT, matId, norm, flags, riDesc, -1)
  if res && riDesc.isValid
    let collres = getRiGenCollisionResource(riDesc)
    if collres != null
      let numNodes = collres_get_nodesCount(*collres)

      var numFaces = 0
      var valid = false
      for i in range(numNodes)
        let node = collres_get_node(*collres, i)
        if (uint(node.behaviorFlags) & uint(BehaviorFlag.PHYS_COLLIDABLE)) != 0u
          valid = true
          if node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_MESH
            numFaces += collres_get_node_face_count(*collres, int(node.nodeIndex))

      if valid
        intersection_out = cameraPos + cameraDir * bestT
        sel_ri_name := getRIGenResName(riDesc)
        return "ri={getRIGenResName(riDesc)}; physmat={get_material_name(get_material(matId))}; faces={numFaces}"
  sel_ri_name := ""
  return ""



[es(tag=(sound, dev), on_event=EventSoundDrawDebug)]
def snd_draw_geometry_ruler(evt : Event;
                            sound_occlusion__debug : bool;
                            sound_occlusion__debugDist : float;
                            var sel_ri_name : das_string&;
                            sound_occlusion__ri_name_to_occluder_type : Object&)
  if !sound_occlusion__debug
    return
  var intersection : float3
  var bestT : float
  var text = trace_ri(intersection, bestT, sel_ri_name, sound_occlusion__debugDist)
  if !empty(text)

    if !empty(sel_ri_name)
      let typ = sound_occlusion__ri_name_to_occluder_type[string(sel_ri_name)] ?? "???"
      text = "{text}; typ={typ}"
      let val = sound_debug_get_sound_occlusion(string(sel_ri_name), -1.)
      if val >= 0.
        text = "{text}; riVal={val}"

    begin_draw_cached_debug_lines(false, false, false)
    add_debug_text_mark(intersection, text, -1, 0.0, E3DCOLOR(0xFF8FFF8F))
    let rad = pow(bestT / 200., 0.8)
    draw_cached_debug_sphere(intersection, rad, E3DCOLOR(0xFFFFFF00), 24)
    end_draw_cached_debug_lines()


struct NameType
  name : string
  typ : string


def copy_to_clipboard(sound_occlusion__ri_name_to_occluder_type : Object&)
  var text = ""
  var namesTypes : array<NameType>
  resize(namesTypes, length(sound_occlusion__ri_name_to_occluder_type))
  for it, nameType in sound_occlusion__ri_name_to_occluder_type, namesTypes
    nameType.name = string(it.key)
    nameType.typ = it.value as string ?? ""
  namesTypes |> sort() <| $(a, b)
    return a.name < b.name
  for it in namesTypes
    if !empty(text)
      text += "\n"
    text += "  {it.name}:t=\"{it.typ}\""
  if !empty(text)
    set_clipboard_ansi_text(text)
    sound_debug("copied to clipboard")


[console_cmd(name="snd.add_cur_occluder")]
def snd_add_cur_occluder__console_cmd(typ : string)
  query() <| $ [es] (var sound_occlusion__ri_name_to_occluder_type : Object&;
                     sound_occlusion__occluderTypes : Object&;
                     sel_ri_name : string)
    if !empty(sel_ri_name) && sound_occlusion__occluderTypes[typ] != null
      sound_occlusion__ri_name_to_occluder_type |> set(sel_ri_name, typ)
    copy_to_clipboard(sound_occlusion__ri_name_to_occluder_type)


[console_cmd(name="snd.remove_cur_occluder")]
def snd_remove_cur_occluder__console_cmd()
  query() <| $ [es] (var sound_occlusion__ri_name_to_occluder_type : Object&;
                     sel_ri_name : string)
    if !empty(sel_ri_name)
      sound_occlusion__ri_name_to_occluder_type |> erase(sel_ri_name)
    copy_to_clipboard(sound_occlusion__ri_name_to_occluder_type)


[console_cmd(name="snd.copy_occluders_to_clipboard")]
def copy_occluders_to_clipboard__console_cmd()
  query() <| $ [es] (sound_occlusion__ri_name_to_occluder_type : Object)
    copy_to_clipboard(sound_occlusion__ri_name_to_occluder_type)


def load_occluders_impl(filepath : string;
                        var sound_occluders_debug : Object&;
                        var sound_occluders_types : Object&)
  var success = false
  clear(sound_occluders_debug)
  using() <| $(var blk : DataBlock)
    if datablock_load(blk, filepath)
      datablock_get_block(blk, "soundOccluders") <| $(soundOccluders)
        for i in range(soundOccluders.paramCount)
          let name = soundOccluders |> datablock_getParamName(uint(i))
          let typ = soundOccluders |> datablock_getStr(i)
          sound_occluders_debug |> set(name, typ)
      sound_debug("loaded {length(sound_occluders_debug)} objects from '{filepath}_sound_occluders.blk'")
      success = true
  if success
    success = false
    clear(sound_occluders_types)
    using() <| $(var blk : DataBlock)
      if datablock_load(blk, filepath)
        datablock_get_block(blk, "soundOccluderTypes") <| $(soundOccluders)
          for i in range(soundOccluders.paramCount)
            let name = soundOccluders |> datablock_getParamName(uint(i))
            let value = soundOccluders |> datablock_getReal(i)
            sound_occluders_types |> set(name, value)
        sound_debug("loaded {length(sound_occluders_types)} types from '{filepath}_sound_occluder_types.blk'")
        success = true
  if !success
    logerr("something is wrong (load_occluders from \"{filepath}\")")
    clear(sound_occluders_debug)
    clear(sound_occluders_types)



[console_cmd(name="snd.debug_occluders")]
def snd_debug_occluders__console_cmd()
  query() <| $ [es] (var sound_occlusion__debug : bool&;
                     var sound_occlusion__ri_name_to_occluder_type : Object&;
                     var sound_occlusion__occluderTypes : Object&;
                     sound_load_occluders_debug__filepath : string)
    sound_occlusion__debug = !sound_occlusion__debug
    if sound_occlusion__debug
      load_occluders_impl(sound_load_occluders_debug__filepath, sound_occlusion__ri_name_to_occluder_type, sound_occlusion__occluderTypes)



[console_cmd(name="snd.set_occluders_dist")]
def snd_set_occluders_dist__console_cmd(dist : float)
  query() <| $ [es] (var sound_occlusion__debugDist : float&)
    sound_occlusion__debugDist = max(0., dist)


[console_cmd(name="snd.delete_used_occluders")]
def snd_delete_used_occluders__console_cmd()
  var wasCount = 0
  var nowCount = 0
  query() <| $ [es] (var sound_occlusion__ri_name_to_occluder_type : Object&)
    wasCount = length(sound_occlusion__ri_name_to_occluder_type)
    using() <| $(var obj : Object)
      for it in sound_occlusion__ri_name_to_occluder_type
        let name = string(it.key)
        let typ = it.value as string ?? ""
        if get_rigen_extra_res_idx(name) == -1
          set(obj, name, typ)
      sound_occlusion__ri_name_to_occluder_type := obj
      nowCount = length(sound_occlusion__ri_name_to_occluder_type)
      copy_to_clipboard(sound_occlusion__ri_name_to_occluder_type)
  sound_debug("wasCount={wasCount}, nowCount={nowCount}, deleted {wasCount - nowCount} existing objects")


[console_cmd(name="snd.delete_unused_occluders")]
def snd_delete_unused_occluders__console_cmd()
  var wasCount = 0
  var nowCount = 0
  query() <| $ [es] (var sound_occlusion__ri_name_to_occluder_type : Object&)
    wasCount = length(sound_occlusion__ri_name_to_occluder_type)
    using() <| $(var obj : Object)
      for it in sound_occlusion__ri_name_to_occluder_type
        let name = string(it.key)
        let typ = it.value as string ?? ""
        if get_rigen_extra_res_idx(name) != -1
          set(obj, name, typ)
      sound_occlusion__ri_name_to_occluder_type := obj
      nowCount = length(sound_occlusion__ri_name_to_occluder_type)
      copy_to_clipboard(sound_occlusion__ri_name_to_occluder_type)
  sound_debug("wasCount={wasCount}, nowCount={nowCount}, deleted {wasCount - nowCount} existing objects")



let WIRE_COLOR = 0x30FFFFFF
let SOLID_COLOR = 0x0000FF00

def make_color_temp(t : float; alpha : uint) : E3DCOLOR
  let clr0 = float3(0., 0., 1.)
  let clr1 = float3(0., 1., 0.)
  let clr2 = float3(1., 0., 0.)
  let clr = saturate(t < 0.5 ? lerp(clr0, clr1, t * 2.) : lerp(clr1, clr2, (t - 0.5) * 2.))
  let r = uint(clr.x * 255.) << 16u
  let g = uint(clr.y * 255.) << 8u
  let b = uint(clr.z * 255.)
  let a = alpha << 24u
  return E3DCOLOR(r | g | b | a)


let SOLID_BOX_DIST = 100.

let BOX_FACES = fixed_array(float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(0.5, -0.5, 0.5), float3(0.5, -0.5, 0.5), float3(0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, -0.5, 0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, -0.5, -0.5), float3(0.5, -0.5, -0.5), float3(0.5, -0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5))


def draw_occluders(name : string; value : float;
                   listener : float3;
                   var verts : array<float3>&;
                   is_sel : bool;
                   debug_dist : float)
  var maxFaces = 1024
  let resIdx = get_rigen_extra_res_idx(name)
  if resIdx != -1
    get_ri_gen_extra_instances(resIdx) <| $(handles : array<riex_handle_t>#)
      for handle in handles
        let desc = RendInstDesc(handle)
        let collres = getRiGenCollisionResource(desc)

        if collres != null
          let tm = getRIGenMatrix(desc)
          let box = getRIGenBBox(desc)
          let maxBound = max(max(box.width.x, box.width.y), box.width.z)

          if distance_sq(listener, tm[3]) > square(debug_dist + maxBound)
            continue

          let numNodes = collres_get_nodesCount(*collres)
          for i in range(numNodes)
            let node = collres_get_node(*collres, i)
            if (uint(node.behaviorFlags) & uint(BehaviorFlag.PHYS_COLLIDABLE)) != 0u
              var nodeTm : float3x4
              collres_get_collision_node_tm(*collres, i, tm, null, nodeTm)

              let nodeBox = collres_get_node_bbox(*collres, i)
              if is_sel
                begin_draw_cached_debug_lines(false, false, false)
                set_cached_debug_lines_wtm(nodeTm)
                draw_cached_debug_box(nodeBox, make_color_temp(value, 200u))
                end_draw_cached_debug_lines()

              if !is_sel
                begin_draw_cached_debug_lines(false, false, false)
                set_cached_debug_lines_wtm(nodeTm)
                draw_cached_debug_box(nodeBox, make_color_temp(value, 100u))
                end_draw_cached_debug_lines()
              else
                let color = value >= 0. ? make_color_temp(value, is_sel ? 200u : 100u) : E3DCOLOR(0x40FFFFFF)

                let addExtent = distance(listener, nodeTm[3]) * 0.005

                var box3f : bbox3f
                box3f.bmin = float4(nodeBox.boxMin, 1.)
                box3f.bmax = float4(nodeBox.boxMax, 1.)
                let pt = float3(dot(nodeTm[0], listener - nodeTm[3]), dot(nodeTm[1], listener - nodeTm[3]), dot(nodeTm[2], listener - nodeTm[3]))
                let inside = bbox3f_test_pt_inside(box3f, pt) != 0

                if node.nodeType == CollisionResourceNodeType.COLLISION_NODE_TYPE_MESH
                  let numFaces = collres_get_node_face_count(*collres, int(node.nodeIndex))
                  let drawFaces = min(numFaces, maxFaces)
                  if drawFaces > 0
                    maxFaces -= drawFaces
                    clear(verts)
                    reserve(verts, drawFaces)

                    begin_draw_cached_debug_lines(is_sel ? false : true, false, false)
                    set_cached_debug_lines_wtm(nodeTm)

                    collres_node_iterate_face_verts(*collres, int(node.nodeIndex)) <| $ [unused_argument(idx)] (idx : int; v0, v1, v2 : float3)
                      if is_sel
                        draw_cached_debug_line(v0, v1, color)
                        draw_cached_debug_line(v1, v2, color)
                        draw_cached_debug_line(v2, v0, color)

                      let norm = normalize(cross(v1 - v0, v2 - v0)) * addExtent
                      push(verts, v0 + norm)
                      push(verts, v1 + norm)
                      push(verts, v2 + norm)

                    if !inside
                      draw_cached_debug_trilist(verts, color)
                    end_draw_cached_debug_lines()
                else
                  var boxTM = nodeTm
                  let scale = nodeBox.width + float3(addExtent)
                  boxTM[3] += boxTM[0] * (nodeBox.boxMin.x + nodeBox.boxMax.x) * 0.5
                  boxTM[3] += boxTM[1] * (nodeBox.boxMin.y + nodeBox.boxMax.y) * 0.5
                  boxTM[3] += boxTM[2] * (nodeBox.boxMin.z + nodeBox.boxMax.z) * 0.5
                  boxTM[0] *= scale.x
                  boxTM[1] *= scale.y
                  boxTM[2] *= scale.z
                  begin_draw_cached_debug_lines(true, false, false)
                  set_cached_debug_lines_wtm(boxTM)
                  if !inside
                    draw_cached_debug_trilist(BOX_FACES, color)
                  end_draw_cached_debug_lines()




[es(tag=(sound, dev, render), on_event=EventSoundDrawDebug, no_order)]
def snd_draw_occluders_debug(evt : Event;
                             sound_occlusion__ri_name_to_occluder_type : Object&;
                             sound_occlusion__occluderTypes : Object&;
                             sel_ri_name : string;
                             sound_occlusion__debug : bool;
                             sound_occlusion__debugDist : float)
  if !sound_occlusion__debug
    return
  var verts : array<float3>
  let listener = get_listener_pos()
  var drawSel = true
  for it in sound_occlusion__ri_name_to_occluder_type
    let name = string(it.key)
    let typ = it.value as string ?? ""
    let value = sound_occlusion__occluderTypes[typ] ?? 1.
    if sel_ri_name != name
      draw_occluders(name, value, listener, verts, false, sound_occlusion__debugDist)
    else
      draw_occluders(name, value, listener, verts, true, sound_occlusion__debugDist)
      drawSel = false

  if !empty(sel_ri_name) && drawSel
    draw_occluders(sel_ri_name, -1., listener, verts, true, sound_occlusion__debugDist)

require app
require ecs
require DagorSystem
require DagorDataBlock
require DagorMath
require DngCamera

[cpp_event(broadcast)]
struct EventOnClientConnected
  connid : int
  matching : uint64
  userName : das_string
  groupId : int64
  clientFlags : uint16
  platformUserId : das_string
  platform : das_string
  matchingTeam : int
  appId : int

[es(on_event=EventOnGameAppStarted)]
def on_gameapp_started_es(evt : Event)
  let isDisableMenu = dgs_get_settings() |> datablock_getBool("disableMenu", false)
  let settingScene = dgs_get_settings() |> datablock_getStr("scene", "")

  if true || isDisableMenu
    var sceneName = dgs_get_argv("scene")
    var importScenes : array<string>
    dgs_get_argv_all("importScene") <| $(importScene)
      importScenes |> push(importScene)
      return false
    if empty(sceneName)
      sceneName = settingScene

    print("[SCENE LOAD] starting with scene = {sceneName}, importScenes = {importScenes}")
    switch_scene(sceneName, importScenes)
  else
    print("[SCENE LOAD] starting with scene = gamedata/scenes/menu.blk")
    switch_scene("gamedata/scenes/menu.blk") // launch from enlist

[es]
def on_client_connected_server(evt : EventOnClientConnected)
  print("EventOnClientConnected")
  var spawnTms : array<float3x4>
  query() <| $ [es(REQUIRE=respbase)] (transform : float3x4)
    spawnTms |> push(transform)

  var spawnTm : float3x4
  assert(length(spawnTms) > 0, "No spawn bases to spawn onto")
  var idx : int = 0 //grnd() % length(spawnTms)
  if (idx < length(spawnTms))
    spawnTm = spawnTms[idx]

  createEntity("phys_global_state")
  let eid : EntityId = createEntitySync("character_controller") <| $(var init)
    init |> set("transform", spawnTm)
    init |> set("phys__pos", spawnTm[3] - float3(0.f, 1.f, 0.f))
    init |> set("phys__rot", float4(DagorMath::quat(spawnTm)))

  set_scene_camera_entity(eid)

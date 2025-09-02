require ecs
require BloodPuddles
require app // dgs_get_game_params
require DagorDataBlock
require danetlibs.renderer.includes.render_events
require danetlibs.blood_puddles.blood_puddles_events
require RendInst
require DagorMath
require Dacoll
require GamePhys
require PhysMat
require DagorRandom


[es(tag=render, on_event=OnLevelLoaded, track=(render_settings__bare_minimum, render_settings__fullDeferred))]
def blood_puddles_on_level_loaded_es(evt : Event;
                                     render_settings__bare_minimum : bool;
                                     render_settings__fullDeferred : bool)
  let useBloodPuddles = (!render_settings__bare_minimum &&
                         render_settings__fullDeferred &&
                         dgs_get_game_params() |> datablock_getBool("use_blood_puddles", true))
  if !useBloodPuddles
    close_blood_puddles_mgr()
    return
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr == null
    init_blood_puddles_mgr()


[es(tag=render, on_event=UnloadLevel)]
def blood_puddles_unload_level_es(evt : Event)
  close_blood_puddles_mgr()


[es(tag=render, no_order)]
def blood_puddles_update_es(info : UpdateStageInfoAct)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr != null
    *bloodPuddlesMgr |> update()


[es(tag=render, after=animchar_before_render_es)] //require for execute animchar_before_render_es as early as possible
def blood_puddles_before_render_es(info : UpdateStageInfoBeforeRender)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr != null
    *bloodPuddlesMgr |> beforeRender()


[es(tag=render)]
def blood_puddles_splash_emmiter_es(evt : BloodPuddlesSplashEmmiterEvent)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr == null
    return
  *bloodPuddlesMgr |> addSplashEmitter(evt.start_pos, evt.target_pos, evt.normal, evt.dir, evt.size, evt.ri_desc_handle, evt.grav)


[es(tag=render)]
def blood_puddles_spray_decal_es(evt : BloodPuddlesSprayDecalEvent)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr == null
    return
  *bloodPuddlesMgr |> put_decal(int(BloodPuddlesGroup.BLOOD_DECAL_GROUP_SPRAY), evt.pos, evt.normal, evt.size, evt.ri_desc_handle, evt.hit_pos, evt.projective)


[es(tag=render)]
def blood_puddles_splash_decal_es(evt : BloodPuddlesSplashDecalEvent)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr == null
    return
  *bloodPuddlesMgr |> put_decal(int(BloodPuddlesGroup.BLOOD_DECAL_GROUP_SPLASH), evt.pos, evt.normal, evt.size, evt.ri_desc_handle, evt.hit_pos, evt.projective)


[es(tag=render)]
def blood_puddles_leak_decal_es(evt : BloodPuddlesLeakDecalEvent)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr == null
    return
  *bloodPuddlesMgr |> put_decal(int(BloodPuddlesGroup.BLOOD_DECAL_GROUP_LEAK), evt.pos, evt.normal, evt.size, evt.ri_desc_handle, evt.hit_pos, evt.projective)

[es(tag=render)]
def blood_puddles_explosion_decal_es(evt : BloodPuddlesExplosionEmmiterEvent)
  var bloodPuddlesMgr = get_blood_puddles_mgr()
  if bloodPuddlesMgr == null
    return
  *bloodPuddlesMgr |> put_decal(int(BloodPuddlesGroup.BLOOD_DECAL_GROUP_EXPLOSION), evt.pos, evt.normal, evt.size, evt.ri_desc_handle, evt.hit_pos, evt.projective)
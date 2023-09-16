from "%darg/ui_imports.nut" import *
from "dagor.fs" import read_text_from_file//, file_exists
//from "dagor.localize" import loc
from "dagor.workcycle" import defer
let cursors = require("samples_prog/_cursors.nut")
let {mkLegalWindow, txtBtn, txt, textArea} = require("legal-lib-ui.nut")
let {loginData, mkLoginWindow} = require("login.nut")
let {parse_json, loadJson, saveJson} = require("%sqstd/json.nut")
let {
  requestCurVersions,
  mkLegalsTextRequest,
  confirmVersionsOnServer,
  legals_url,
  parse_legal_config
} = require("legal-lib.nut")

//NOTE: this part is most likely be changed in real game
const pathToLocalCache = "confirmedVer.json" //note - this part should be unique for user - at least save in my documents on PC (and in correspodning places in other platforms)
let localCache = Watched(loadJson(pathToLocalCache) ?? {}) //in real game scenarion her should be some different cach - already existing local storage
let function updateLocalCacheVersions(versions) {
  let data = {legal_versions = versions, user_id = loginData.value?.user_id ?? "unknown_user"}
  saveJson(pathToLocalCache, data)  // in real game scenario you want to override only part of local cache
  localCache(data)
}
//end of part to be changed

let localCacheConfirmedVersions = Computed(@() localCache.value?.legal_versions ?? {})
let isNewUserByCache = Computed(@() localCacheConfirmedVersions.value.len()==0) //this is not correct way. Not existing cache at all is correct way!


let {legal_docs_fall_back, legal_docs_ids} = parse_legal_config(loadJson("samples_prog/legal/legal_config.json", {logger=dlog, logerr=logerr}))

let remoteConfirmedVersions = Computed(function() {
  let doc = loginData.value?.meta.doc ?? {}
  let t = type(doc)
  try {
    return t == "string"
      ? parse_json(doc)
       : doc == null
         ? null
         : doc
   }
   catch (e) {
     log(e)
     logerr("error on parsing legal doc")
     return null
   }
})

let curVersions = Watched({})
let getCurVersionsFailed = Watched(false)
let {areTextsReceived, areTextsFailed, legal_docs_status, requestLegalTexts} = mkLegalsTextRequest(legal_docs_ids)
let isManuallyOpen = mkWatched(persist, "isManuallyOpen", false)

let isNewUser = Computed(@() (loginData.value!=null && (remoteConfirmedVersions.value ?? {}).len() == 0) || isNewUserByCache.value)

let onLoginBtnPressed = requestCurVersions({
  ids = legal_docs_ids,
  onSuccess = @(versions) curVersions(versions),
  onFail = @(_) getCurVersionsFailed(true)
})

let isEqual = function(vers1, vers2) { //shallow compare
  if (type(vers1) != type(vers2))
    return false
  if (vers1.len() != vers2.len())
    return false
  foreach(k, v in vers1){
    if (vers2?[k] != v)
      return false
  }
  return true
}

let acceptLegalsOnServerForCurUser = function(versions) {
  let {user_id = null, jwt=null} = loginData.value
  if (jwt == null || user_id==null) {
    logerr("no jwt for userid")
    return
  }
  confirmVersionsOnServer(versions, jwt, user_id, @() updateLocalCacheVersions(versions))
  defer(function() {
    if (loginData.value==null)
      return
    loginData.mutate(function(v) {
      if ("meta" not in v)
        v.meta <- {}
      if ("doc" not in v.meta)
        v.meta.doc <- {}
      v.meta.doc.__update(versions)
    })
  })
}
let needSilentSync = keepref(Computed(function(){
  if (curVersions.value.len()==0 || loginData.value==null)
    return false
  if (isEqual(remoteConfirmedVersions.value, curVersions.value))
    return false
  if (localCache.value?.user_id != loginData.value?.user_id)
    return false
  if (isEqual(localCacheConfirmedVersions.value, curVersions.value))
    return true
  return false
}))

remoteConfirmedVersions.subscribe(function(vers){
  if (vers && vers.len()>0 && isEqual(curVersions.value, vers) && !isEqual(vers, localCacheConfirmedVersions.value))
    updateLocalCacheVersions(vers)
})
needSilentSync.subscribe(@(...) acceptLegalsOnServerForCurUser(localCacheConfirmedVersions.value))

let notConfirmed = Computed(function() {
  if (loginData.value == null || needSilentSync.value || curVersions.value.len()==0)
    return false
  if (!isEqual(curVersions.value, remoteConfirmedVersions.value))
    return true
})

let user_language_code = "ru"
const fb_language_code = "en"
let needRequest = keepref(Computed(@() (notConfirmed.value || isManuallyOpen.value) && !getCurVersionsFailed.value))
let requestTexts = @(v) v ? requestLegalTexts(user_language_code, fb_language_code) : null
needRequest.subscribe(requestTexts)
requestTexts(needRequest.value)

let needApprove = Computed(function(){
  if (areTextsReceived.value && notConfirmed.value)
    return true
  if (isNewUser.value && (areTextsFailed.value || getCurVersionsFailed.value)) { //if you want to show it in any case befor login - just if isNewUser
      return true
  }
  return false
})

let useFallBack = Computed(@() (loginData.value == null && getCurVersionsFailed.value)
  || (isNewUser.value && (areTextsFailed.value || getCurVersionsFailed.value))
  || (isManuallyOpen.value && areTextsFailed.value)
)

let legal_docs_status_final = Computed(function(){
  if (useFallBack.value ) {
    return legal_docs_status.value.map(function(v){
      let {id, status} = v
      return {
        id
        text = read_text_from_file($"samples_prog/legal/{id}.txt")
        status
        loaded = true
      }
    })
  }
  return clone legal_docs_status.value
})

let legal_window = mkLegalWindow({
  isManuallyOpen,
  legals_url,
  legal_docs_status = legal_docs_status_final,
  isNewUser,
  needApprove,
  onDetach = function(){
    legal_docs_status.mutate(function(v) {
      foreach (i in v) {
        i.loaded = false
        i.text = null //to free memory
      }
    })
  },
  onConfirm = function() {
    let versions = useFallBack.value ? legal_docs_fall_back : curVersions.value
    acceptLegalsOnServerForCurUser(versions)
  },
  onCancel = @() loginData(null)
})
let dbgTextStyle = freeze({size = [sw(10), SIZE_TO_CONTENT], fontSize = hdpx(15)})
let mkDbgText = function(watch, name = null){
  return @() {
    watch
    flow = FLOW_VERTICAL
    children = [
      name != null  && watch.value.len() ? txt(name) : null
      textArea("; ".join(watch.value?.reduce(@(res, ver, id) res.append($"{id}:{ver}"), []) ?? []), dbgTextStyle)
    ]
  }
}

let devBtnsWnd = {
  gap = hdpx(5)
  flow = FLOW_VERTICAL
  hplace = ALIGN_BOTTOM
  children = [
    mkLoginWindow(onLoginBtnPressed)
    txtBtn("reset remote ver", @() acceptLegalsOnServerForCurUser(legal_docs_fall_back.map(@(_) 2)))
    txtBtn("reset local ver", @() updateLocalCacheVersions({}))
    txtBtn("toggleManual", @() isManuallyOpen(!isManuallyOpen.value))
    txtBtn("toggle fail versions", @() getCurVersionsFailed(!getCurVersionsFailed.value))
    txtBtn("set newUser", @() localCache({}))
    @() {watch = isNewUser, children = isNewUser.value ? txt("here is new user") : txt("here is old user")}
    mkDbgText(curVersions, "cur versions:")
    mkDbgText(remoteConfirmedVersions, "confirmed versions:")
  ]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  size = flex()
  cursor = cursors.normal

  children = [
    legal_window
    devBtnsWnd
  ]
}
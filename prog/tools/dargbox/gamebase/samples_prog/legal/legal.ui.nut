from "dagor.fs" import read_text_from_file//, file_exists
from "%darg/ui_imports.nut" import *
let { requestCurVersion, requestDocument, loadConfig, languageCodeMap, requestRemoteConfirmedVersions, confirmRemoteVersion } = require("legal_lib.nut")
let { requestLogin, loginData } = require("samples_prog/web_login/web_login.nut")

// code to implement in game
/*
  how it supposed to work
  1. after login
    - read current version which user agreed with from whatever online storage
    - compare it to requested version from legals.gaijin.net
    - if differs
      - request text, when received - show 'legals updated' screen, and update online storage version of confirmed
  2. before login or on manual open
    add 'show legals button'
    on login screen - request text with reasonable timeout (10 seconds), and show 'loading'
      if it is received - show received, and update version to save in online storage after login
      if not - show fallback version
  3. for new user:
    after login store version of confirmed docs in online storage

  NOTE: on Legals screen add url text (and optionally QR code)
  NOTE2: we have only open manually codepath here, no notification case
*/
let { txt, textArea, txtBtn, makeVertScroll } = require("legal_ui_lib.nut")

let language = Watched("English")

let curLangLegalConfig = Watched({version = null, filePath=null})
const LEGALS_CONFIG_PATH = "samples_prog/legal/legal_config.json"
const ENGLISH_FALLBACK_TEXT = "samples_prog/legal/eula.txt"
const PROJECT = "wt"
loadConfig(LEGALS_CONFIG_PATH, language, curLangLegalConfig)

let legalsText = Watched(null)
let openLegalsManually = Watched(false)
let remoteVersion = Watched(null)
let confirmedVersions = Watched(null)
let remoteTextLoaded = Watched(false)
requestCurVersion(language.get(), @(v) remoteVersion.set(v?.result[languageCodeMap?[language.get()]]))

let openLegalsBtn = txtBtn("toggle show legals", @() openLegalsManually.set(!openLegalsManually.get()))

loginData.subscribe(function(data){
  let {jwt=null} = data
  if (jwt==null)
    return
  requestRemoteConfirmedVersions(jwt, PROJECT, function(versions){
    if (versions!=null)
      confirmedVersions.set(versions)
  })
})

function onFailReqTxt(...){
  let fbFilePath = curLangLegalConfig.get()?.filePath
  remoteTextLoaded.set(false)
  try{
    legalsText.set(read_text_from_file(fbFilePath))
  }
  catch(e) {
    log(e)
    legalsText.set("Error loading text, please follow url to read End User License Agreement")
    try {
      legalsText.set(read_text_from_file(ENGLISH_FALLBACK_TEXT))
    }
    catch(e2){
      log(e2)
    }
  }
}

function onSuccessReqTxt(v) {
  legalsText.set(v)
  remoteTextLoaded.set(true)
}

let onAttachLegalsScreen = @() requestDocument(language.get(), onSuccessReqTxt, onFailReqTxt)

let closeBtn = txtBtn("x", @() openLegalsManually.set(false))
function legalsScreen(){
  return {
    padding = sh(5) size = flex()
    watch = legalsText
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    onAttach = onAttachLegalsScreen
    onDetach = @() legalsText.set(null)
    children = legalsText.get()==null ? txt("Loading...") : makeVertScroll(textArea(legalsText.get()))
  }
}
let needNotification = keepref(Computed(function(){
  if (loginData.get()==null
      || remoteVersion.get() == null
      || confirmedVersions.get() == null
      || confirmedVersions.get().contains(remoteVersion.get())
      || curLangLegalConfig.get()?.version == remoteVersion.get()
   )
    return false
  return true
}))
needNotification.subscribe(function(v) {
  if (v)
    openLegalsManually.set(true)
})

function confirmCurVersion(){
  if (remoteVersion.get()==null || confirmedVersions.get()?.contains(remoteVersion.get()))
    return
  confirmedVersions.set([remoteVersion.get()].extend(confirmedVersions.get() ?? []))
  let {jwt=null} = loginData.get()
  if (jwt==null)
    return
  if (remoteTextLoaded.get())
    confirmRemoteVersion(jwt,"wt", remoteVersion.get(), @(v) dlog("confirm success", v), dlog)
  else {
    let ver = curLangLegalConfig.get()?.version
    if (ver!=null)
      confirmRemoteVersion(jwt,"wt", ver)
  }
}

let loginBtn = txtBtn("login", @() requestLogin())

return {
  size = flex()
  flow = FLOW_VERTICAL
  gap = sh(2)
  padding = hdpx(10)
  children = [
    openLegalsBtn
    loginBtn
    @() {watch = curLangLegalConfig children = txt($"fallback version: {curLangLegalConfig.get().version}") hplace = ALIGN_LEFT}
    @() {watch = remoteVersion children = txt($"remote version: {remoteVersion.get()}") hplace = ALIGN_LEFT}
    @() {watch = confirmedVersions children = txt($"curuser versions: {confirmedVersions.get()?.len()};{"".join(confirmedVersions.get() ?? [])}") hplace = ALIGN_LEFT}
    @() {
      watch = openLegalsManually
      size = openLegalsManually.get() ? flex() : null
      children = openLegalsManually.get() ? {
        onDetach = confirmCurVersion
        flow = FLOW_VERTICAL
        size = flex()
        halign = ALIGN_CENTER
        rendObj = ROBJ_SOLID
        color = Color(10,10,10)
        padding = hdpx(10)
        children = [
          {children = closeBtn hplace = ALIGN_RIGHT}
          txt("See more on https://legals.gaijin.net")
          legalsScreen
        ]
      } : null
    }
  ]
}
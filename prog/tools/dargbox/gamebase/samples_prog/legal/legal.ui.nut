from "%darg/ui_imports.nut" import *

let {
  requestCurVersion, requestDocument, loadConfig,
  languageCodeMap, requestRemoteConfirmedVersions,
  confirmRemoteVersion
} = require("legal_lib.nut")
let {requestLogin, loginData} = require("samples_prog/web_login/web_login.nut")

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
let { txt, textArea, txtBtn, makeVertScroll} = require("legal_ui_lib.nut")
let {
  read_text_from_file
//  ,file_exists
} = require("dagor.fs")

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
requestCurVersion(language.value, @(v) remoteVersion(v?.result[languageCodeMap?[language.value]]))

let openLegalsBtn = txtBtn("toggle show legals", @() openLegalsManually(!openLegalsManually.value))

loginData.subscribe(function(data){
  let {jwt=null} = data
  if (jwt==null)
    return
  requestRemoteConfirmedVersions(jwt, PROJECT, function(versions){
    if (versions!=null)
      confirmedVersions.set(versions)
  })
})

let function onFailReqTxt(...){
  let fbFilePath = curLangLegalConfig.value?.filePath
  remoteTextLoaded.set(false)
  try{
    legalsText.update(read_text_from_file(fbFilePath))
  }
  catch(e) {
    log(e)
    legalsText.update("Error loading text, please follow url to read End User License Agreement")
    try {
      legalsText.update(read_text_from_file(ENGLISH_FALLBACK_TEXT))
    }
    catch(e2){
      log(e2)
    }
  }
}

function onSuccessReqTxt(v) {
  legalsText(v)
  remoteTextLoaded.set(true)
}

let onAttachLegalsScreen = @() requestDocument(language.value, onSuccessReqTxt, onFailReqTxt)

let closeBtn = txtBtn("x", @() openLegalsManually(false))
let function legalsScreen(){
  return {
    padding = sh(5) size = flex()
    watch = legalsText
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    onAttach = onAttachLegalsScreen
    onDetach = @() legalsText.update(null)
    children = legalsText.value==null ? txt("Loading...") : makeVertScroll(textArea(legalsText.value))
  }
}
let needNotification = keepref(Computed(function(){
  if (loginData.value==null
      || remoteVersion.value == null
      || confirmedVersions.value == null
      || confirmedVersions.value.contains(remoteVersion.value)
      || curLangLegalConfig.value?.version == remoteVersion.value
   )
    return false
  return true
}))
needNotification.subscribe(function(v) {
  if (v)
    openLegalsManually(true)
})

let function confirmCurVersion(){
  if (remoteVersion.value==null || confirmedVersions.value?.contains(remoteVersion.value))
    return
  confirmedVersions.set([remoteVersion.value].extend(confirmedVersions.value ?? []))
  let {jwt=null} = loginData.value
  if (jwt==null)
    return
  if (remoteTextLoaded.value)
    confirmRemoteVersion(jwt,"wt", remoteVersion.value, @(v) dlog("confirm success", v), dlog)
  else {
    let ver = curLangLegalConfig.value?.version
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
    @() {watch = curLangLegalConfig children = txt($"fallback version: {curLangLegalConfig.value.version}") hplace = ALIGN_LEFT}
    @() {watch = remoteVersion children = txt($"remote version: {remoteVersion.value}") hplace = ALIGN_LEFT}
    @() {watch = confirmedVersions children = txt($"curuser versions: {confirmedVersions.value?.len()};{"".join(confirmedVersions.value ?? [])}") hplace = ALIGN_LEFT}
    @() {
      watch = openLegalsManually
      size = openLegalsManually.value ? flex() : null
      children = openLegalsManually.value ? {
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
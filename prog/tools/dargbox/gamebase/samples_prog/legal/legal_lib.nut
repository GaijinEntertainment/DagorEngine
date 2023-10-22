from "%darg/ui_imports.nut" import *

let {read_text_from_file, file_exists} = require("dagor.fs")
let {HTTP_ABORTED, HTTP_FAILED, HTTP_SUCCESS, httpRequest} = require("dagor.http")
let {parse_json} = require("json")

let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}

let logerrCache = persist("logerrCache", @() {})

let function logerrOnce(text, key = null){
  let k = key ?? text
  if (k in logerrCache) {
    log(text)
    return
  }
  logerr(text)
  logerrCache[k] <- true
}

let function mkCb(url, onSuccess, onFail = null, isJson = true){
  return function(response){
    try{
      let status = response.status
      let sttxt = statusText?[status]
      log($"http status for '{url}' = {sttxt}")
      if (status != HTTP_SUCCESS || response?.http_code!=200) {
        throw($"http error {url}, status = {sttxt}")
      }
      if (isJson) {
        let resp = parse_json(response.body.as_string())
        if (resp.status != "OK")
          throw($"couldn't receive status for {url}, status = {resp.status}")
        onSuccess(resp)
      }
      else {
        onSuccess(response.body.as_string())
      }
    }
    catch(err) {
      log("FAIL on", url)
      logerrOnce(err)
      onFail?(err)
    }
  }
}

let function httpGetText(url, onSuccess, onFail = null){
  httpRequest({ method = "GET", url, callback = mkCb(url, onSuccess, onFail, false)})
}
let function httpGetJson(url, onSuccess, onFail = null){
  httpRequest({ method = "GET", url, callback = mkCb(url, onSuccess, onFail, true)})
}
const DOC_ID = "ingame-notice"
const legals_url = "https://legal.gaijin.net"
let languageCodeMap = {
  "English":"en",
  "Russian":"ru",
  "Polish": "pl",
  "German": "de",
  "Czech": "cz",
  "French":"fr",
  "Spanish":"es",
  "Italian":"it",
  "Portugese":"pt",
  "Turkish":"tr",
  "Koreana":"ko",
  "Chinese":"zh",
  "TChinese":"zh",
  "Japanese":"ja",
  "Ukraine":"ua"
}

function requestCurVersions(lang, onSuccess, onFail = null) {
  let lang_code = languageCodeMap?[lang]
  let langstr = lang_code !=null ? $"&lang={lang_code}" : ""
  httpGetJson($"{legals_url}/api/v2/documents?token={DOC_ID}{langstr}", onSuccess, onFail)
}

function requestCurVersion(lang, onSuccess, onFail = null) {
  let lang_code = languageCodeMap?[lang]
  let langstr = lang_code !=null ? $"?lang={lang_code}" : ""
  httpGetJson($"{legals_url}/api/v2/document/{DOC_ID}{langstr}", onSuccess, onFail)
}

let function requestDocument(lang, onSuccess, onFail=null) {
  let lang_code = languageCodeMap?[lang]
  let lang_code_url = lang!=null ? $"{lang_code}/" : ""
  httpGetText($"{legals_url}/{lang_code_url}/{DOC_ID}?format=plain", onSuccess, onFail)
}

let function loadConfig(filePath, language, curLangLegalConfig) {
  try{
    let config = file_exists(filePath) ? parse_json(read_text_from_file(filePath)) : null
    local curLang = language.value.tolower()
    if (!(curLang in config))
      curLang = "english"
    curLangLegalConfig.update({
      version = config?[curLang]?.version
      filePath = config?[curLang]?.file
    })
  }
  catch(e){
    logerrOnce(e)
    curLangLegalConfig.update({version=null, filePath=null})
  }
}

function confirmRemoteVersion(jwt, project, version, onSuccess = null, onFail=null){
  const url = "https://legal.gaijin.net/api/v2/accept"
  httpRequest({
    method = "POST", url,
    json = {"code": DOC_ID, project, version},
    callback = mkCb(url, onSuccess, onFail, true),
    headers = {Authorization = $"Bearer {jwt}"}
  })
}

function requestRemoteConfirmedVersions(jwt, project, onSuccess, onFail=null){
  let url = $"https://legal.gaijin.net/api/v2/accept/{DOC_ID}?project={project}"
  httpRequest({
    method = "GET", url,
    callback = mkCb(url, @(res) onSuccess(res?.result[project][DOC_ID]), onFail, true)
    headers = {Authorization = $"Bearer {jwt}"}
  })
}

return {
  requestCurVersion, requestDocument,
  logerrOnce, loadConfig, languageCodeMap,
  requestRemoteConfirmedVersions, confirmRemoteVersion

}
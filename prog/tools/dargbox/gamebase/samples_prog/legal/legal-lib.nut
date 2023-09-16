from "%darg/ui_imports.nut" import *
//from "dagor.fs" import file_exists, read_text_from_file
let {HTTP_ABORTED, HTTP_FAILED, HTTP_SUCCESS, httpRequest} = require("dagor.http")
let {parse_json} = require("json")

const legals_url = "https://legal.gaijin.net"

let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}

/*
  legal_docs_status: observable with {id = {id, text=null, loaded=false}}
*/

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

let requestCurVersions = kwarg(function(ids, onSuccess, onFail = null){
  let filters = ",".join(ids)
  let url = $"{legals_url}/api/v1/getversions?filter={filters}"

  log($"http request {filters}")
  let callback = function(response){
    try{
      let status = response.status
      let sttxt = statusText?[status]
      log($"http status for '{url}' = {sttxt}")
      if (status != HTTP_SUCCESS || response?.http_code!=200) {
        throw($"http error {url}, status = {sttxt}")
      }
      let resp = parse_json(response.body.as_string())
      if (resp.status != "OK")
        throw($"couldn't receive status for {url}, status = {resp.status}")
      onSuccess?(resp.result)
    }
    catch(err) {
      logerrOnce(err)
      onFail?(err)
    }
  }
  httpRequest({ method = "GET", url, callback })
})

let LEGAL_TEXTS_STATUS = freeze({
  NOT_REQUESTED = "NOT_REQUESTED"
  REQUESTED = "REQUESTED"
  RECEIVED = "RECEIVED"
  RECEIVED_FALLBACK_LANG = "RECEIVED_FALLBACK_LANG"
  FAILED = "FAILED"
})

let function mkLegalsTextRequest(ids) {
  let legal_docs_status = Watched(ids.map(@(id) {id, text=null, loaded=false, status = LEGAL_TEXTS_STATUS.NOT_REQUESTED}))
  let areTextsReceived = Computed(@() legal_docs_status.value.reduce(@(res, v) res && v.loaded, true))
  let areTextsFailed = Computed(@() legal_docs_status.value.reduce(@(res, v) res && (LEGAL_TEXTS_STATUS.FAILED == v.status), true))
  const ErrorOnGettingDoc = "errorOnGettingDoc"
  let update_status_text = function(id, text, status) {
    legal_docs_status.mutate(function(v){
      foreach (entry in v) {
        if (id == entry.id) {
          entry.text = text
          entry.loaded = [LEGAL_TEXTS_STATUS.RECEIVED, LEGAL_TEXTS_STATUS.RECEIVED_FALLBACK_LANG].contains(status)
          entry.status = status
          break
        }
      }
    })
  }
  let function mkCbOnLoadDoc(id, url, fb_url = null) {
    let fb_cb = fb_url ? callee()(id, fb_url) : null
    return function(response) {
      try {
        let status = response.status
        let sttxt = statusText?[status]
        log($"http status for '{url}' = {sttxt}")
        if (status != HTTP_SUCCESS || response?.http_code!=200) {
          log($"http error status for {url} = {sttxt}")
          throw($"http error status for {url} = {sttxt}")
        }
        let text = response.body.as_string()
        update_status_text(id, text, response.context.status)
      }
      catch(err) {
        log($"error loading doc {url}, {err}")
        logerrOnce(ErrorOnGettingDoc, $"Error loading legal doc {id}, {url}")
        if (fb_cb)
          httpRequest({method = "GET", url=fb_url, callback = fb_cb, context = {status = LEGAL_TEXTS_STATUS.RECEIVED_FALLBACK_LANG}})
        else {
          log($"failed receive id={id}, url = {url}", err)
          update_status_text(id, null, LEGAL_TEXTS_STATUS.FAILED)
        }
      }
    }
  }

  // note: beware not to store docs all the times as they are huge in size
  let function requestLegalTexts(language_code, fb_lang_code=null) {
    let lang = language_code!=null ? $"{language_code}/" : ""
    legal_docs_status.mutate(function(v) {
      foreach(i in v) {
        if (!i.loaded) {
          let {id} = i
          let url = $"{legals_url}/{lang}{id}?format=plain"
          let fb_url = fb_lang_code != null ? $"{legals_url}/{fb_lang_code}/{id}?format=plain" : $"{legals_url}/{id}?format=plain"
          httpRequest({method = "GET", url, context = {status = LEGAL_TEXTS_STATUS.RECEIVED}, callback = mkCbOnLoadDoc(id, url, fb_url!=url ? fb_url : null)})
          i.status = LEGAL_TEXTS_STATUS.REQUESTED
        }
      }
    })
  }
  return {areTextsReceived, areTextsFailed, legal_docs_status, requestLegalTexts, LEGAL_TEXTS_STATUS}
}

const CONFIRM_LEGALS_URL  = "https://auth.gaijinent.com/accept_legal.php"

let function confirmVersionsOnServer(versions, jwt, userId, cb=null){
  let url = CONFIRM_LEGALS_URL
  let function callback(response) {
    try {
      let status = response.status
      let sttxt = statusText?[status]
      log($"http status for '{url}' = {sttxt}")
      if (status != HTTP_SUCCESS || response?.http_code!=200) {
        log($"http error status for {url} = {sttxt}")
        throw($"http error status for {url} = {sttxt}")
      }
      let res = parse_json(response.body.as_string())
      log($"accept versions result, userId = {userId}", res)
      if (res?.status != "OK"){
        log("Error on accept:", res?.error, res)
        throw res
      }
      log(res)
      cb?()
    }
    catch(err) {
      log($"error accept version userId = {userId}", err)
      logerrOnce("Error on accept legal docs")
    }
  }
  httpRequest({method = "POST",
    url,
    callback,
    headers = {"Content-Type": "application/json", "Authorization": $"Bearer {jwt}"}
    json = versions
  })
}

let function parse_legal_config(config){
  assert(type(config)=="table", "legal config has not valid format, should be table")
  assert("legal_docs_fall_back" in config && "legal_docs_ids" in config, "config should have 'legal_docs_fall_back' and 'legal_docs_ids' fields. 'legal_docs_fall_back' id:version and 'legal_docs_ids' Array if string ids <ids>")
  let {legal_docs_fall_back, legal_docs_ids} = config
  assert(type(legal_docs_fall_back)=="table", "'legal_docs_fall_back' in config is not a table in 'id:version' format")
  assert(type(legal_docs_ids)=="array", "'legal_docs_ids' in config is not a array of string ids")
  const msg ="'legal_docs_fall_back' should have the same all of fields in 'legal_docs_ids'"
  assert(legal_docs_fall_back.len()>=legal_docs_ids.len(), msg)
  foreach (id in legal_docs_ids)
    assert(id in legal_docs_fall_back, msg)

  return freeze(config)
}

return {
  statusText
  requestCurVersions
  mkLegalsTextRequest
  confirmVersionsOnServer
  legals_url
  parse_legal_config
  logerrOnce
}
from "%darg/ui_imports.nut" import *
from "system" import system
let {HTTP_ABORTED, HTTP_FAILED, HTTP_SUCCESS, httpRequest} = require("dagor.http")
let {parse_json} = require("json")

//See information here https://info.gaijin.lan/pages/viewpage.action?pageId=72075038
//NOTE: better implement digital signature
let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}

//const url ="https://auth.gaijinent.com/login.php/proxy/api/gameSession/getGsid"
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
        onSuccess(resp)
      }
      else {
        onSuccess(response.body.as_string())
      }
    }
    catch(err) {
      log("FAIL on", url)
      logerr(err)
      onFail?(err)
    }
  }
}

let function httpPostJson(url, onSuccess, onFail = null, data=null){
  httpRequest({ method = "POST", url, callback = mkCb(url, onSuccess, onFail, true), data})
}

const open_session_url ="https://login.gaijin.net/proxy/api/gameSession/getGsid"
const get_session_resut_url = "https://login.gaijin.net/proxy/api/gameSession/getGsidResult"

let { loginData, gsidData } = function mkLogin(){
  let gsid = Watched(null)
  local time_wating = 0
  local timeToNextReq = 5
  const maxWaitingTime = 121
  let loginResult = Watched(null)
  local getResults
  getResults = @() httpPostJson(get_session_resut_url, function onSuccess(v){
      if (v.status == "OK")
        loginResult({token = v.token, started=v.started, user_id=v.user_id, tags=v.tags, token_exp =v.token_exp, jwt=v.jwt, country=v.country, reg_time=v.reg_time, nick=v.token, ts=v.ts, lang=v.lang})
      if (v.status == "RETRY") {
        time_wating += timeToNextReq
        timeToNextReq = v?.delay ?? 5
        if (time_wating < maxWaitingTime)
          gui_scene.resetTimeout(timeToNextReq, getResults)
        else
          gsid(null)
      }
    }, null, {gsid=gsid.value})

  let function stopReqResults(){
      gui_scene.clearTimer(getResults)
      time_wating = 0
      timeToNextReq = 2
  }
  gsid.subscribe(function(v){
    if (v==null) {
      stopReqResults()
      return
    }
    //shell_execute({cmd=$"rundll32 url.dll,FileProtocolHandler https://login.gaijin.net/?gsid={v}" params=$"https://login.gaijin.net/?gsid={v}" dir="."})
    system($"rundll32 url.dll,FileProtocolHandler https://login.gaijin.net/?gsid={v}" )
    if (loginResult.value==null)
      gui_scene.resetTimeout(timeToNextReq, getResults)
  })

  loginResult.subscribe(function(v){
    if (v != null)
      stopReqResults()
  })
  return {gsidData = gsid, loginData=loginResult}
}()

function requestLogin(onSuccess = null, onFail=null){
  httpPostJson(open_session_url, function(v) {
    if (v.status!="OK") {
      log("failed to login, {v.status}")
      onFail?(v)
    }
    else {
      gsidData(v?.gsid)
      onSuccess?(v)
    }
  })
}

return {requestLogin, loginData, gsidData}
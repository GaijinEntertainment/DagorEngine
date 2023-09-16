from "%darg/ui_imports.nut" import *
let {httpRequest, HTTP_SUCCESS} = require("dagor.http")
let {parse_json} = require("%sqstd/json.nut")
let {statusText} = require("legal-lib.nut")
let {txtBtn, txt} = require("legal-lib-ui.nut")
let textInput = require("samples_prog/_basic/components/textInput.nut")

const login_url = "https://auth.gaijinent.com/login.php"

let password = mkWatched(persist, "password", "")
let two_step = mkWatched(persist, "two_step", "")
let login_id = mkWatched(persist, "login_id", "")
let loginData = mkWatched(persist, "loginData", null)

let function loginAndGetConfirmedVersions(onSuccess = null){
  let url = login_url
  let function callback(response) {
    try {
      let status = response.status
      let sttxt = statusText?[status]
      log($"http status for '{url}' = {sttxt}")
      if (status != HTTP_SUCCESS || response?.http_code!=200) {
        log($"http error status for {url} = {sttxt}")
        throw($"http error status for {url} = {sttxt}")
      }
      log(response.body.as_string())
      let res = parse_json(response.body.as_string())

      if (res?.status!="OK"){
        dlog("Error:", res?.error)
        throw res.status
      }
      if (type(res.meta?.doc)=="string")
        res.meta.doc = parse_json(res.meta.doc)
      loginData(res)
      log("login successfull!")
      onSuccess?(res.meta.doc ?? {})
    }
    catch(err) {
      dlog($"error login {login_id.value}: {err}")
      logerr($"Error login: {login_id.value}")
    }
  }
  httpRequest({method = "POST",
    url,
    callback,
    //headers = {"Content-Type":"application/x-www-form-urlencoded"}
    data = "&".join({
      meta = 1,
      client = "TestClient"
      login = login_id.value
      password = password.value
    }.__update(two_step.value != "" ? {["2step"] = two_step.value} : {}).reduce(@(acc, val, field) acc.append($"{field}={val}"), []))
  })
}

let mkLoginWindow = @(onLoginBtnPressed=null) @() {
  flow = FLOW_VERTICAL
  watch = [loginData]
  size = [sw(10), SIZE_TO_CONTENT]
  rendObj = ROBJ_SOLID
  color = Color(20,20,20)
  children = loginData.value != null ? txt($"logged in {login_id.value}, remoteConfirmedVersions: {loginData.value?.meta.doc}") :[
    textInput(login_id, {placeholder = "login"})
    textInput(password, {placeholder = "password", password="*"})
    textInput(two_step, {placeholder = "two step"})
    txtBtn("login", function() {onLoginBtnPressed?(); loginAndGetConfirmedVersions()})
  ]
}

return {
  loginData
  loginAndGetConfirmedVersions
  mkLoginWindow
}
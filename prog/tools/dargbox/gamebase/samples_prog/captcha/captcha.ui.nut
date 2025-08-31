from "dagor.http" import HTTP_ABORTED, HTTP_FAILED, HTTP_SUCCESS, httpRequest
from "json" import parse_json, object_to_json_string
from "%darg/ui_imports.nut" import *
//consider generate captcha in squirrel
let textInput = require("samples_prog/_basic/components/textInput.nut")


let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}

let logerrCache = persist("logerrCache", @() {})

function logerrOnce(text, key = null){
  let k = key ?? text
  if (k in logerrCache) {
    log(text)
    return
  }
  logerr(text)
  logerrCache[k] <- true
}

let captchaData = mkWatched(persist, "captchaData")
const BASE_URL = "https://warthunder.com/external/captcha"

let url_CAPTCHA_GET = $"{BASE_URL}/captcha/json"
let url_CAPTCHA_VERIFY = $"{BASE_URL}//captcha/verify"
let requestCaptcha = function(){
  httpRequest({
    method = "GET",
    url = url_CAPTCHA_GET,
    callback = function(response){
      try{
        let status = response.status
        let sttxt = statusText?[status]
        log($"http status for '{url_CAPTCHA_GET}' = {sttxt}")
        if (status != HTTP_SUCCESS || response?.http_code!=200) {
          throw($"http error {url_CAPTCHA_GET}, status = {sttxt}, http_code = {response?.http_code}")
        }
        let {image, token} = parse_json(response.body.as_string())
        //dlog(token)
        captchaData.set({image, token})
      }
      catch(err) {
        logerrOnce(err)
      }

    }
  })
}
if (captchaData.get()==null)
  requestCaptcha()
let image = function(){
  return {
    watch = captchaData
    rendObj = ROBJ_IMAGE
    size = static [hdpxi(300), hdpxi(120)]
    image = captchaData.get() != null ? Picture($"b64://{captchaData.get().image}.png:256:256:K") : null
  }
}

let testCaptcha = function(code, token){
  dlog("test code", code, "for token", token)
  httpRequest({
    method = "POST",
    url = url_CAPTCHA_VERIFY,
    json = {token, answer=code}
    callback = function(response){
      try{
        let status = response.status
        let sttxt = statusText?[status]
        log($"http status for '{url_CAPTCHA_VERIFY}' = {sttxt}")
        if (status != HTTP_SUCCESS || response?.http_code!=200) {
          throw($"http error {url_CAPTCHA_VERIFY}, status = {sttxt}, http_code = {response?.http_code}")
        }
        let r = parse_json(response.body.as_string())
        dlog(r)
        captchaData.mutate(@(v) v.result <- r.result)
      }
      catch(err) {
        logerrOnce(err)
      }

    }
  })
}
let code = mkWatched(persist, "code", "")
let codeTest = textInput(code, {placeholder = "enter code", onReturn = @() testCaptcha(code.get(), captchaData.get().token)})

return {
  size = flex()
  rendObj = ROBJ_SOLID
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  children = [
    image
    codeTest
  ]
  color = Color(90,50,50)
}
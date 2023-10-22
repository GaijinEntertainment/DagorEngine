from "%darg/ui_imports.nut" import *
from "iostream" import blob

let { httpRequest, HTTP_SUCCESS, HTTP_FAILED, HTTP_ABORTED } = require("dagor.http")
let cursors = require("samples_prog/_cursors.nut")

let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}


let function mkhttpButton(text, request){
  return @() {
    rendObj = ROBJ_SOLID
    color = Color(0,25,205)
    size = [200,50]
    behavior = Behaviors.Button
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    onClick = function() {
      httpRequest(request)
    }
    children = {
      rendObj = ROBJ_TEXT
      text = text
    }
  }
}
let function callback(response){
  vlog($"status = {response?.status}")
  vlog($"http_code = {response?.http_code} ({statusText?[response?.http_code]})")
  vlog($"body = {response?.body?.as_string?()}")
  vlog($"body len = {response?.body?.len?()}")
}

let sampleGetButton = mkhttpButton("GET",{
  method = "GET"
  url = "https://gaijin.net/"
  callback
})

let fatalPostButton = mkhttpButton("FATAL POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  headers = {a=1,b=2}
  callback
})

let sampleFormPostButton = mkhttpButton("FORM POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  data = {a=1,b=2}
  callback
})

let sampleJsonStrPostButton = mkhttpButton("JSON as string POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  json = "{\"a\":1,\"b\":2}"//-forgot-subst
  callback
})

let sampleJsonObjPostButton = mkhttpButton("JSON as obj POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  json = {a=1, b=2}
  callback
})

let blobStream = blob()
blobStream.writen('A', 'c')
blobStream.writen('B', 'c')
blobStream.writen('C', 'c')


let sampleBlobPostButton = mkhttpButton("Blob POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  data = blobStream
  callback
})

let function Root() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    cursor = cursors.normal
    size = flex()
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        flow = FLOW_VERTICAL
        size = flex()
        gap = hdpx(10)
        children = [
          sampleGetButton
          fatalPostButton
          sampleFormPostButton
          sampleJsonStrPostButton
          sampleJsonObjPostButton
          sampleBlobPostButton
        ]
      }
    ]
  }
}

return Root

from "dagor.http" import httpRequest, HTTP_SUCCESS, HTTP_FAILED, HTTP_ABORTED
from "%darg/ui_imports.nut" import *
from "iostream" import blob
let cursors = require("samples_prog/_cursors.nut")

let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}


function mkhttpButton(text, request){
  return @() {
    rendObj = ROBJ_SOLID
    color = Color(0,25,205)
    size = static [200,50]
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
function callback(response){
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
blobStream.writen('A', 'b')
blobStream.writen('B', 'b')
blobStream.writen('C', 'b')


let sampleBlobPostButton = mkhttpButton("Blob POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  data = blobStream
  callback
})

function Root() {
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

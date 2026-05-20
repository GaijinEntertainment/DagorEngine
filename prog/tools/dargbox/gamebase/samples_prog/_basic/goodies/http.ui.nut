from "dagor.http" import httpRequest, httpFetch, HTTP_SUCCESS, HTTP_FAILED, HTTP_ABORTED
from "%darg/ui_imports.nut" import *
from "iostream" import blob
let cursors = require("samples_prog/_cursors.nut")

let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}


function mkButton(text, onClick) {
  return @() {
    rendObj = ROBJ_SOLID
    color = Color(0,25,205)
    size = const [hdpx(300), hdpx(50)]
    behavior = Behaviors.Button
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    onClick
    children = {
      rendObj = ROBJ_TEXT
      text
    }
  }
}

function mkHttpCbButton(text, request) {
  return mkButton(text, function() {
    httpRequest(request)
  })
}

function printResponse(response) {
  vlog("response", response)
  vlog($"status = {response?.status}")
  vlog($"http_code = {response?.http_code} ({statusText?[response?.http_code]})")
  vlog($"body = {response?.body?.as_string?()}")
  vlog($"body len = {response?.body?.len?()}")
}

let sampleGetButton = mkHttpCbButton("GET", {
  method = "GET"
  url = "https://gaijin.net/"
  callback = printResponse
})

let sampleFormPostButton = mkHttpCbButton("FORM POST", {
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  data = {a=1,b=2}
  callback = printResponse
})

let sampleJsonStrPostButton = mkHttpCbButton("JSON as string POST", {
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  json = "{\"a\":1,\"b\":2}"
  callback = printResponse
})

let sampleJsonObjPostButton = mkHttpCbButton("JSON as obj POST", {
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  json = {a=1, b=2}
  callback = printResponse
})

let blobStream = blob()
blobStream.writen('A', 'b')
blobStream.writen('B', 'b')
blobStream.writen('C', 'b')


let sampleBlobPostButton = mkHttpCbButton("Blob POST",{
  method = "POST"
  url = "http://localhost/api/add_results/1122"
  data = blobStream
  printResponse
})

async function fetchGet() {
  try {
    let res = await httpFetch({
      method = "GET"
      url = "https://gaijin.net/"
    })
    printResponse(res)
  } catch (err) {
    vlog("fetch failed", err)
  }
}

async function fetchGetFail() {
  try {
    let res = await httpFetch({
      method = "GET"
      url = "https://127.0.0.1:0/"
    })
    printResponse(res)
  } catch (err) {
    vlog("fetch failed", err)
  }
}

function column(children) {
  return {
    halign = ALIGN_CENTER
    flow = FLOW_VERTICAL
    gap = hdpx(10)
    children
  }
}

function Root() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    cursor = cursors.normal
    size = flex()
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    children = {
      flow = FLOW_HORIZONTAL
      gap = hdpx(10)
      children = [
        column([
          {rendObj=ROBJ_TEXT text="Request with callback"}
          sampleGetButton
          sampleFormPostButton
          sampleJsonStrPostButton
          sampleJsonObjPostButton
          sampleBlobPostButton
        ])
        column([
          {rendObj=ROBJ_TEXT text="Async fetch"}
          mkButton("GET", fetchGet)
          mkButton("GET (invalid)", fetchGetFail)
        ])
      ]
    }
  }
}

return Root

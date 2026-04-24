from "dagor.http" import httpRequest, HTTP_FAILED, HTTP_ABORTED, HTTP_SUCCESS
from "functools.nut" import *
let { Task } = require("monads.nut")
//local dlog = require("log.nut")().dlog
let statusText = freeze({
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
})
function mkHttpCallback(resolveFn, rejectFn, url){
  return tryCatch(
    function(response){
      let {status, http_code, body=null} = response

      let sttxt = statusText?[status]
      println($"http status for '{url}' = {sttxt}")
      if (status != HTTP_SUCCESS || !(http_code==0 || (http_code>=200 && http_code<300))) {
        throw({error=$"http error status = {sttxt}, http_code = {http_code}", http_code, status, body})
      }
      resolveFn(body)
    },
    rejectFn
  )
}
function httpGet(url, resolveFn, rejectFn=null, params=null){
  assert(type(url)=="string", "url should be string")
  assert(type(resolveFn)=="function", "resolveFn should be function that accepts responce body blob object (has 'as_string' method)")
  assert(rejectFn==null || type(rejectFn)=="function", "rejectFn should be null or function that accepts error object that can be string or object with 'status', 'http_code' and 'body' fields, or any other error")
  httpRequest({url, method = "GET", callback = mkHttpCallback(resolveFn, rejectFn, url)}.__update(params ?? {}))
}

function httpPost(url, resolveFn, rejectFn=null, params=null){
  assert(type(url)=="string", "url should be string")
  assert(type(resolveFn)=="function", "resolveFn should be function that accepts responce body blob object (has 'as_string' method)")
  assert(rejectFn==null || type(rejectFn)=="function", "rejectFn should be null or function that accepts error object that can be string or object with 'status', 'http_code' and 'body' fields, or any other error")
  httpRequest({ method = "POST", url, callback=mkHttpCallback(resolveFn, rejectFn, url)}.__update(params ?? {}))
}

function httpFormPost(url, data, resolveFn, rejectFn=null) {
  assert(type(url)=="string", "url should be string")
  assert(type(resolveFn)=="function", "resolveFn should be function that accepts responce body blob object (has 'as_string' method)")
  assert(rejectFn==null || type(rejectFn)=="function", "rejectFn should be null or function that accepts error object that can be string or object with 'status', 'http_code' and 'body' fields, or any other error")
  assert(type(data)=="table", "data should be object with string keys")
  httpRequest({ method = "POST", url, data, callback=mkHttpCallback(resolveFn, rejectFn, url)})
}

function httpJson(url, json, resolveFn, rejectFn=null){
  assert(type(url)=="string", "url should be string")
  assert(type(resolveFn)=="function", "resolveFn should be function that accepts responce body blob object (has 'as_string' method)")
  assert(rejectFn==null || type(rejectFn)=="function", "rejectFn should be null or function that accepts error object that can be string or object with 'status', 'http_code' and 'body' fields, or any other error")
  assert(type(json)=="table" || type(json)=="string", "'json`' should be object or string")
  httpRequest({ method = "POST", url, json, callback=mkHttpCallback(resolveFn, rejectFn, url)})
}


function TaskHttpGet(url) {
  return Task(function(rejectFn, resolveFn) {
    println($"http 'get' requested for '{url}'")
    httpRequest({
      url
      method = "GET"
      callback = tryCatch(
        function(response){
          let {status, http_code} = response

          let sttxt = statusText?[status]
          println($"http status for '{url}' = {sttxt}")
          if (status != HTTP_SUCCESS || !(http_code==0 || (http_code>=200 && http_code<300))) {
            throw($"http error status = {sttxt}")
          }
          resolveFn(response.body)
        },
        rejectFn
      )
    })
  })
}

let UNRESOLVED = persist("UNRESOLVED", @() {})
let RESOLVED = persist("RESOLVED", @() {})
let REJECTED = persist("REJECTED", @() {})

function TaskHttpMultiGet(urls, rejectOne=@(x) x, resolveOne=@(x) x) {
  assert(type(urls) == "array", @() $"expected urls as 'array' got '{type(urls)}'")
  assert(type(rejectOne) == "function" && type(resolveOne) == "function", "incorrect type of arguments")
  return Task(function(rejectFn, resolveFn) {
    let total = urls.len()
    let res = array(total)
    let statuses = array(total, UNRESOLVED)
    local executed = false
    function checkStatus(){
      let rejected = statuses.findindex(@(v) v==REJECTED) != null
      let resolved = !rejected && statuses.filter(@(v) v==RESOLVED).len() == total
      if (resolved && !executed){
        executed = true
        resolveFn(res)
      }
      if (rejected && !executed){
        executed = true
        rejectFn(res)
      }
    }
    foreach (i, u in urls) {
      let id = i
      let url = u
      println($"http requested get for '{url}'")
      httpRequest({
        url
        method = "GET"
        callback = tryCatch(
          function(response){
            let status = response.status
            let sttxt = statusText?[status]
            println($"http status for '{url}' = {sttxt}")
            if (status != HTTP_SUCCESS) {
              throw($"http error status = {sttxt}")
            }
            res[id] = resolveOne(response.body)
            statuses[id] = RESOLVED
            checkStatus()
          },
          function(r) {
            res[id] = rejectOne(r)
            statuses[id] = REJECTED
            checkStatus()
          }
        )
      })
    }
  })
}

return freeze({
  TaskHttpGet
  TaskHttpMultiGet
  httpGet
  httpPost
  httpJson
  httpFormPost
})
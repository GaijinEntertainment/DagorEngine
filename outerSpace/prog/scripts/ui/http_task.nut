from "%darg/ui_imports.nut" import *
from "json" import parse_json

let { httpRequest, HTTP_SUCCESS, HTTP_FAILED, HTTP_ABORTED } = require("dagor.http")

const ASSERT_MSG = "provided value should be Monad"

let statusText = {
  [HTTP_SUCCESS] = "SUCCESS",
  [HTTP_FAILED] = "FAILED",
  [HTTP_ABORTED] = "ABORTED",
}

/*
  monad has interface with
  of, flatMap, map
*/


local Task
Task = class {
  exec = null
  static isMonad = @() true
  constructor(fn){
    this.exec = fn
  }
  function map(fn) {
    let f = this.exec
    return Task(@(errFn, okFn)
      f(errFn, @(x) okFn(fn(x)))
    )
  }
  function flatMap(fn){
    let f = this.exec
    return Task(@(errFn, okFn) f(errFn, @(x) fn(x).exec(errFn, okFn)))
  }
  function pipe(...){
    local next = this
    foreach (f in vargv){
      let fn = f
      next = this.flatMap(@(x) this.of(fn(x)))
      assert(next?.isMonad(), ASSERT_MSG)
    }
    return next
  }
  static isTask = @() true
  get = @() this.flatMap(@(x) x)
  of = @(x) Task(@(_, ok) ok(x))
}

function HttpPostTask(url, args=null) {
  return Task(function(rejectFn, resolveFn) {
    println($"http 'get' requested for '{url}'")
    let params = {
      url
      method = "POST"
      callback = tryCatch(
        function(response){
          let {status, http_code} = response
          let sttxt = statusText?[status] ?? status
          println($"http status for '{url}' = {sttxt}")
          if (status != HTTP_SUCCESS) {
            throw($"Error status: {sttxt}")
          }
          if (http_code != 200) {
            throw($"Error code: {http_code}")
          }
          resolveFn(response.body)
        },
        rejectFn
      )
    }
    params.__update(args ?? {})
    httpRequest(params)
  })
}


let mkJsonHttpReq = @(apiUrl, onError, onSuccess) function(data=null, jsondata=null) {
  let params = data != null || jsondata!=null ? {} : null
  if (data != null && params)
    params.data <- data
  if (jsondata && params)
    params.json <- jsondata
  HttpPostTask(apiUrl, params)
  .map(@(v) v.as_string())
  .map(@(v) parse_json(v))
  .exec(
    onError,
    tryCatch(onSuccess, onError)
  )
}

return {
  Task
  HttpPostTask
  mkJsonHttpReq
}
/*
  Promise
  copypasted from some JS code
*/

const P_PENDING = "pending"
const P_FULFILLED = "fulfilled"
const P_REJECTED = "rejected"

local Promise

Promise = class {
  _state = null
  resolve = null
  reject = null

  constructor(handler) {
    let state = {
      status = P_PENDING
      onFulfilledCallbacks = []
      onRejectedCallbacks = []
      value = null
    }
    this._state = state
    this.resolve = function (v) {
      if (state.status == P_PENDING) {
        state.status = P_FULFILLED
        state.value = v
        state.onFulfilledCallbacks.each(@(fn) fn())
        state.onFulfilledCallbacks.clear()
        state.onRejectedCallbacks.clear()
      }
    }

    this.reject = function (v) {
      if (state.status == P_PENDING) {
        state.status = P_REJECTED
        state.value = v
        state.onRejectedCallbacks.each(@(fn) fn())
        state.onFulfilledCallbacks.clear()
        state.onRejectedCallbacks.clear()
      }
    }

    try {
      handler(this.resolve, this.reject)
    }
    catch (err) {
      this.reject(err)
    }
  }

  function then(onFulfilled, onRejected=@(...) null) {
      let state = this._state
      return Promise(function(resolve, reject) { // -disable-warning: -ident-hides-ident
          if (state.status == P_PENDING) {
              state.onFulfilledCallbacks.append(function() {
                  try {
                      let fulfilledFromLastPromise = onFulfilled(state.value)
                      if (fulfilledFromLastPromise instanceof Promise) {
                          fulfilledFromLastPromise.then(resolve, reject)
                      } else {
                          resolve(fulfilledFromLastPromise)
                      }
                  } catch (err) {
                      reject(err)
                  }
              })
              state.onRejectedCallbacks.append(function() {
                  try {
                      let rejectedFromLastPromise = onRejected(state.value)
                      if (rejectedFromLastPromise instanceof Promise) {
                          rejectedFromLastPromise.then(resolve, reject)
                      } else {
                          reject(rejectedFromLastPromise)
                      }
                  } catch (err) {
                      reject(err)
                  }
              })
          }

          if (state.status == P_FULFILLED) {
              try {
                  let fulfilledFromLastPromise = onFulfilled(state.value)
                  if (fulfilledFromLastPromise instanceof Promise) {
                      fulfilledFromLastPromise.then(resolve, reject)
                  } else {
                      resolve(fulfilledFromLastPromise)
                  }
              } catch (err) {
                  reject(err)
              }

          }

          if (state.status == P_REJECTED) {
              try {
                  let rejectedFromLastPromise = onRejected(state.value)
                  if (rejectedFromLastPromise instanceof Promise) {
                      rejectedFromLastPromise.then(resolve, reject)
                  } else {
                      reject(rejectedFromLastPromise)
                  }
              } catch (err) {
                  reject(err)
              }
          }
      })

  }
}

return Promise
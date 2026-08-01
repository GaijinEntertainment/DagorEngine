let letMismatch = function letFunction() { //expect:w346
  return 1
}
let _letValue = letMismatch()

local localMismatch = function localFunction() { //expect:w346
  return 2
}
let _localValue = localMismatch()

let letMatch = function letMatch() {
  return 3
}
let _letMatchValue = letMatch()

local localMatch = function localMatch() {
  return 4
}
let _localMatchValue = localMatch()

let anonymous = function() {
  return 5
}
let _anonymousValue = anonymous()

let lambda = @() 6
let _lambdaValue = lambda()

let lmb = @fn() 7 //expect:w346
let _lmb = lmb;

let lmb2 = @lmb2() 8
let _lmb2 = lmb2;

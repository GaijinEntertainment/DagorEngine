//expect:w256

const C = 1

let function bar() {}
let tt = {
  "1foo" : function bar1() {}, // OK, not id
  "_foo2" : function bar2() {}, // WARN
  "_foo3" : function _foo3() {}, // OK

  foo4 = function bar5() {}, // WARN
  foo6 = function foo6() {}, // OK

  ["foo7"] = function bar7() {}, // WARN
  ["8foo"] = function bar8() {}, // OK, not id

  [C] = function bar9(_p) {}
}

tt.foo <- bar // OK
tt.qux <- function fex() {} // WARN
tt["fk"] <- function uyte() {} // WARN
tt["f:g:h"] <- function fgh() {} // OK
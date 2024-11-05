//expect:w256

const C = 1

function bar() {}
let tt = {
  "1foo" : function bar1() {}, // FP 1, not id
  "_foo2" : function bar2() {}, // EXPECTED 1
  "foo3" : function foo3() {}, // FP 2

  foo4 = function bar5() {}, // EXPECTED 2
  foo6 = function foo6() {}, // FP 3

  ["foo7"] = function bar7() {}, // EXPECTED 3
  ["8foo"] = function bar8() {}, // FP 4, not id

  [C] = function bar9(_p) {}
}

tt.foo <- bar // FP 5
tt.qux <- function fex() {} // EXPECTED 4
tt["fk"] <- function uyte() {} // EXPECTED 5
tt["f:g:h"] <- function fgh() {} // FP 6
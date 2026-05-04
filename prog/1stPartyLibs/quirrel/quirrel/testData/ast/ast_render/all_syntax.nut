from "math" import sin, cos as cosine
from "string" import *
import "datetime"
import "debug" as Debug

let { getlocals, doc } = require("debug")

{
  null;
  null;
  null;
}

if (__FILE__ && __LINE__)
  null;

println();

local tab = {}

enum Enum1 {}
enum Enum2 { AA }
enum Enum3 { AA, }
enum Enum4 { AA BB }
enum Enum5 { AA, BB }
enum Enum6 { AA, BB, }
enum Enum7 { AA = -101, BB, }
enum Enum8 { AA = 102, BB = "enum_str1", }
enum Enum9 { AA BB = "enum_str2", }
global enum Enum10 {CC = null}


#allow-auto-freeze

const cn0 = 201
const cn1 = "const_str1"
const cn2 = ["const_str2", "const_str3"]
const cn3 = {abc = "const_str4"}
global const cn4 = true

local a0;
local m0, m1;
local a1 = null
local a2 = 123_456
local a3 = 123.25
local a4 = 3.5e12
local a5 = 3.5e+12
local a6 = 3.5e-12
local a7 = -3.5e-12
local a8 = true
local a9 = false
local a10 = "\tlocal_str1\uFDF0"
local a11 = @"local_str2
local_str3"
local a12 = $"a9={a9}"
local a13 = (123_458).tofloat()
local a14 = (12.45).tostring()
local m3 = 301, m4 = 302
local a15 = Enum9.AA
local a16 = m4 = 303


let b0 = 0x401
let b1 = 0x402_FF_AA
let b2 = 'B'
let b3 = '\n'
let b4 = '\t'.tochar()


let e0 = []
let e1 = [501]
let e2 = [502,]
let e3 = [503,504]
let e4 = [505,506,]

let e5 = [507,
          508,
         ]

let e6 = [509
          510]

let e7 = [511 512]


let t0 = {}
let t1 = {x = 601}
let t2 = {"x": 602}
let t3 = {["x"] = 603}
let t4 = {[null] = 604}
let t5 = {[({y = 605})] = 606}
let t6 = {a2}
let t7 = {a3 a8}
let t8 = {x = 1}.__merge({y = 607})
let t9 = clone {z = 608}


tab.x <- "new_slot_str1"
tab["y"] <- "new_slot_str2"


local { abc } = cn3
{
  local { y, } = t8
}
{
  local { y = 609, x } = t2
  println(x)
}

{
  local { y: int = 609, x: string|int|null } = t2
  println(x)
}

local [d1, d2] = e3
local [d11: int, d12: (int | null)] = e3
local [d21, d22: (int | null)] = e3
local [d31: int, d32] = e3

function type_test1(x: int): int {
  return 1 + x
}

function type_test2(x: int, ...: float): int {
  return 1 + x
}

function type_test3(x: number|null): null|number {
  return 1.0 + (x ?? 5)
}

function type_test4(x: number|null = null): null|number {
  return 1.0 + (x ?? 6)
}

function type_test5(x: (number |
                        null)): (null
                                 | number) {
  return 1.0 + (x ?? 5)
}

function type_test6(x: (number|null) = null): (int|float) {
  return 1.0 + (x ?? 6)
}

function type_test7(x: bool|number|int|float|string|table|array|userdata|function|generator|userpointer|thread|instance|class|weakref|null, y: any): any {
  return x + y
}

let type_test8 = @(x: bool|number|int|float|string|table|array|userdata|function|generator|userpointer|thread|instance|class|weakref|null, y: any): any (
  x + y
)

let type_test9 = @type_test9(x: bool|number|int|null, y: any): any (
  x + y
)

let type_test10 = (@ [pure,] type_test10(x: bool|number|int|null, y: any): any (
  x + y
))


let x901: int = 5
local t902: table = {}
local t903: table|array = {}


class C4 {
  static x = true
  y = false
  constructor() {}
  function fn1() {}
  fn2 = function() { this.fn1() }
  fn3 = function fn3() { this.fn1() }
  fn4 = @() "class_str1"
  fn5 = @fn5() "class_str2"
  _call = function (name) {
    this[name]();
    this.constructor();
    ::tab.y <- 610
  }
}

let instance = C4()
instance.fn4()
instance?.fn4()
instance?["fn4"]()
instance.fn4?()
if (instance instanceof C4) {
  println("instance instanceof C4")
}


function gfn1() {}
local gfn2 = function() { this.gfn1() }
let gfn3 = function gfn3() { this.gfn1() }
local gfn4 = @() "lambda_str1"
let gfn5 = @gfn5() "lambda_str2"
local _call = function (name) { this[name]() }
let gfn_pure = @[pure] gfn5() "lambda_str3"
let gfn_nodiscard = @[nodiscard] gfn6() "lambda_str4"
let gfn_pure_nodiscard = @[pure, nodiscard] gfn7() "lambda_str5"

function fa1(x) { return x }
function fa2(x, y) { return x + y; }
function fa3(x, y, z) { return x + y * z; }
function fa4(x, ...) { return x + vargv.len(); }
function fa5(x = [1, 2]) { return x?[1]; }
function fa6(x, y = null) { return y; }
function fa7(x, y = {a = 4}, ) { return y; }
function fa8(x, y = {a = 5}) { return y; }
function [pure,] pure_fn(x) {return x * x}
function [nodiscard,pure,] pure_nodiscard_fn(x) {return x * x}
function coroutine_fn() {
  yield 1100;
  yield 1200;
}

println(resume coroutine_fn())


function des1({x}) { return x + 1 }
function des2([x]) { return x + 1 }
function des3([x], {y}) { return x + y }
function des4([x, z, w,], {y}) { return x + y }
function des5(x, {y = null}) { return x + (y ?? 2) }
function des6(x, {y = null, z: int|float = 1000}) { return x + (y ?? 2) + z }

function [nodiscard, pure] des7([x: function = @() @() @() null, t, r: string = "abc555", g = "abc666"], {y = null, z: int|float = 1000,}, ...) {
  let {p, i} = t
  println()
  return x + (y ?? 2) + z;
}

let i0 = 1 + 2
let i1 = 1 - 2
let i2 = 1 * 2
let i3 = 1 / 2
let i4 = 1 % 2
let i5 = 1 ^ 2
let i6 = 1 & 2
let i7 = 1 | 2
let i8 = 1 || 2
let i9 = 1 && 2
let i10 = 1 == 2
let i11 = 1 != 2
let i12 = 1 < 2
let i13 = 1 > 2
let i14 = 1 <= 2
let i15 = 1 >= 2
let i16 = 1 <=> 2
let i17 = -(123)
let i18 = ~(123)
let i19 = !(123)
let i20 = typeof 123
let i21 = const 123
let i22 = static 123
let i23 = "x" in {x = 5}
let i24 = "x" not in {x = 6}
let i25 = 1 >> 2
let i26 = 1 << 2
let i27 = 1 >>> 2
let i28 = 1 ?? 2


local iv = 123
iv++
iv--
--iv
++iv
iv += 123
iv -= 123
iv *= 123
iv /= 123
iv %= 123
//iv |= 123
//iv ^= 123
//iv &= 123
//iv ||= 123
//iv ^^= 123
//iv &&= 123

/* comment */

if (base) {
  println("base != null")
}

{
  ;;;;;;
}


if (701 == 701)
  fa1(701)


if (702 == 703)
  fa1(702)
else
  fa2(703, 704)


if (705 == 706)
  fa1(707)
else if (708 == 709)
  fa2(710, 711)
else
  fa2(712, 713)


if (local cv = iv)
  println(cv)


if (local cv: int|null = iv)
  println(cv)
else
  println(cv - 1)


if (let cv: int = iv; cv > -1000000)
  println(cv)


if (local cv = iv)
  println(cv)
else if (local cv2 = iv)
  println("fail")


while (iv--)
  fa1(714)


do
  fa1(715)
while (iv++ < 3)


foreach (i in e1)
  fa1(i)


foreach (k, v in t7)
  fa2(k, v)


for (;;)
  break


for (local iter = 0; iter < 4; ++iter)
  continue;

for (local iter = 0; iter < 4; iter += 2, iter--)
  continue;


try
  fa1(716)
catch (e)
  fa1(717)


try
  throw null
catch (e)
  fa1(718)


#forbid-auto-freeze

local w = getlocals()
local res = []
foreach (k, v in w)
  res.append($"{k} = {["integer", "bool", "string", "float"].indexof(type(v)) >= 0 ? v : type(v)}")

res.sort()
foreach (_, v in res)
  println(v)


class A {
  @@"Class docstring"
  function x() { return 0; }
}

let instance_a = A()

function test() {
  return {
    @@"Table docstring"
    x = 4
    fn = function() {
      @@"Function docstring"
      return 1234
    }
  }
}

println(doc(A))
println(doc(instance_a))
println(doc(test()))
println(doc(test().fn))


println("done")

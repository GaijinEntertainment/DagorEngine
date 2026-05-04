from "types" import *

function format_array(arr) {
  let s = ", ".join(arr)
  return $"[{s}]"
}

function format_table(tbl) {
  let s = ", ".join(tbl.topairs().map(@(e) $"[{e[0]}]={e[1]}"))
  return $"\{{s}\}"
}

println("Integer():", Integer())
println("Integer(42):", Integer(42))
println("Integer(3.14):", Integer(3.14))
println("Integer(\"100\"):", Integer("100"))
println("Integer(true):", Integer(true))

println()

println("Float():", Float())
println("Float(42):", Float(42))
println("Float(3.14):", Float(3.14))
println("Float(\"3.14\"):", Float("3.14"))
println("Float(true):", Float(true))

println()

println("Bool():", Bool())
println("Bool(0):", Bool(0))
println("Bool(1):", Bool(1))
println("Bool(3.14):", Bool(3.14))
println("Bool(null):", Bool(null))
println("Bool(\"hello\"):", Bool("hello"))
println("Bool(\"\"):", Bool(""))

println()

println("String(42):", String(42))
println("String(3.14):", String(3.14))
println("String(true):", String(true))
println("String(null):", String(null))

println()

let arr1 = Array(0)
println("Array(0):", format_array(arr1))
let arr2 = Array(5)
println("Array(5):", format_array(arr2))
let arr3 = Array(3, "hello")
println("Array(3, \"hello\"):", format_array(arr3))

println()
let tbl = Table()
println("Table():", format_table(tbl))
tbl.x <- 10
tbl.y <- 20
println("After adding keys:", format_table(tbl))

println()
println("Null():", Null())

println()
let obj = {foo = "bar"}
let wr = WeakRef(obj)
println("type(WeakRef(obj)):", type(wr))
println("wr.ref():", format_table(wr.ref()))

println()
let x = Integer("42")
println("x = Integer(\"42\"):", x, "type:", typeof(x))
println("x instanceof Integer:", x instanceof Integer)

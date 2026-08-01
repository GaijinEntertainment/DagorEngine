let isEven: function
let value: int
let unset
let first, second

let callIsEven = @() isEven(42)
let readValue = @() value
let readUnset = @() unset

println(readValue() == null ? "null" : "value")
println(readUnset() == null ? "unset" : "value")

isEven = function (value: int): bool {
  return value % 2 == 0
}
value = 7
unset = "late"
first = 1
second = 2

println(callIsEven() ? "even" : "odd")
println(readValue())
println(readUnset())
println(first + second)

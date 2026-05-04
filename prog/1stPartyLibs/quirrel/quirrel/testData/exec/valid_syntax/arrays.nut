// Valid syntax: Arrays

// Empty array
let empty = []
assert(empty.len() == 0)

// Array with values
let arr = [1, 2, 3, 4, 5]
assert(arr.len() == 5)
assert(arr[0] == 1)
assert(arr[4] == 5)

// Mixed types
let mixed = [1, "hello", 3.14, null, true, [1, 2], {a = 1}]
assert(mixed.len() == 7)
assert(typeof mixed[0] == "integer")
assert(typeof mixed[1] == "string")
assert(typeof mixed[5] == "array")
assert(typeof mixed[6] == "table")

// Nested arrays
let nested = [[1, 2], [3, 4], [5, 6]]
assert(nested[0][0] == 1)
assert(nested[2][1] == 6)

// Array methods - append
local a = [1, 2, 3]
a.append(4)
assert(a.len() == 4)
assert(a[3] == 4)

// Multiple append
a.append(5, 6)
assert(a.len() == 6)

// extend
local b = [1, 2]
b.extend([3, 4])
assert(b.len() == 4)

// pop
let popped = b.pop()
assert(popped == 4)
assert(b.len() == 3)

// top
assert(b.top() == 3)

// insert
b.insert(0, 99)
assert(b[0] == 99)

// remove
let removed = b.remove(0)
assert(removed == 99)

// resize
local resizable = [1, 2, 3]
resizable.resize(5, 0)
assert(resizable.len() == 5)
assert(resizable[3] == 0)

// sort
local sortable = [3, 1, 4, 1, 5, 9, 2, 6]
sortable.sort()
assert(sortable[0] == 1)
assert(sortable[sortable.len() - 1] == 9)

// sort with custom comparator
local desc = [3, 1, 4, 1, 5]
desc.sort(@(a, b) b <=> a)
assert(desc[0] == 5)
assert(desc[desc.len() - 1] == 1)

// reverse
local rev = [1, 2, 3]
rev.reverse()
assert(rev[0] == 3)
assert(rev[2] == 1)

// slice
let sliced = [1, 2, 3, 4, 5].slice(1, 4)
assert(sliced.len() == 3)
assert(sliced[0] == 2)
assert(sliced[2] == 4)

// slice with default end
let sliced2 = [1, 2, 3, 4, 5].slice(2)
assert(sliced2.len() == 3)
assert(sliced2[0] == 3)

// filter
let evens = [1, 2, 3, 4, 5, 6].filter(@(v) v % 2 == 0)
assert(evens.len() == 3)
assert(evens[0] == 2)

// map
let doubled = [1, 2, 3].map(@(v) v * 2)
assert(doubled[0] == 2)
assert(doubled[1] == 4)
assert(doubled[2] == 6)

// reduce
let sum = [1, 2, 3, 4, 5].reduce(@(acc, v) acc + v, 0)
assert(sum == 15)

// each
local each_sum = 0
let _ = [1, 2, 3].each(@(v) each_sum += v)
assert(each_sum == 6)

// findindex
let found = [10, 20, 30, 40].findindex(@(v) v > 25)
assert(found == 2)

// findvalue
let fval = [10, 20, 30, 40].findvalue(@(v) v > 25)
assert(fval == 30)

// indexof
let idx = [10, 20, 30].indexof(20)
assert(idx == 1)

// contains
assert([1, 2, 3].contains(2))
assert(![1, 2, 3].contains(4))

// clone
let orig = [1, 2, 3]
let copy = clone orig
copy[0] = 99
assert(orig[0] == 1)

// clear
local clearable = [1, 2, 3]
clearable.clear()
assert(clearable.len() == 0)

// swap
local swappable = [1, 2, 3]
swappable.swap(0, 2)
assert(swappable[0] == 3)
assert(swappable[2] == 1)

// hasindex
assert([1, 2, 3].hasindex(0))
assert([1, 2, 3].hasindex(2))
assert(![1, 2, 3].hasindex(3))

// apply (modify in-place)
local applyable = [1, 2, 3]
applyable.apply(@(v) v * 10)
assert(applyable[0] == 10)
assert(applyable[1] == 20)

// Iteration
local iter_sum = 0
foreach (v in [10, 20, 30]) {
    iter_sum += v
}
assert(iter_sum == 60)

// Iteration with index
local indices = []
foreach (i, v in ["a", "b", "c"]) {
    indices.append(i)
}
assert(indices[0] == 0)
assert(indices[2] == 2)

print("arrays: OK\n")

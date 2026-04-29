// Valid syntax: Control flow statements

// if/else
local result = null
if (true)
    result = "yes"
assert(result == "yes")

if (false)
    result = "no"
else
    result = "else"
assert(result == "else")

// if/else if/else chain
local val = 2
if (val == 1)
    result = "one"
else if (val == 2)
    result = "two"
else if (val == 3)
    result = "three"
else
    result = "other"
assert(result == "two")

// if with block
if (true) {
    result = "block"
}
assert(result == "block")

// if with local declaration in condition
if (local cond_val = 42) {
    assert(cond_val == 42)
}

// while loop
local count = 0
while (count < 5) {
    count++
}
assert(count == 5)

// do/while loop
local dw_count = 0
do {
    dw_count++
} while (dw_count < 3)
assert(dw_count == 3)

// do/while executes at least once
local dw_once = 0
do {
    dw_once++
} while (false)
assert(dw_once == 1)

// for loop
local sum = 0
for (local i = 0; i < 5; i++) {
    sum += i
}
assert(sum == 10)

// for loop with empty parts
local inf_count = 0
for (;;) {
    inf_count++
    if (inf_count >= 3) break
}
assert(inf_count == 3)

// foreach with array
let arr = [10, 20, 30]
local arr_sum = 0
foreach (v in arr) {
    arr_sum += v
}
assert(arr_sum == 60)

// foreach with index and value
local idx_sum = 0
foreach (i, v in arr) {
    idx_sum += i
}
assert(idx_sum == 3) // 0+1+2

// foreach with table
let tbl = {a = 1, b = 2, c = 3}
local tbl_sum = 0
foreach (k, v in tbl) {
    tbl_sum += v
}
assert(tbl_sum == 6)

// foreach with string
local char_count = 0
foreach (ch in "hello") {
    char_count++
}
assert(char_count == 5)

// break in loop
local break_val = 0
for (local i = 0; i < 100; i++) {
    if (i == 5) break
    break_val = i
}
assert(break_val == 4)

// continue in loop
local cont_sum = 0
for (local i = 0; i < 10; i++) {
    if (i % 2 == 0) continue
    cont_sum += i
}
assert(cont_sum == 25) // 1+3+5+7+9

// break in while
local bw = 0
while (true) {
    bw++
    if (bw >= 3) break
}
assert(bw == 3)

// continue in while
local cw = 0
local cw_sum = 0
while (cw < 10) {
    cw++
    if (cw % 2 == 0) continue
    cw_sum += cw
}
assert(cw_sum == 25)

// break in foreach
local bf_last = 0
foreach (i, v in [10, 20, 30, 40, 50]) {
    if (v == 30) break
    bf_last = v
}
assert(bf_last == 20)

// Nested loops with break
local nested_count = 0
for (local i = 0; i < 5; i++) {
    for (local j = 0; j < 5; j++) {
        if (j == 2) break
        nested_count++
    }
}
assert(nested_count == 10)

// switch statement
#allow-switch-statement
local sw_val = 2
local sw_result = null
switch (sw_val) {
    case 1:
        sw_result = "one"
        break
    case 2:
        sw_result = "two"
        break
    case 3:
        sw_result = "three"
        break
    default:
        sw_result = "default"
        break
}
assert(sw_result == "two")

// switch with default
sw_val = 99
switch (sw_val) {
    case 1:
        sw_result = "one"
        break
    default:
        sw_result = "default"
        break
}
assert(sw_result == "default")

print("control_flow: OK\n")

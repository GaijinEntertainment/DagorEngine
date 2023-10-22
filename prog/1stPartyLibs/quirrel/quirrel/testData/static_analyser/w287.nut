//-file:undefined-global
//-file:declared-never-used

local curVal = ::a
local x = ::g_chat.rooms.len()

let e1 = (curVal < 0 || curVal > x) // EXPECTED 1
let c1 = (curVal < 0 || curVal >= x) // FP 1

local value = ::a
local cnt = ::listObj.childrenCount()

let e2 = (value >= 0) && (value <= cnt) // EXPECTED 2
let c2 = (value >= 0) && (value < cnt) // FP 2

let e3 = (::idx < 0 || ::idx > ::tblObj.childrenCount()) // EXPECTED 3
let c3 = (::idx < 0 || ::idx >= ::tblObj.childrenCount()) // FP 3

let e4 = (0 <= value && value <= ::obj.childrenCount()) // EXPECTED 4
let c4 = (0 <= value && value < ::obj.childrenCount()) // FP 4
//-file:declared-never-used

let t = {a = 1, b = 2}

let a = t.__merge({}) //warning:merge-empty-table

let b = t.__merge({x = 1}) // ok, non-empty table

let c = clone t // ok, already using clone

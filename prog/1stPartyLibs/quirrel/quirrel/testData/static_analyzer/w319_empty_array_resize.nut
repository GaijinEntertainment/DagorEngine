//-file:declared-never-used

let a = [].resize(10, 0) //warning:empty-array-resize

let b = array(10, 0) // ok, already using array()

let c = [1, 2, 3].resize(10, 0) // ok, non-empty array

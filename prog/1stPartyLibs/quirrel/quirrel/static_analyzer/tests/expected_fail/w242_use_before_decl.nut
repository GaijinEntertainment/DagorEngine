//expect:w242

local function x() { //-declared-never-used
  y()
}

local function y() {
}

return [x, y];
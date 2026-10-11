//-file:declared-never-used
from "math" import sin

local function id(a) { return a }

// the statement after an import statement is the indentation baseline of the lines below it,
// as after any other statement
  local first = id(sin(0))
local second = id(2)

// the same for a try statement
local function afterTry() {
  try {
    id(1)
  } catch (e) {
    id(2)
  }
    id(3)
  id(4)
}

local function afterLocal() {
  local a = 1
    id(a)
  id(2)
}

local function tryOnOneLine() {
  try { id(1) } catch (e) { id(2) }
  id(3)
}

// an unbraced try statement as the body of an if: its end is the line of its own last token, so
// the formatting check of the if statement reads it
local function tryAsIfBody(c) {
  if (c)
    try {
      id(1)
    } catch (e) {
      id(2)
    }
      id(3)
  return c
}

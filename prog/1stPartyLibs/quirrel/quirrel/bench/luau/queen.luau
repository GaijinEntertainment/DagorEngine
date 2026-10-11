--!strict
--[[
MIT License

Copyright (c) 2017 Gabriel de Quadros Ligneul

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]
local profile = require("./profile")

local N = 8
local BOARDS = 50

local function isPlaceOk(board: { number }, n: number, c: number): boolean
  for i = 1, n - 1 do
    local qi = board[i]
    if qi == c or qi - i == c - n or qi + i == c + n then
      return false
    end
  end
  return true
end

local function addQueen(board: { number }, n: number): number
  if n > N then
    return 1
  end
  local solutions = 0
  for c = 1, N do
    if isPlaceOk(board, n, c) then
      board[n] = c
      solutions += addQueen(board, n + 1)
    end
  end
  return solutions
end

local function boards(): number
  local solutions = 0
  for _ = 1, BOARDS do
    solutions = addQueen(table.create(N, 0), 1)
  end
  return solutions
end

profile.benchmark("queen", 20, boards, BOARDS)

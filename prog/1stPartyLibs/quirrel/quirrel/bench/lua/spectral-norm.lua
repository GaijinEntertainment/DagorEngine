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

local sqrt <const> = math.sqrt
local create <const> = table.create
local format <const> = string.format
local benchmark <const> = require("profile").benchmark

local N <const> = 500

local function filled(value)
  local arr = create(N)
  for i = 1, N do
    arr[i] = value
  end
  return arr
end

local function A(i, j)
  local ij = i + j - 1
  return 1.0 / (ij * (ij - 1) // 2 + i)
end

local function Av(x, y)
  for i = 1, N do
    local a = 0.0
    for j = 1, N do
      a = a + A(i, j) * x[j]
    end
    y[i] = a
  end
end

local function Atv(x, y)
  for i = 1, N do
    local a = 0.0
    for j = 1, N do
      a = a + A(j, i) * x[j]
    end
    y[i] = a
  end
end

local function AtAv(x, y, t)
  Av(x, t)
  Atv(t, y)
end

local function spectralNorm()
  local u = filled(1.0)
  local v = filled(0.0)
  local t = filled(0.0)
  for _ = 1, 10 do
    AtAv(u, v, t)
    AtAv(v, u, t)
  end
  local vBv = 0.0
  local vv = 0.0
  for i = 1, N do
    vBv = vBv + u[i] * v[i]
    vv = vv + v[i] * v[i]
  end
  return format("%.9f", sqrt(vBv / vv))
end

benchmark("spectral norm", 10, spectralNorm)

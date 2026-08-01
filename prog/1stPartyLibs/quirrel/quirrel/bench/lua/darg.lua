local CompsToUpdateEachFrame = 30
local ROBJ_TEXT = 0
local ROBJ_FRAME= 1
local ROBJ_SOLID= 2
local ALIGN_CENTER = 0
local FLOW_VERTICAL = 0
local screenWidth = 1920.0
local screenHeight= 1080.0

Behaviors = {
    Button = 0
}

Watched = {}
Watched.__index = Watched
local random = math.random

function Watched:new(value)
    local instance = setmetatable({}, self)
    instance.value = value
    return instance
end

function Watched:get()
    return self.value
end

-- Generate a random integer up to `max`
function rint(max)
    return math.floor(math.random() * max)
end

-- Convert a value to a percentage of screen width
function sw(val)
    return (val / 100) * screenWidth
end

-- Convert a value to a percentage of screen height
function sh(val)
    return (val / 100) * screenHeight
end

-- Create a color from RGBA values
function Color(r, g, b, a)
    a = a or 255  -- Default alpha to 255 if not provided
    return (math.floor(a) * 16777216) + (math.floor(r) * 65536) + (math.floor(g) * 256) + math.floor(b)
end

-- Convert a value to a high-density pixel value
function hdpx(val)
    return val / sh(100) * screenHeight
end

-- Default flex configuration
local defflex = {}

-- Flex function to return default or custom flex
function flex(val)
    if val == nil then
        return defflex
    else
        return {val = val}
    end
end



local componentsNum = 2000
local borders = true


function simpleComponent(i, watch)
    local pos = {sw(random() * 80), sh(random() * 80)}
    local size = {sh(random() * 15 + 2), sh(random() * 15 + 2)}
    local color = Color(random() * 255, random() * 255, random() * 255)
    local textCanBePlaced = size[1] > hdpx(150)
    local addText = rint(5) == 1
    local text = nil

    if textCanBePlaced then
        text = function()
            return {
                watch = watch,
                rendObj = ROBJ_TEXT,
                text = (addText or not watch:get()) and ("" .. i .. tostring(watch:get())) or nil
            }
        end
    end

    local children = {}

    if borders then
        table.insert(children, function()
            return {
                borderWidth = {1, 1, 1, 1},
                rendObj = ROBJ_FRAME,
                size = flex()
            }
        end)
        table.insert(children, text)
    end

    return function()
        return {
            rendObj = ROBJ_SOLID,
            size = size,
            key = {},
            pos = pos,
            valign = ALIGN_CENTER,
            halign = ALIGN_CENTER,
            padding = {hdpx(4), hdpx(5)},
            margin = {hdpx(5), hdpx(3)},
            color = watch:get() and color or Color(0, 0, 0),
            behavior = Behaviors.Button,
            onClick = function() watch:modify(function(v) return not v end) end,
            watch = watch,
            children = children
        }
    end
end

local num = 0
local children = {}
for i = 0, componentsNum - 1 do
    table.insert(children, simpleComponent(i, Watched:new(i % 3 == 0)))
end

function benchmark()
    return {
        size = flex(),
        flow = FLOW_VERTICAL,
        children = children
    }
end

function testUi(entry)
    if entry == nil then
        return true
    end
    if type(entry) == "function" then
        entry = entry()
    end
    local t = type(entry)
    if t == "table" then
        if entry.children == nil then
            return true
        end
        if type(entry.children) == "table" then
            for _, child in ipairs(entry.children) do
                if not testUi(child) then
                    return false
                end
            end
            return true
        end
        return testUi(entry.children)
    end
    return false
end

local Frames = 100
local testsNum = 20

---
loadfile("profile.lua")()
local testf = function()
    local frames = Frames
    while frames > 0 do
        frames = frames - 1
        testUi(benchmark)
    end
end
io.write(string.format("\"profile darg ui\", %.8f, %s\n", profile_it(testsNum, testf), testsNum))



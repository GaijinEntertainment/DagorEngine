let a = [1, 2, 3, 4, 5]
local m = []

foreach (_, x in a) {
    m.append(@() print(x))
    m.each(@(_p) print(x))
}
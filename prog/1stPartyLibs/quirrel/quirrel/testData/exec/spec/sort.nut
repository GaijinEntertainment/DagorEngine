let a = [4, 2, 5]
a.sort(@(a, b) a<=>b)
a.each(@(v) print(v))
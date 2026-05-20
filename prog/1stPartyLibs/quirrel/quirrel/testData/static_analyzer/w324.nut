//-file:declared-never-used

let arr = [1, 2, 3]

local guard = 0
//expect:w324
for (local i = 0; i < 10; i--) {
  if (guard++ > 1)
    break
}

guard = 0
//expect:w324
for (local i = arr.len() - 1; i >= 0; i++) {
  if (guard++ > 1)
    break
}

guard = 0
//expect:w324
for (local i = 0; i > 10; i++) {
  if (guard++ > 1)
    break
}

guard = 0
//expect:w324
for (local i = 0; i > 0; i++) {
  if (guard++ > 1)
    break
}

guard = 0
//expect:w324
for (local i = 0; 0 < i; i++) {
  if (guard++ > 1)
    break
}

guard = 0
//expect:w324
for (local i = 0; 10 > i; i--) {
  if (guard++ > 1)
    break
}

const n = -2
guard = 0
//expect:w324
for (local i = 0; i < 10; i += n) {
  if (guard++ > 1)
    break
}

for (local i = 0; i < 10; i++) {
}

for (local i = arr.len() - 1; i >= 0; i--) {
}

const p = 2
for (local i = 0; i < 10; i += p) {
}

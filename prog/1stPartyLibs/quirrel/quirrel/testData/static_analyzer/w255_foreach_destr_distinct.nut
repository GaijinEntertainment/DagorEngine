// Two functions whose bodies differ only in the foreach destructuring pattern
// must NOT be flagged as duplicate (w255) or similar (w258). Covers
// diffForeach / cmpForeach including valDestructuring() in the comparison.
function sum_xy(rows) {
  local total = 0
  foreach ({x, y = 0} in rows) {
    total += x + y
  }
  return total
}

function sum_a_only(rows) {
  local total = 0
  foreach ({a, b = 0} in rows) {
    total += a + b
  }
  return total
}

println(sum_xy([{x = 1, y = 2}]))
println(sum_a_only([{a = 1, b = 2}]))

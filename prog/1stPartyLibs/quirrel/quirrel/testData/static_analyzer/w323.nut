//expect:w323
//-file:expr-cannot-be-null

let A = 5
let w = 7
let x = 11
let y = 13

let falseBranch = A == 5 ? x : A == 5 ? w : y
//expect:w323
let trueBranch = A == 5 ? (A == 5 ? w : y) : x
//expect:w323
let deeperBranch = ((A == 5)) ? x : (A == null ? w : (A == 5 ? 1 : 2))
//expect:w323
let chainOuter = x == 1 ? "a" : x == 2 ? "b" : x == 1 ? "c" : "d"
//expect:w323
let chainInner = x == 1 ? "a" : x == 2 ? "b" : x == 2 ? "c" : "d"

if (A == 5) {
  //expect:w323
  println(A == 5 ? 1 : 2)
}

if (A == 5) {
  let inFunction = function() {
    return A == 5 ? 1 : 2
  }
  println(inFunction())
}

let _ = $"{falseBranch + trueBranch + deeperBranch}{chainOuter}{chainInner}"

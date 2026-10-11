from "profile.nut" import benchmark

const N = 8
const BOARDS = 50

const function isPlaceOk(board, n, c) { //-ident-hides-ident
  for (local i = 0; i < n; i++) {
    let qi = board[i]
    if (qi == c || qi - i == c - n || qi + i == c + n)
      return false
  }
  return true
}

function addQueen(board, n) {
  if (n == N)
    return 1
  local solutions = 0
  for (local c = 0; c < N; c++) {
    if (isPlaceOk(board, n, c)) {
      board[n] = c
      solutions += addQueen(board, n + 1)
    }
  }
  return solutions
}

function boards() {
  local solutions = 0
  for (local i = 0; i < BOARDS; i++)
    solutions = addQueen(array(N, 0), 0)
  return solutions
}

benchmark("queen", 20, boards, BOARDS)

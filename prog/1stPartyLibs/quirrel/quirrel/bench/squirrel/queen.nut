dofile("profile.nut")

const N = 8
const BOARDS = 50

local function isPlaceOk(board, n, c) {
  for (local i = 0; i < n; i++) {
    local qi = board[i]
    if (qi == c || qi - i == c - n || qi + i == c + n)
      return false
  }
  return true
}

// a local function cannot see itself
local addQueen
addQueen = function(board, n) {
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

local function boards() {
  local solutions = 0
  for (local i = 0; i < BOARDS; i++)
    solutions = addQueen(array(N, 0), 0)
  return solutions
}

benchmark("queen", 20, boards, BOARDS)

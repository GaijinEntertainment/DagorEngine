import { benchmark } from "./profile.js";

const N = 8;
const BOARDS = 50;

function isPlaceOk(board, n, c) {
  for (let i = 0; i < n; i++) {
    const qi = board[i];
    if (qi === c || qi - i === c - n || qi + i === c + n)
      return false;
  }
  return true;
}

function addQueen(board, n) {
  if (n === N)
    return 1;
  let solutions = 0;
  for (let c = 0; c < N; c++) {
    if (isPlaceOk(board, n, c)) {
      board[n] = c;
      solutions += addQueen(board, n + 1);
    }
  }
  return solutions;
}

function boards() {
  let solutions = 0;
  for (let i = 0; i < BOARDS; i++)
    solutions = addQueen(new Array(N).fill(0), 0);
  return solutions;
}

benchmark("queen", 20, boards, BOARDS);

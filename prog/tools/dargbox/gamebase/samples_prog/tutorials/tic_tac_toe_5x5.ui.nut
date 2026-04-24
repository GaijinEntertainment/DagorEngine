from "%darg/ui_imports.nut" import *

//---------------------------------------------------------------------
// Palette: Colors for the tic-tac-toe game
//---------------------------------------------------------------------
let Palette = {
  appBg              = Color(24,26,34)
  boardBg            = Color(40,44,52)
  cellBg             = Color(52,58,68)
  cellBgHover        = Color(62,68,78)
  cellBgActive       = Color(72,78,88)
  cellBorder         = Color(80,86,96)
  cellBorderHover    = Color(120,126,136)
  playerX            = Color(220,100,100)
  playerO            = Color(100,150,220)
  textPrimary        = Color(240,245,255)
  textSecondary      = Color(180,185,195)
  btnBg              = Color(60,68,92)
  btnBgHover         = Color(80,88,112)
  btnBorder          = Color(100,108,132)
  winnerBg           = Color(60,120,60)
}

//---------------------------------------------------------------------
// Game state
//---------------------------------------------------------------------
let EMPTY = 0
let PLAYER_X = 1
let PLAYER_O = 2

let board = Watched(array(25, EMPTY))  // 5x5 = 25 cells
let currentPlayer = Watched(PLAYER_X)
let gameStatus = Watched("playing")    // "playing", "x_wins", "o_wins", "draw"
let winner = Watched(null)             // winning line coordinates

//---------------------------------------------------------------------
// Game logic functions
//---------------------------------------------------------------------
function resetGame() {
  board.set(array(25, EMPTY))
  currentPlayer.set(PLAYER_X)
  gameStatus.set("playing")
  winner.set(null)
}

function getCell(row, col) {
  return board.get()[row * 5 + col]
}

function setCell(row, col, player) {
  let newBoard = clone board.get()
  newBoard[row * 5 + col] = player
  board.set(newBoard)
}

function checkWin() {
  let b = board.get()

  // Check rows
  for (local row = 0; row < 5; row++) {
    for (local startCol = 0; startCol <= 1; startCol++) {
      let player = b[row * 5 + startCol]
      if (player != EMPTY) {
        local count = 1
        for (local col = startCol + 1; col < 5; col++) {
          if (b[row * 5 + col] == player) {
            count++
          } else {
            break
          }
        }
        if (count >= 4) {
          return { player, type = "row", row, startCol, endCol = startCol + count - 1 }
        }
      }
    }
  }

  // Check columns
  for (local col = 0; col < 5; col++) {
    for (local startRow = 0; startRow <= 1; startRow++) {
      let player = b[startRow * 5 + col]
      if (player != EMPTY) {
        local count = 1
        for (local row = startRow + 1; row < 5; row++) {
          if (b[row * 5 + col] == player) {
            count++
          } else {
            break
          }
        }
        if (count >= 4) {
          return { player, type = "col", col, startRow, endRow = startRow + count - 1 }
        }
      }
    }
  }

  // Check diagonals (top-left to bottom-right)
  for (local startRow = 0; startRow <= 1; startRow++) {
    for (local startCol = 0; startCol <= 1; startCol++) {
      let player = b[startRow * 5 + startCol]
      if (player != EMPTY) {
        local count = 1
        local row = startRow + 1
        local col = startCol + 1
        while (row < 5 && col < 5) {
          if (b[row * 5 + col] == player) {
            count++
            row++
            col++
          } else {
            break
          }
        }
        if (count >= 4) {
          return { player, type = "diag1", startRow, startCol, count }
        }
      }
    }
  }

  // Check diagonals (top-right to bottom-left)
  for (local startRow = 0; startRow <= 1; startRow++) {
    for (local startCol = 3; startCol < 5; startCol++) {
      let player = b[startRow * 5 + startCol]
      if (player != EMPTY) {
        local count = 1
        local row = startRow + 1
        local col = startCol - 1
        while (row < 5 && col >= 0) {
          if (b[row * 5 + col] == player) {
            count++
            row++
            col--
          } else {
            break
          }
        }
        if (count >= 4) {
          return { player, type = "diag2", startRow, startCol, count }
        }
      }
    }
  }

  return null
}

function checkDraw() {
  let b = board.get()
  foreach (cell in b) {
    if (cell == EMPTY) {
      return false
    }
  }
  return true
}

function makeMove(row, col) {
  if (gameStatus.get() != "playing" || getCell(row, col) != EMPTY) {
    return
  }

  let player = currentPlayer.get()
  setCell(row, col, player)

  let winResult = checkWin()
  if (winResult != null) {
    winner.set(winResult)
    gameStatus.set(winResult.player == PLAYER_X ? "x_wins" : "o_wins")
  } else if (checkDraw()) {
    gameStatus.set("draw")
  } else {
    currentPlayer.set(player == PLAYER_X ? PLAYER_O : PLAYER_X)
  }
}

//---------------------------------------------------------------------
// Helper function to check if a cell is part of winning line
//---------------------------------------------------------------------
function isWinningCell(row, col) {
  let w = winner.get()
  if (w == null) return false

  if (w.type == "row") {
    return w.row == row && col >= w.startCol && col <= w.endCol
  } else if (w.type == "col") {
    return w.col == col && row >= w.startRow && row <= w.endRow
  } else if (w.type == "diag1") {
    let dr = row - w.startRow
    let dc = col - w.startCol
    return dr == dc && dr >= 0 && dr < w.count
  } else if (w.type == "diag2") {
    let dr = row - w.startRow
    let dc = w.startCol - col
    return dr == dc && dr >= 0 && dr < w.count
  }

  return false
}

//---------------------------------------------------------------------
// UI Components
//---------------------------------------------------------------------
function Cell(row, col) {
  return watchElemState(function(sf) {
    let cellValue = getCell(row, col)
    let isWinner = isWinningCell(row, col)

    return {
      size = const [sh(6), sh(6)]
      behavior = Behaviors.Button
      rendObj = ROBJ_BOX
      fillColor = isWinner ? Palette.winnerBg
                : (sf & S_ACTIVE) ? Palette.cellBgActive
                : (sf & S_HOVER) ? Palette.cellBgHover
                : Palette.cellBg
      borderWidth = 2
      borderColor = (sf & S_HOVER) ? Palette.cellBorderHover : Palette.cellBorder
      onClick = function() { makeMove(row, col) }
      watch = [board, winner]

      children = cellValue != EMPTY ? {
        rendObj = ROBJ_TEXT
        text = cellValue == PLAYER_X ? "×" : "○"
        color = cellValue == PLAYER_X ? Palette.playerX : Palette.playerO
        fontSize = sh(4)
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
      } : null
    }
  })
}

function GameBoard() {
  let rows = []
  for (local row = 0; row < 5; row++) {
    let cells = []
    for (local col = 0; col < 5; col++) {
      cells.append(Cell(row, col))
    }
    rows.append({
      flow = FLOW_HORIZONTAL
      gap = sh(0.3)
      children = cells
    })
  }

  return {
    rendObj = ROBJ_BOX
    fillColor = Palette.boardBg
    padding = sh(1)
    gap = sh(0.3)
    flow = FLOW_VERTICAL
    children = rows
  }
}

let StatusPanel = @() {
  watch = [gameStatus, currentPlayer]
  rendObj = ROBJ_TEXT
  color = Palette.textPrimary
  fontSize = sh(2)
  text = gameStatus.get() == "playing" ?
         $"Current player: {currentPlayer.get() == PLAYER_X ? "×" : "○"}" :
         gameStatus.get() == "x_wins" ? "× wins!" :
         gameStatus.get() == "o_wins" ? "○ wins!" :
         "It's a draw!"
}

let ResetButton = watchElemState(@(sf) {
  behavior = Behaviors.Button
  rendObj = ROBJ_BOX
  fillColor = (sf & S_HOVER) ? Palette.btnBgHover : Palette.btnBg
  borderWidth = 1
  borderColor = Palette.btnBorder
  padding = const [sh(0.8), sh(1.5)]
  onClick = resetGame

  children = {
    rendObj = ROBJ_TEXT
    color = Palette.textPrimary
    text = "New Game"
    fontSize = sh(1.5)
  }
})

//---------------------------------------------------------------------
// Main application
//---------------------------------------------------------------------
function TicTacToe5x5() {
  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = Palette.appBg
    flow = FLOW_VERTICAL
    gap = sh(2)
    padding = sh(3)
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER

    children = [
      {
        rendObj = ROBJ_TEXT
        text = "5×5 Tic-Tac-Toe"
        color = Palette.textPrimary
        fontSize = sh(3)
      }
      {
        rendObj = ROBJ_TEXT
        text = "Get 4 in a row to win!"
        color = Palette.textSecondary
        fontSize = sh(1.2)
      }
      StatusPanel
      GameBoard
      ResetButton
    ]
  }
}

return TicTacToe5x5
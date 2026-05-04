options gen2
options persistent_heap

module bot_greedy public

require bot_utils public
require gameplay
require math

// Greedy bot: scores each legal move by configurable weights and picks the best.
// See bot_utils.das for weight definitions and scoring functions.
// Ties broken by reservoir sampling (seed-based random).
// No lookahead — purely greedy, one move at a time.

// Main entry point — with explicit weights
def bot_greedy_move(game : GameState; player_idx : int; seed : int; w : GreedyWeights) : Move {
    var moves <- legal_moves(game, player_idx)
    let n = length(moves)
    if (n == 0) {
        return Move()
    }
    var best_score = -1.0e10
    var best_idx = 0
    var num_tied = 0
    var rng = seed
    for (i in range(n)) {
        let m = moves[i]
        let card = get_hand_card(game, player_idx, m.card_idx)
        var score = 0.0
        if (is_black_jack(card)) {
            score = score_removal(game, m.row, m.col, player_idx, w)
        } else {
            score = score_placement(game, m.row, m.col, player_idx, w)
        }
        if (score > best_score) {
            best_score = score
            best_idx = i
            num_tied = 1
        } elif (score == best_score) {
            num_tied++
            rng = rng * 1103515245 + 12345
            if (abs(rng) % num_tied == 0) {
                best_idx = i
            }
        }
    }
    let result = moves[best_idx]
    delete moves
    return result
}

// Convenience overload with default weights
def bot_greedy_move(game : GameState; player_idx : int; seed : int) : Move {
    return bot_greedy_move(game, player_idx, seed, GREEDY_DEFAULT)
}

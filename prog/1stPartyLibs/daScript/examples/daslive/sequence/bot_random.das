options gen2
options persistent_heap

module bot_random public

require gameplay
require math

// Random bot: picks a uniformly random legal move.

def bot_random_move(game : GameState; player_idx : int; seed : int) : Move {
    var moves <- legal_moves(game, player_idx)
    let n = length(moves)
    if (n == 0) {
        return Move()
    }
    // Simple LCG random from seed
    var rng = seed * 1103515245 + 12345
    let idx = abs(rng) % n
    let m = moves[idx]
    delete moves
    return m
}

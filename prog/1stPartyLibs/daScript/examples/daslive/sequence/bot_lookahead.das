options gen2
options persistent_heap

module bot_lookahead public

require bot_utils public
require gameplay
require math

// Lookahead bot: simulates N turns in a row (ignoring opponent) and picks
// the first move of the best-scoring multi-turn sequence.
//
// Inherits all greedy scoring weights. Extra settings:
//   depth           — how many turns to simulate (1=greedy, 2-3=lookahead)
//   future_discount — multiplier per future turn (0.5 = move 2 worth half)
//   jack_lookahead  — 0=skip jacks in inner sim, N=sample top N positions

struct LookaheadWeights : GreedyWeights {
    depth : int = 2
    future_discount : float = 0.5
    jack_lookahead : int = 8
}

// Evolved defaults (60 gen ES, 200 games/pair, depth=2, jack=8)
let LOOKAHEAD_DEFAULT = LookaheadWeights(
    sequence_complete = 11963.0,
    line_4 = 232.0,
    line_3 = 10.6,
    line_2 = 1.7,
    gap_4 = 645.9,
    gap_3 = 2.2,
    block_4 = 731.1,
    block_3 = 24.0,
    block_gap_4 = 131.4,
    adjacency = 1.7,
    center = 2.3,
    disrupt_4 = 1427.1,
    disrupt_3 = 24.0,
    disrupt_2 = 7.0,
    useful_after = 20.3,
    future_discount = 0.5
)

// Collect unique cards in hand, excluding the card at skip_idx.
def private unique_hand_cards(game : GameState; player_idx, skip_idx : int) : array<Card> {
    var seen : array<Card>
    for (c in range(get_hand_size(game, player_idx))) {
        if (c == skip_idx) {
            continue
        }
        let card = get_hand_card(game, player_idx, c)
        var dup = false
        for (s in seen) {
            if (s == card) {
                dup = true
                break
            }
        }
        if (!dup) {
            seen |> push(card)
        }
    }
    return <- seen
}

// Collect unique cards from an array, excluding a specific card value.
def private unique_cards_excluding(cards : array<Card>; exclude : Card) : array<Card> {
    var seen : array<Card>
    for (card in cards) {
        if (card == exclude) {
            continue
        }
        var dup = false
        for (s in seen) {
            if (s == card) {
                dup = true
                break
            }
        }
        if (!dup) {
            seen |> push(card)
        }
    }
    return <- seen
}

// Sample top N positions by score for jacks in inner lookahead.
def private sample_jack_positions(game : GameState; card : Card; player_idx : int;
                                  w : GreedyWeights; max_count : int) : array<int2> {
    let color = player_chip_color(player_idx)
    var all_pos <- card_positions(game, card, color)
    if (length(all_pos) <= max_count) {
        return <- all_pos
    }
    // Score each position, keep top max_count
    var scored : array<float>
    scored |> resize(length(all_pos))
    for (i in range(length(all_pos))) {
        let pos = all_pos[i]
        if (is_black_jack(card)) {
            scored[i] = score_removal(game, pos.x, pos.y, player_idx, w)
        } else {
            scored[i] = score_placement(game, pos.x, pos.y, player_idx, w)
        }
    }
    // Simple selection: find top max_count by repeated max extraction
    var result : array<int2>
    for (_ in range(max_count)) {
        var best_i = -1
        var best_s = -1.0e10
        for (i in range(length(all_pos))) {
            if (scored[i] > best_s) {
                best_s = scored[i]
                best_i = i
            }
        }
        if (best_i >= 0) {
            result |> push(all_pos[best_i])
            scored[best_i] = -1.0e20  // mark as used
        }
    }
    delete all_pos
    delete scored
    return <- result
}

// Get positions for a card in inner sim, respecting jack_lookahead sampling.
def private inner_positions(game : GameState; card : Card; player_idx : int;
                            w : GreedyWeights; jack_lookahead : int) : array<int2> {
    if (jack_lookahead == 0 && is_jack(card)) {
        var empty : array<int2>
        return <- empty
    }
    if (jack_lookahead > 0 && is_jack(card)) {
        return <- sample_jack_positions(game, card, player_idx, w, jack_lookahead)
    }
    let color = player_chip_color(player_idx)
    return <- card_positions(game, card, color)
}

// Score a single move on a game state.
def private score_move(game : GameState; card : Card; row, col : int;
                       player_idx : int; w : GreedyWeights) : float {
    if (is_black_jack(card)) {
        return score_removal(game, row, col, player_idx, w)
    } else {
        return score_placement(game, row, col, player_idx, w)
    }
}

// Apply a move to board_chips in-place (lightweight, no hand/pile changes).
def private sim_apply(var game : GameState; card : Card; row, col : int; color : ChipColor) {
    if (is_black_jack(card)) {
        game.board_chips[row * BOARD_COLS + col] = ChipColor.none
    } else {
        game.board_chips[row * BOARD_COLS + col] = color
    }
}

// Evaluate best future score from depth-2 onward.
def private eval_future(game : GameState; remaining_cards : array<Card>;
                        player_idx : int; w : LookaheadWeights;
                        current_depth, max_depth : int) : float {
    if (current_depth > max_depth || length(remaining_cards) == 0) {
        return 0.0
    }
    let color = player_chip_color(player_idx)
    var best_future = 0.0
    for (card in remaining_cards) {
        var positions <- inner_positions(game, card, player_idx, w, w.jack_lookahead)
        for (pos in positions) {
            let s = score_move(game, card, pos.x, pos.y, player_idx, w)
            var deeper = 0.0
            if (current_depth < max_depth) {
                var sim := game
                sim_apply(sim, card, pos.x, pos.y, color)
                var next_cards <- unique_cards_excluding(remaining_cards, card)
                deeper = eval_future(sim, next_cards, player_idx, w, current_depth + 1, max_depth)
                delete next_cards
            }
            let total = s + w.future_discount * deeper
            if (total > best_future) {
                best_future = total
            }
        }
        delete positions
    }
    return best_future
}

// Main entry point — with explicit weights
def bot_lookahead_move(game : GameState; player_idx : int; seed : int; w : LookaheadWeights) : Move {
    var moves <- legal_moves(game, player_idx)
    let n = length(moves)
    if (n == 0) {
        return Move()
    }
    // Depth 1 = pure greedy
    if (w.depth <= 1 || get_hand_size(game, player_idx) <= 1) {
        var best_score = -1.0e10
        var best_idx = 0
        var num_tied = 0
        var rng = seed
        for (i in range(n)) {
            let m = moves[i]
            let card = get_hand_card(game, player_idx, m.card_idx)
            let score = score_move(game, card, m.row, m.col, player_idx, w)
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
    // Depth 2+: lookahead
    let color = player_chip_color(player_idx)
    var best_score = -1.0e10
    var best_idx = 0
    var num_tied = 0
    var rng = seed
    for (i in range(n)) {
        let m = moves[i]
        let card = get_hand_card(game, player_idx, m.card_idx)
        let score1 = score_move(game, card, m.row, m.col, player_idx, w)
        // Create sim state and apply first move
        var sim <- make_sim_state(game)
        sim_apply(sim, card, m.row, m.col, color)
        // Get remaining cards for future turns
        var remaining <- unique_hand_cards(game, player_idx, m.card_idx)
        let future = eval_future(sim, remaining, player_idx, w, 2, w.depth)
        delete remaining
        let total = score1 + w.future_discount * future
        if (total > best_score) {
            best_score = total
            best_idx = i
            num_tied = 1
        } elif (total == best_score) {
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
def bot_lookahead_move(game : GameState; player_idx : int; seed : int) : Move {
    return bot_lookahead_move(game, player_idx, seed, LOOKAHEAD_DEFAULT)
}

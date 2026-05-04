options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require gameplay
require bot_random
require bot_greedy
require bot_lookahead

var game = GameState()

[test]
def test_bot_random_returns_legal_move(t : T?) {
    t |> run("random bot always returns a legal move") @(t : T?) {
        make_game(game, 2, 42)
        let m = bot_random_move(game, 0, 100)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 0, m), "move should be legal")
    }
    t |> run("random bot returns legal move for player 1") @(t : T?) {
        make_game(game, 2, 42)
        let m = bot_random_move(game, 1, 200)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 1, m), "move should be legal")
    }
}

[test]
def test_bot_random_different_seeds(t : T?) {
    t |> run("different seeds can produce different moves") @(t : T?) {
        make_game(game, 2, 42)
        var saw_different = false
        let m0 = bot_random_move(game, 0, 1)
        for (s in range(2, 20)) {
            let m = bot_random_move(game, 0, s)
            if (m.row != m0.row || m.col != m0.col) {
                saw_different = true
                break
            }
        }
        t |> success(saw_different, "different seeds should eventually pick different moves")
    }
}

[test]
def test_bot_random_handles_jacks(t : T?) {
    t |> run("bot with only a red jack plays it on an open cell") @(t : T?) {
        make_game(game, 2, 1)
        // Give player 0 only a red jack
        var hand : array<Card>
        hand |> push(Card.heart_jack)
        set_hand(game, 0, hand)
        delete hand
        let m = bot_random_move(game, 0, 50)
        t |> success(m.card_idx == 0, "should play the only card (red jack)")
        t |> success(is_legal_move(game, 0, m), "red jack move should be legal")
        // Red jack target should be an empty cell
        t |> equal(int(get_chip(game, m.row, m.col)), int(ChipColor.none))
    }
    t |> run("bot with only a black jack plays it on opponent chip") @(t : T?) {
        make_game(game, 2, 1)
        // Place an opponent chip
        place_chip(game, 2, 3, ChipColor.green)
        // Give player 0 only a black jack
        var hand2 : array<Card>
        hand2 |> push(Card.spade_jack)
        set_hand(game, 0, hand2)
        delete hand2
        let m = bot_random_move(game, 0, 50)
        t |> success(m.card_idx >= 0, "should find a move with black jack")
        t |> success(is_legal_move(game, 0, m), "black jack move should be legal")
    }
}

[test]
def test_bot_random_no_moves(t : T?) {
    t |> run("bot returns empty move when no legal moves exist") @(t : T?) {
        make_game(game, 2, 1)
        // Give player an empty hand
        game.hand_sizes[0] = 0
        let m = bot_random_move(game, 0, 50)
        t |> equal(m.card_idx, -1)
    }
}

[test]
def test_bot_random_many_moves_all_legal(t : T?) {
    t |> run("100 bot moves are all legal across different game states") @(t : T?) {
        make_game(game, 2, 99)
        for (s in range(100)) {
            let p = game.current_player
            discard_dead_cards(game, p)
            let m = bot_random_move(game, p, s)
            if (m.card_idx >= 0) {
                t |> success(is_legal_move(game, p, m), "move {s} should be legal")
                apply_move(game, p, m)
                if (game.phase == GamePhase.game_over) {
                    break
                }
            }
        }
    }
}

[test]
def test_bot_random_full_game(t : T?) {
    t |> run("two random bots can play a full game to completion") @(t : T?) {
        make_game(game, 2, 777)
        var moves_played = 0
        while (game.phase == GamePhase.playing && moves_played < 500) {
            let p = game.current_player
            discard_dead_cards(game, p)
            let m = bot_random_move(game, p, moves_played + 1)
            if (m.card_idx >= 0) {
                apply_move(game, p, m)
                moves_played++
            } else {
                break
            }
        }
        t |> success(moves_played > 0, "should play at least one move")
        // Game may or may not finish in 500 moves, but should not crash
    }
}

// --- Greedy bot tests (via bot_utils extraction) ---

[test]
def test_bot_greedy_returns_legal_move(t : T?) {
    t |> run("greedy bot always returns a legal move") @(t : T?) {
        make_game(game, 2, 42)
        let m = bot_greedy_move(game, 0, 100)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 0, m), "move should be legal")
    }
}

[test]
def test_bot_greedy_full_game(t : T?) {
    t |> run("two greedy bots play a full game to completion") @(t : T?) {
        make_game(game, 2, 777)
        var moves_played = 0
        while (game.phase == GamePhase.playing && moves_played < 500) {
            let p = game.current_player
            discard_dead_cards(game, p)
            let m = bot_greedy_move(game, p, moves_played + 1)
            if (m.card_idx >= 0) {
                apply_move(game, p, m)
                moves_played++
            } else {
                break
            }
        }
        t |> success(moves_played > 0, "should play at least one move")
    }
}

// --- Lookahead bot tests ---

[test]
def test_bot_lookahead_returns_legal_move(t : T?) {
    t |> run("lookahead bot returns a legal move") @(t : T?) {
        make_game(game, 2, 42)
        let m = bot_lookahead_move(game, 0, 100)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 0, m), "move should be legal")
    }
    t |> run("lookahead bot returns legal move for player 1") @(t : T?) {
        make_game(game, 2, 42)
        game.current_player = 1
        let m = bot_lookahead_move(game, 1, 200)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 1, m), "move should be legal")
    }
}

[test]
def test_bot_lookahead_depth1_like_greedy(t : T?) {
    t |> run("depth=1 lookahead behaves like greedy") @(t : T?) {
        make_game(game, 2, 42)
        let w = LookaheadWeights(depth = 1)
        let m_la = bot_lookahead_move(game, 0, 100, w)
        let m_gr = bot_greedy_move(game, 0, 100)
        t |> equal(m_la.row, m_gr.row)
        t |> equal(m_la.col, m_gr.col)
    }
}

[test]
def test_bot_lookahead_no_moves(t : T?) {
    t |> run("lookahead bot returns empty move when no legal moves") @(t : T?) {
        make_game(game, 2, 1)
        game.hand_sizes[0] = 0
        let m = bot_lookahead_move(game, 0, 50)
        t |> equal(m.card_idx, -1)
    }
}

[test]
def test_bot_lookahead_many_moves_all_legal(t : T?) {
    t |> run("100 lookahead moves are all legal") @(t : T?) {
        make_game(game, 2, 99)
        for (s in range(100)) {
            let p = game.current_player
            discard_dead_cards(game, p)
            let m = bot_lookahead_move(game, p, s)
            if (m.card_idx >= 0) {
                t |> success(is_legal_move(game, p, m), "move {s} should be legal")
                apply_move(game, p, m)
                if (game.phase == GamePhase.game_over) {
                    break
                }
            }
        }
    }
}

[test]
def test_bot_lookahead_full_game(t : T?) {
    t |> run("two lookahead bots play a full game to completion") @(t : T?) {
        make_game(game, 2, 777)
        var moves_played = 0
        while (game.phase == GamePhase.playing && moves_played < 500) {
            let p = game.current_player
            discard_dead_cards(game, p)
            let m = bot_lookahead_move(game, p, moves_played + 1)
            if (m.card_idx >= 0) {
                apply_move(game, p, m)
                moves_played++
            } else {
                break
            }
        }
        t |> success(moves_played > 0, "should play at least one move")
    }
}

[test]
def test_bot_lookahead_depth3(t : T?) {
    t |> run("depth=3 lookahead returns legal move without crash") @(t : T?) {
        make_game(game, 2, 42)
        let w = LookaheadWeights(depth = 3)
        let m = bot_lookahead_move(game, 0, 100, w)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 0, m), "move should be legal")
    }
}

[test]
def test_bot_lookahead_jack_lookahead_zero(t : T?) {
    t |> run("jack_lookahead=0 skips jacks in inner sim without crash") @(t : T?) {
        make_game(game, 2, 42)
        let w = LookaheadWeights(jack_lookahead = 0)
        let m = bot_lookahead_move(game, 0, 100, w)
        t |> success(m.card_idx >= 0, "bot should find a move")
        t |> success(is_legal_move(game, 0, m), "move should be legal")
    }
}

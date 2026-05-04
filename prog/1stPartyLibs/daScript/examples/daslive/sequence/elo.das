options gen2

module elo public

require gameplay public
require bot_random public
require bot_greedy public
require bot_lookahead public
require math
require daslib/jobque_boost

// ELO rating system + headless tournament runner.
// Run: bin/Release/daslang.exe examples/daslive/sequence/elo.das
// JIT: bin/Release/daslang.exe -jit examples/daslive/sequence/elo.das

let ELO_K = 32.0
let ELO_START = 1500.0

struct BotEntry {
    name : string
    bot_type : string       // "random", "greedy", or "lookahead"
    weights : GreedyWeights = GreedyWeights()
    la_weights : LookaheadWeights = LookaheadWeights()
    elo : float
    wins : int
    losses : int
    draws : int
}

def make_random_bot(name : string) : BotEntry {
    return BotEntry(name = name, bot_type = "random", elo = ELO_START)
}

def make_greedy_bot(name : string; w : GreedyWeights = GreedyWeights()) : BotEntry {
    return BotEntry(name = name, bot_type = "greedy", weights = w, elo = ELO_START)
}

def make_lookahead_bot(name : string; w : LookaheadWeights = LookaheadWeights()) : BotEntry {
    return BotEntry(name = name, bot_type = "lookahead", la_weights = w, elo = ELO_START)
}

def elo_expected(rating_a, rating_b : float) : float {
    return 1.0 / (1.0 + pow(10.0, (rating_b - rating_a) / 400.0))
}

def elo_update(var a_elo, b_elo : float&; a_won : bool) {
    let ea = elo_expected(a_elo, b_elo)
    let eb = 1.0 - ea
    let sa = a_won ? 1.0 : 0.0
    let sb = a_won ? 0.0 : 1.0
    a_elo += ELO_K * (sa - ea)
    b_elo += ELO_K * (sb - eb)
}

def pick_move(bot : BotEntry; game : GameState; player_idx, seed : int) : Move {
    if (bot.bot_type == "random") {
        return bot_random_move(game, player_idx, seed)
    } elif (bot.bot_type == "greedy") {
        return bot_greedy_move(game, player_idx, seed, bot.weights)
    } elif (bot.bot_type == "lookahead") {
        return bot_lookahead_move(game, player_idx, seed, bot.la_weights)
    }
    return Move()
}

// Play a single headless game. Returns 0 if bot_a wins, 1 if bot_b wins, -1 for draw.
def play_headless_game(bot_a, bot_b : BotEntry; seed : int; max_turns : int = 500) : int {
    var game = GameState()
    make_game(game, 2, seed)
    var turn = 0
    var rng = seed
    while (game.phase == GamePhase.playing && turn < max_turns) {
        let p = game.current_player
        rng++
        var m = Move()
        if (p == 0) {
            m = pick_move(bot_a, game, p, rng)
        } else {
            m = pick_move(bot_b, game, p, rng)
        }
        if (m.card_idx < 0) {
            discard_dead_cards(game, p)
            game.current_player = (game.current_player + 1) % game.num_players
            turn++
            continue
        }
        apply_move(game, p, m)
        turn++
    }
    if (game.phase == GamePhase.game_over) {
        return game.winner
    }
    return -1
}

// --- Result struct for parallel game evaluation ---

struct GameResult {
    winner : int  // 0=a_wins, 1=b_wins, -1=draw
}

// Play all games for a bot pair in parallel, return (a_wins, b_wins, draws).
def play_pair_parallel(bot_a, bot_b : BotEntry; num_games, base_seed : int) : int3 {
    var a_wins = 0
    var b_wins = 0
    var draws = 0
    with_channel(num_games) $(ch) {
        with_wait_group(num_games) $(wg) {
            for (g in range(num_games)) {
                let seed = base_seed + g
                let even = g % 2 == 0
                new_job() @() {
                    var winner : int
                    if (even) {
                        winner = play_headless_game(bot_a, bot_b, seed)
                    } else {
                        let w = play_headless_game(bot_b, bot_a, seed)
                        winner = w < 0 ? -1 : (w == 0 ? 1 : 0)
                    }
                    ch |> push_clone(GameResult(winner = winner))
                    ch |> notify_and_release
                    wg |> done
                }
            }
        }
        ch |> for_each_clone() $(r : GameResult#) {
            if (r.winner == 0) {
                a_wins++
            } elif (r.winner == 1) {
                b_wins++
            } else {
                draws++
            }
        }
    }
    return int3(a_wins, b_wins, draws)
}

// Quiet tournament — no per-pair printing, just updates bots in-place with ELO/wins/losses.
def run_tournament_quiet(var bots : array<BotEntry>; num_games : int; gen_seed : int = 42) {
    let n = length(bots)
    var game_seed = gen_seed
    with_job_que() {
        for (a in range(n)) {
            for (b in range(a + 1, n)) {
                let result = play_pair_parallel(bots[a], bots[b], num_games, game_seed)
                game_seed += num_games
                let a_wins = result.x
                let b_wins = result.y
                let draws = result.z
                // ELO updates are sequential (order-dependent)
                for (g in range(a_wins)) {
                    elo_update(bots[a].elo, bots[b].elo, true)
                }
                for (g in range(b_wins)) {
                    elo_update(bots[a].elo, bots[b].elo, false)
                }
                bots[a].wins += a_wins
                bots[a].losses += b_wins
                bots[a].draws += draws
                bots[b].wins += b_wins
                bots[b].losses += a_wins
                bots[b].draws += draws
            }
        }
    }
}

def run_tournament(var bots : array<BotEntry>; num_games : int) {
    let n = length(bots)
    print("Tournament: {num_games} games per pair, {n} bots\n")
    print("---\n")
    var game_seed = 42
    with_job_que() {
        for (a in range(n)) {
            for (b in range(a + 1, n)) {
                let result = play_pair_parallel(bots[a], bots[b], num_games, game_seed)
                game_seed += num_games
                let a_wins = result.x
                let b_wins = result.y
                let draws = result.z
                // ELO updates are sequential (order-dependent)
                for (g in range(a_wins)) {
                    elo_update(bots[a].elo, bots[b].elo, true)
                }
                for (g in range(b_wins)) {
                    elo_update(bots[a].elo, bots[b].elo, false)
                }
                bots[a].wins += a_wins
                bots[a].losses += b_wins
                bots[a].draws += draws
                bots[b].wins += b_wins
                bots[b].losses += a_wins
                bots[b].draws += draws
                let pct = num_games > 0 ? float(a_wins) * 100.0 / float(num_games) : 0.0
                print("{bots[a].name} vs {bots[b].name}: {a_wins}W {b_wins}L {draws}D ({pct:.1f}%)\n")
            }
        }
    }
    // Sort by ELO descending
    for (i in range(1, n)) {
        var j = i
        while (j > 0 && bots[j].elo > bots[j - 1].elo) {
            let tmp = bots[j]
            bots[j] = bots[j - 1]
            bots[j - 1] = tmp
            j--
        }
    }
    print("\n--- Final Ratings ---\n")
    for (bot in bots) {
        print("{bot.name}: ELO={int(bot.elo)}  W={bot.wins} L={bot.losses} D={bot.draws}\n")
    }
}


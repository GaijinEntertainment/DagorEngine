options gen2

// (1+λ) Evolution Strategy for tuning bot weights.
// Supports both greedy and lookahead bots.
//
// Usage:
//   bin/Release/daslang.exe examples/daslive/sequence/evolve.das -- --bot greedy
//   bin/Release/daslang.exe examples/daslive/sequence/evolve.das -- --bot lookahead
//   bin/Release/daslang.exe examples/daslive/sequence/evolve.das -- --bot lookahead --depth 3 --jack 4
//
// JIT: bin/Release/daslang.exe -jit examples/daslive/sequence/evolve.das -- --bot lookahead

require elo
require math
require strings
require daslib/random
require daslib/jobque_boost

// --- ES parameters ---
let ES_LAMBDA = 63         // children per generation (saturate 64c/128t CPU)
let ES_GENERATIONS = 60
let ES_GAMES_PER_PAIR = 200
let ES_SIGMA_INIT = 0.3
let ES_SIGMA_DECAY = 0.95
let ES_SIGMA_MIN = 0.02
let ES_MAX_WEIGHT = 50000.0
let ES_MIN_SCALE = 1.0     // floor for proportional noise (prevents zero-stuck)

// --- Weight names for printing ---
let GREEDY_WEIGHT_NAMES = fixed_array(
    "seq_complete", "line_4", "line_3", "line_2",
    "gap_4", "gap_3",
    "block_4", "block_3", "block_gap_4",
    "adjacency", "center",
    "disrupt_4", "disrupt_3", "disrupt_2", "useful_after"
)

let LOOKAHEAD_WEIGHT_NAMES = fixed_array(
    "seq_complete", "line_4", "line_3", "line_2",
    "gap_4", "gap_3",
    "block_4", "block_3", "block_gap_4",
    "adjacency", "center",
    "disrupt_4", "disrupt_3", "disrupt_2", "useful_after",
    "future_discount"
)

// --- Weight <-> array<float> conversion ---

def greedy_weights_to_array(w : GreedyWeights) : array<float> {
    return <- [w.sequence_complete, w.line_4, w.line_3, w.line_2,
        w.gap_4, w.gap_3,
        w.block_4, w.block_3, w.block_gap_4,
        w.adjacency, w.center,
        w.disrupt_4, w.disrupt_3, w.disrupt_2, w.useful_after]
}

def array_to_greedy_weights(a : array<float>) : GreedyWeights {
    return GreedyWeights(
        sequence_complete = a[0],
        line_4 = a[1],
        line_3 = a[2],
        line_2 = a[3],
        gap_4 = a[4],
        gap_3 = a[5],
        block_4 = a[6],
        block_3 = a[7],
        block_gap_4 = a[8],
        adjacency = a[9],
        center = a[10],
        disrupt_4 = a[11],
        disrupt_3 = a[12],
        disrupt_2 = a[13],
        useful_after = a[14]
    )
}

def lookahead_weights_to_array(w : LookaheadWeights) : array<float> {
    var a <- greedy_weights_to_array(w)
    a |> push(w.future_discount)
    return <- a
}

def array_to_lookahead_weights(a : array<float>; depth : int; jack_lookahead : int) : LookaheadWeights {
    return LookaheadWeights(
        sequence_complete = a[0],
        line_4 = a[1],
        line_3 = a[2],
        line_2 = a[3],
        gap_4 = a[4],
        gap_3 = a[5],
        block_4 = a[6],
        block_3 = a[7],
        block_gap_4 = a[8],
        adjacency = a[9],
        center = a[10],
        disrupt_4 = a[11],
        disrupt_3 = a[12],
        disrupt_2 = a[13],
        useful_after = a[14],
        future_discount = a[15],
        depth = depth,
        jack_lookahead = jack_lookahead
    )
}

// --- Gaussian RNG via Box-Muller ---

def random_gaussian(var seed : int4&) : float {
    var u1 = random_float(seed)
    if (u1 < 1.0e-7) {
        u1 = 1.0e-7
    }
    let u2 = random_float(seed)
    return sqrt(-2.0 * log(u1)) * cos(2.0 * PI * u2)
}

// --- Mutation ---

def mutate_weights(parent : array<float>; sigma : float; var seed : int4&; num_greedy : int) : array<float> {
    var child : array<float>
    for (i in range(length(parent))) {
        let v = parent[i]
        let noise = random_gaussian(seed)
        if (i < num_greedy) {
            // Greedy weights: proportional noise with floor, clamped to [0, MAX]
            let scale = max(abs(v), ES_MIN_SCALE)
            child |> push(clamp(v + sigma * scale * noise, 0.0, ES_MAX_WEIGHT))
        } else {
            // future_discount: small fixed-scale noise, clamped to [0.01, 2.0]
            let scale = 0.1
            child |> push(clamp(v + sigma * scale * noise, 0.01, 2.0))
        }
    }
    return <- child
}

// --- Print weights compactly ---

def print_weights(a : array<float>; names : auto) {
    let n = length(names)
    for (i in range(n)) {
        if (i >= length(a)) {
            break
        }
        if (i > 0) {
            print(", ")
        }
        let name = names[i]
        let val = a[i]
        print("{name}={val:.1f}")
    }
    print("\n")
}

// --- Make bot from weight array ---

def make_bot_from_array(a : array<float>; bot_type : string; depth, jack_lookahead : int) : BotEntry {
    if (bot_type == "lookahead") {
        let w = array_to_lookahead_weights(a, depth, jack_lookahead)
        return make_lookahead_bot("child", w)
    }
    let w = array_to_greedy_weights(a)
    return make_greedy_bot("child", w)
}

// --- Play N games between two weight arrays, return wins for A ---

def head_to_head(a_arr, b_arr : array<float>; bot_type : string; depth, jack_lookahead : int;
                 num_games, base_seed : int) : int {
    let bot_a = make_bot_from_array(a_arr, bot_type, depth, jack_lookahead)
    let bot_b = make_bot_from_array(b_arr, bot_type, depth, jack_lookahead)
    let result = play_pair_parallel(bot_a, bot_b, num_games, base_seed)
    return result.x
}

// --- Result struct for channel communication ---

struct EvalResult {
    child_idx : int
    wins : int
}

// --- CLI argument parsing ---

def find_arg(args : array<string>; name : string; default_val : string) : string {
    let idx = args |> find_index(name)
    if (idx >= 0 && idx + 1 < length(args)) {
        return args[idx + 1]
    }
    return default_val
}

def find_int_arg(args : array<string>; name : string; default_val : int) : int {
    let s = find_arg(args, name, "")
    if (!empty(s)) {
        return to_int(s)
    }
    return default_val
}

// --- ES main loop ---

[export]
def main() {
    var args <- get_command_line_arguments()
    let bot_type = find_arg(args, "--bot", "greedy")
    let depth = find_int_arg(args, "--depth", 2)
    let jack_lookahead = find_int_arg(args, "--jack", 8)

    let is_lookahead = bot_type == "lookahead"
    let num_greedy = length(GREEDY_WEIGHT_NAMES)

    print("(1+{ES_LAMBDA}) ES — {ES_GENERATIONS} gen, {ES_GAMES_PER_PAIR} games/pair, bot={bot_type}")
    if (is_lookahead) {
        print(" depth={depth} jack={jack_lookahead}")
    }
    print(" [parallel]\n\n")

    var seed = random_seed(12345)
    var sigma = ES_SIGMA_INIT
    var parent : array<float>
    if (is_lookahead) {
        parent <- lookahead_weights_to_array(LookaheadWeights(depth = depth, jack_lookahead = jack_lookahead))
    } else {
        parent <- greedy_weights_to_array(GreedyWeights())
    }
    let t_start = ref_time_ticks()

    with_job_que() {
        for (gen in range(ES_GENERATIONS)) {
            // Generate λ children (sequential — fast, needs shared seed)
            var children : array<array<float>>
            for (i in range(ES_LAMBDA)) {
                children |> emplace(mutate_weights(parent, sigma, seed, num_greedy))
            }

            // Evaluate all children vs parent in parallel
            var best_child_idx = -1
            var best_child_wins = 0
            with_channel(ES_LAMBDA) $(ch) {
                with_wait_group(ES_LAMBDA) $(wg) {
                    for (i in range(ES_LAMBDA)) {
                        let child_idx = i
                        let child_bot = make_bot_from_array(children[i], bot_type, depth, jack_lookahead)
                        let parent_bot = make_bot_from_array(parent, bot_type, depth, jack_lookahead)
                        let base_seed = gen * 10000 + i * 1000
                        new_job() @() {
                            let result = play_pair_parallel(child_bot, parent_bot, ES_GAMES_PER_PAIR, base_seed)
                            ch |> push_clone(EvalResult(child_idx = child_idx, wins = result.x))
                            ch |> notify_and_release
                            wg |> done
                        }
                    }
                }

                // Collect results
                ch |> for_each_clone() $(r : EvalResult#) {
                    if (r.wins > best_child_wins) {
                        best_child_wins = r.wins
                        best_child_idx = r.child_idx
                    }
                }
            }

            // Select winner: child must win majority to replace parent
            let parent_wins = ES_GAMES_PER_PAIR - best_child_wins
            if (best_child_idx >= 0 && best_child_wins > parent_wins) {
                delete parent
                parent <- children[best_child_idx]
                let pct = float(best_child_wins) * 100.0 / float(ES_GAMES_PER_PAIR)
                print("Gen {gen}: child_{best_child_idx} selected ({pct:.0f}%)  sigma={sigma:.4f}\n")
            } else {
                print("Gen {gen}: parent retained     sigma={sigma:.4f}\n")
            }

            // Quick baseline check — parent vs default
            var default_arr : array<float>
            if (is_lookahead) {
                default_arr <- lookahead_weights_to_array(LookaheadWeights(depth = depth, jack_lookahead = jack_lookahead))
            } else {
                default_arr <- greedy_weights_to_array(GreedyWeights())
            }
            let vs_default_wins = head_to_head(parent, default_arr, bot_type, depth, jack_lookahead, ES_GAMES_PER_PAIR, gen * 10000 + 9500)
            delete default_arr
            print("  vs_default: {vs_default_wins}/{ES_GAMES_PER_PAIR}\n")
            print("  weights: ")
            if (is_lookahead) {
                print_weights(parent, LOOKAHEAD_WEIGHT_NAMES)
            } else {
                print_weights(parent, GREEDY_WEIGHT_NAMES)
            }

            // Decay sigma
            sigma = max(sigma * ES_SIGMA_DECAY, ES_SIGMA_MIN)

            delete children
        }
    }

    // Final report
    let elapsed_ms = get_time_usec(t_start) / 1000
    print("\nCompleted in {elapsed_ms} ms ({elapsed_ms / 1000} sec)\n")
    let struct_name = is_lookahead ? "LookaheadWeights" : "GreedyWeights"
    print("\n=== Final Optimized Weights ===\n")
    print("{struct_name}(\n")
    if (is_lookahead) {
        let n = length(LOOKAHEAD_WEIGHT_NAMES)
        for (i in range(n)) {
            if (i >= length(parent)) {
                break
            }
            let name = LOOKAHEAD_WEIGHT_NAMES[i]
            let val = parent[i]
            let comma = i < n - 1 ? "," : ""
            print("    {name} = {val:.1f}{comma}\n")
        }
        print("    depth = {depth},\n")
        print("    jack_lookahead = {jack_lookahead}\n")
    } else {
        let n = length(GREEDY_WEIGHT_NAMES)
        for (i in range(n)) {
            if (i >= length(parent)) {
                break
            }
            let name = GREEDY_WEIGHT_NAMES[i]
            let val = parent[i]
            let comma = i < n - 1 ? "," : ""
            print("    {name} = {val:.1f}{comma}\n")
        }
    }
    print(")\n")

    delete parent
}

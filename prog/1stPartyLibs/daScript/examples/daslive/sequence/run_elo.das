options gen2

require elo

[export]
def main() {
    var bots : array<BotEntry>
    bots |> push(make_random_bot("random"))
    bots |> push(make_greedy_bot("greedy"))
    bots |> push(make_lookahead_bot("lookahead"))
    run_tournament(bots, 200)
}

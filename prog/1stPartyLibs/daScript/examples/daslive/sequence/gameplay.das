options gen2
options persistent_heap

module gameplay public

require math

// ============================================================================
// Card enum — replaces string-based card representation
// ============================================================================

// Layout: none=0, then 52 cards ordered by suit (club, diamond, heart, spade)
// × rank (1/ace, 2..10, jack, queen, king). Arithmetic extraction:
//   suit  = (int(card) - 1) / 13   → 0=club, 1=diamond, 2=heart, 3=spade
//   rank  = (int(card) - 1) % 13   → 0=ace, 1=2, ..., 9=10, 10=jack, 11=queen, 12=king

enum Card : int {
    card_none = 0
    club_1 = 1
    club_2
    club_3
    club_4
    club_5
    club_6
    club_7
    club_8
    club_9
    club_10
    club_jack
    club_queen
    club_king
    diamond_1
    diamond_2
    diamond_3
    diamond_4
    diamond_5
    diamond_6
    diamond_7
    diamond_8
    diamond_9
    diamond_10
    diamond_jack
    diamond_queen
    diamond_king
    heart_1
    heart_2
    heart_3
    heart_4
    heart_5
    heart_6
    heart_7
    heart_8
    heart_9
    heart_10
    heart_jack
    heart_queen
    heart_king
    spade_1
    spade_2
    spade_3
    spade_4
    spade_5
    spade_6
    spade_7
    spade_8
    spade_9
    spade_10
    spade_jack
    spade_queen
    spade_king
}

let SUIT_COUNT = 4
let RANK_COUNT = 13

// Suit/rank extraction via arithmetic
def card_suit_idx(card : Card) : int {
    return (int(card) - 1) / RANK_COUNT
}

def card_rank_idx(card : Card) : int {
    return (int(card) - 1) % RANK_COUNT
}

def is_jack(card : Card) : bool {
    return card == Card.club_jack || card == Card.spade_jack || card == Card.heart_jack || card == Card.diamond_jack
}

def is_black_jack(card : Card) : bool {
    return card == Card.club_jack || card == Card.spade_jack
}

def is_red_jack(card : Card) : bool {
    return card == Card.heart_jack || card == Card.diamond_jack
}

// Convert Card enum to dasCards string name (for GPU card lookup).
// E.g. Card.heart_queen → "heart_queen", Card.card_none → ""
let SUIT_NAMES = fixed_array("club", "diamond", "heart", "spade")
let RANK_NAMES = fixed_array("1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "jack", "queen", "king")

def card_to_string(card : Card) : string {
    if (card == Card.card_none) {
        return ""
    }
    let s = card_suit_idx(card)
    let r = card_rank_idx(card)
    return "{SUIT_NAMES[s]}_{RANK_NAMES[r]}"
}

// Make a Card from suit and rank indices
def make_card(suit_idx, rank_idx : int) : Card {
    return unsafe(reinterpret<Card>(suit_idx * RANK_COUNT + rank_idx + 1))
}

// Parse a card name string to Card enum. Returns card_none if invalid.
def string_to_card(name : string) : Card {
    for (s in range(SUIT_COUNT)) {
        for (r in range(RANK_COUNT)) {
            let card = make_card(s, r)
            if (card_to_string(card) == name) {
                return card
            }
        }
    }
    return Card.card_none
}

// ============================================================================
// Board layout — original Sequence board card order
// ============================================================================

// card_none = FREE corner. No jacks appear on the board.
let FREE = Card.card_none

let BOARD_LAYOUT = fixed_array(
    //  col 0                col 1                col 2                col 3                col 4                col 5                col 6                col 7                col 8                col 9
    // Row 0
    FREE,                Card.club_1,         Card.club_king,      Card.club_queen,     Card.club_10,        Card.club_9,         Card.club_8,         Card.club_7,         Card.club_6,         FREE,
    // Row 1
    Card.diamond_1,      Card.spade_7,        Card.spade_8,        Card.spade_9,        Card.spade_10,       Card.spade_queen,    Card.spade_king,     Card.spade_1,        Card.spade_5,        Card.spade_2,
    // Row 2
    Card.diamond_king,   Card.spade_6,        Card.club_10,        Card.club_9,         Card.club_8,         Card.club_7,         Card.club_6,         Card.diamond_2,      Card.club_4,         Card.spade_3,
    // Row 3
    Card.diamond_queen,  Card.club_5,         Card.club_queen,     Card.heart_8,        Card.heart_7,        Card.heart_6,        Card.club_5,         Card.diamond_3,      Card.club_3,         Card.spade_4,
    // Row 4
    Card.diamond_10,     Card.spade_4,        Card.club_king,      Card.heart_9,        Card.heart_2,        Card.heart_5,        Card.club_4,         Card.diamond_4,      Card.club_2,         Card.spade_5,
    // Row 5
    Card.diamond_9,      Card.spade_3,        Card.club_1,         Card.heart_10,       Card.heart_3,        Card.heart_4,        Card.club_3,         Card.diamond_5,      Card.heart_1,        Card.spade_6,
    // Row 6
    Card.diamond_8,      Card.spade_2,        Card.diamond_1,      Card.heart_queen,    Card.heart_king,     Card.heart_1,        Card.club_2,         Card.diamond_6,      Card.heart_king,     Card.spade_7,
    // Row 7
    Card.diamond_7,      Card.heart_2,        Card.diamond_king,   Card.diamond_queen,  Card.diamond_10,     Card.diamond_9,      Card.diamond_8,      Card.diamond_7,      Card.heart_queen,    Card.spade_8,
    // Row 8
    Card.diamond_6,      Card.heart_3,        Card.heart_4,        Card.heart_5,        Card.heart_6,        Card.heart_7,        Card.heart_8,        Card.heart_9,        Card.heart_10,       Card.spade_9,
    // Row 9
    FREE,                Card.diamond_5,      Card.diamond_4,      Card.diamond_3,      Card.diamond_2,      Card.spade_1,        Card.spade_king,     Card.spade_queen,    Card.spade_10,       FREE
)

let BOARD_ROWS = 10
let BOARD_COLS = 10
let BOARD_SIZE = 100

// ============================================================================
// Types
// ============================================================================

enum ChipColor {
    none
    blue
    green
    red
    yellow
}

// ============================================================================
// Board access
// ============================================================================

def board_card(row, col : int) : Card {
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) {
        return Card.card_none
    }
    return BOARD_LAYOUT[row * BOARD_COLS + col]
}

def is_free_space(row, col : int) : bool {
    return board_card(row, col) == FREE
}

def is_corner(row, col : int) : bool {
    return (row == 0 || row == BOARD_ROWS - 1) && (col == 0 || col == BOARD_COLS - 1)
}

// ============================================================================
// Chip helpers
// ============================================================================

def next_chip_color(color : ChipColor) : ChipColor {
    if (color == ChipColor.none) {
        return ChipColor.blue
    } elif (color == ChipColor.blue) {
        return ChipColor.green
    } elif (color == ChipColor.green) {
        return ChipColor.red
    } elif (color == ChipColor.red) {
        return ChipColor.yellow
    }
    return ChipColor.none
}

def chip_color_to_float4(color : ChipColor) : float4 {
    if (color == ChipColor.blue) {
        return float4(0.2, 0.4, 0.9, 0.9)
    } elif (color == ChipColor.green) {
        return float4(0.2, 0.8, 0.3, 0.9)
    } elif (color == ChipColor.red) {
        return float4(0.9, 0.2, 0.2, 0.9)
    } elif (color == ChipColor.yellow) {
        return float4(0.9, 0.8, 0.1, 0.9)
    }
    return float4(1.0, 1.0, 1.0, 1.0)
}

// ============================================================================
// Deck building
// ============================================================================

def build_double_deck() : array<Card> {
    var deck : array<Card>
    deck |> reserve(2 * SUIT_COUNT * RANK_COUNT)
    for (copy in range(2)) {
        for (s in range(SUIT_COUNT)) {
            for (r in range(RANK_COUNT)) {
                deck |> push(make_card(s, r))
            }
        }
    }
    return <- deck
}

def shuffle_deck(var deck : array<Card>&; seed : int) {
    var rng_seed = seed
    let n = length(deck)
    var i = n - 1
    while (i > 0) {
        rng_seed = rng_seed * 1103515245 + 12345
        let j = abs(rng_seed) % (i + 1)
        let tmp = deck[i]
        deck[i] = deck[j]
        deck[j] = tmp
        i--
    }
}

// ============================================================================
// Game state
// ============================================================================

let MAX_PLAYERS = 4
let MAX_HAND_SIZE = 7

enum GamePhase {
    not_started
    playing
    game_over
}

struct Sequence {
    cells : int2[5]
    color : ChipColor
}

struct GameState {
    num_players : int = 2
    current_player : int = 0
    phase : GamePhase = GamePhase.not_started
    hands : Card[MAX_PLAYERS][MAX_HAND_SIZE]
    hand_sizes : int[MAX_PLAYERS]
    draw_pile : array<Card>
    discard_pile : array<Card>
    board_chips : ChipColor[100]
    sequences : array<Sequence>
    winner : int = -1
}

struct Move {
    card_idx : int = -1
    row : int = -1
    col : int = -1
}

def hand_size_for_players(num_players : int) : int {
    if (num_players == 2) {
        return 7
    } elif (num_players == 3) {
        return 6
    }
    return 5
}

def player_chip_color(player_idx : int) : ChipColor {
    if (player_idx == 0) {
        return ChipColor.blue
    } elif (player_idx == 1) {
        return ChipColor.green
    } elif (player_idx == 2) {
        return ChipColor.red
    }
    return ChipColor.yellow
}

// ============================================================================
// Chip state
// ============================================================================

def get_chip(game : GameState; row, col : int) : ChipColor {
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) {
        return ChipColor.none
    }
    return game.board_chips[row * BOARD_COLS + col]
}

def place_chip(var game : GameState; row, col : int; color : ChipColor) : bool {
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) {
        return false
    }
    game.board_chips[row * BOARD_COLS + col] = color
    return true
}

def clear_all_chips(var game : GameState) {
    for (i in range(BOARD_SIZE)) {
        game.board_chips[i] = ChipColor.none
    }
}

// ============================================================================
// Game setup and hand management
// ============================================================================

def make_game(var game : GameState; num_players : int; seed : int = 42) {
    game.num_players = clamp(num_players, 2, MAX_PLAYERS)
    game.current_player = seed >= 0 ? (abs(seed) % num_players) : 0
    game.phase = GamePhase.playing
    game.winner = -1
    for (p in range(MAX_PLAYERS)) {
        game.hand_sizes[p] = 0
        for (c in range(MAX_HAND_SIZE)) {
            game.hands[p][c] = Card.card_none
        }
    }
    game.draw_pile <- build_double_deck()
    shuffle_deck(game.draw_pile, seed)
    game.discard_pile |> clear()
    clear_all_chips(game)
    game.sequences |> clear()
    let hs = hand_size_for_players(game.num_players)
    for (c in range(hs)) {
        for (p in range(game.num_players)) {
            if (length(game.draw_pile) > 0) {
                game.hands[p][game.hand_sizes[p]] = game.draw_pile[length(game.draw_pile) - 1]
                game.hand_sizes[p]++
                game.draw_pile |> resize(length(game.draw_pile) - 1)
            }
        }
    }
}

def draw_card_from_pile(var game : GameState; player_idx : int) : bool {
    if (player_idx < 0 || player_idx >= game.num_players) {
        return false
    }
    if (game.hand_sizes[player_idx] >= MAX_HAND_SIZE) {
        return false
    }
    if (length(game.draw_pile) == 0) {
        return false
    }
    game.hands[player_idx][game.hand_sizes[player_idx]] = game.draw_pile[length(game.draw_pile) - 1]
    game.hand_sizes[player_idx]++
    game.draw_pile |> resize(length(game.draw_pile) - 1)
    return true
}

def remove_card_from_hand(var game : GameState; player_idx, card_idx : int) : Card {
    if (player_idx < 0 || player_idx >= game.num_players) {
        return Card.card_none
    }
    if (card_idx < 0 || card_idx >= game.hand_sizes[player_idx]) {
        return Card.card_none
    }
    let card = game.hands[player_idx][card_idx]
    for (i in range(card_idx, game.hand_sizes[player_idx] - 1)) {
        game.hands[player_idx][i] = game.hands[player_idx][i + 1]
    }
    game.hand_sizes[player_idx]--
    game.hands[player_idx][game.hand_sizes[player_idx]] = Card.card_none
    return card
}

def get_hand_card(game : GameState; player_idx, card_idx : int) : Card {
    if (player_idx < 0 || player_idx >= game.num_players) {
        return Card.card_none
    }
    if (card_idx < 0 || card_idx >= game.hand_sizes[player_idx]) {
        return Card.card_none
    }
    return game.hands[player_idx][card_idx]
}

def get_hand_size(game : GameState; player_idx : int) : int {
    if (player_idx < 0 || player_idx >= game.num_players) {
        return 0
    }
    return game.hand_sizes[player_idx]
}

// ============================================================================
// Dead card detection
// ============================================================================

def is_dead_card(game : GameState; card : Card) : bool {
    if (is_jack(card)) {
        return false
    }
    var occupied = 0
    var total = 0
    for (i in range(BOARD_SIZE)) {
        if (BOARD_LAYOUT[i] == card) {
            total++
            if (game.board_chips[i] != ChipColor.none) {
                occupied++
            }
        }
    }
    return total > 0 && occupied == total
}

def discard_dead_cards(var game : GameState; player_idx : int) {
    var changed = true
    while (changed) {
        changed = false
        for (c in range(game.hand_sizes[player_idx])) {
            let card = game.hands[player_idx][c]
            if (is_dead_card(game, card)) {
                remove_card_from_hand(game, player_idx, c)
                game.discard_pile |> push(card)
                draw_card_from_pile(game, player_idx)
                changed = true
                break
            }
        }
    }
}

// ============================================================================
// Legal moves
// ============================================================================

def legal_moves(game : GameState; player_idx : int) : array<Move> {
    var moves : array<Move>
    let color = player_chip_color(player_idx)
    for (c in range(game.hand_sizes[player_idx])) {
        let card = game.hands[player_idx][c]
        if (is_red_jack(card)) {
            for (i in range(BOARD_SIZE)) {
                let row = i / BOARD_COLS
                let col = i % BOARD_COLS
                if (!is_corner(row, col) && game.board_chips[i] == ChipColor.none) {
                    moves |> push(Move(card_idx = c, row = row, col = col))
                }
            }
        } elif (is_black_jack(card)) {
            for (i in range(BOARD_SIZE)) {
                let row = i / BOARD_COLS
                let col = i % BOARD_COLS
                let chip = game.board_chips[i]
                if (chip != ChipColor.none && chip != color && !is_in_sequence(game, row, col)) {
                    moves |> push(Move(card_idx = c, row = row, col = col))
                }
            }
        } else {
            for (i in range(BOARD_SIZE)) {
                let row = i / BOARD_COLS
                let col = i % BOARD_COLS
                if (BOARD_LAYOUT[i] == card && game.board_chips[i] == ChipColor.none) {
                    moves |> push(Move(card_idx = c, row = row, col = col))
                }
            }
        }
    }
    return <- moves
}

def is_legal_move(game : GameState; player_idx : int; m : Move) : bool {
    if (m.card_idx < 0 || m.card_idx >= game.hand_sizes[player_idx]) {
        return false
    }
    if (m.row < 0 || m.row >= BOARD_ROWS || m.col < 0 || m.col >= BOARD_COLS) {
        return false
    }
    let card = game.hands[player_idx][m.card_idx]
    let idx = m.row * BOARD_COLS + m.col
    let color = player_chip_color(player_idx)
    if (is_red_jack(card)) {
        return !is_corner(m.row, m.col) && game.board_chips[idx] == ChipColor.none
    } elif (is_black_jack(card)) {
        let chip = game.board_chips[idx]
        return chip != ChipColor.none && chip != color && !is_in_sequence(game, m.row, m.col)
    } else {
        return BOARD_LAYOUT[idx] == card && game.board_chips[idx] == ChipColor.none
    }
}

def has_legal_moves(game : GameState; player_idx : int) : bool {
    for (c in range(game.hand_sizes[player_idx])) {
        let card = game.hands[player_idx][c]
        if (is_red_jack(card)) {
            for (i in range(BOARD_SIZE)) {
                if (!is_corner(i / BOARD_COLS, i % BOARD_COLS) && game.board_chips[i] == ChipColor.none) {
                    return true
                }
            }
        } elif (is_black_jack(card)) {
            let color = player_chip_color(player_idx)
            for (i in range(BOARD_SIZE)) {
                let chip = game.board_chips[i]
                if (chip != ChipColor.none && chip != color && !is_in_sequence(game, i / BOARD_COLS, i % BOARD_COLS)) {
                    return true
                }
            }
        } else {
            for (i in range(BOARD_SIZE)) {
                if (BOARD_LAYOUT[i] == card && game.board_chips[i] == ChipColor.none) {
                    return true
                }
            }
        }
    }
    return false
}

// ============================================================================
// Apply move
// ============================================================================

def apply_move(var game : GameState; player_idx : int; move : Move) : bool {
    if (game.phase != GamePhase.playing) {
        return false
    }
    if (player_idx != game.current_player) {
        return false
    }
    if (move.card_idx < 0 || move.card_idx >= game.hand_sizes[player_idx]) {
        return false
    }
    let card = game.hands[player_idx][move.card_idx]
    let color = player_chip_color(player_idx)
    if (is_black_jack(card)) {
        let chip = game.board_chips[move.row * BOARD_COLS + move.col]
        if (chip == ChipColor.none || chip == color) {
            return false
        }
        if (is_in_sequence(game, move.row, move.col)) {
            return false
        }
        game.board_chips[move.row * BOARD_COLS + move.col] = ChipColor.none
    } else {
        if (game.board_chips[move.row * BOARD_COLS + move.col] != ChipColor.none) {
            return false
        }
        game.board_chips[move.row * BOARD_COLS + move.col] = color
    }
    let removed = remove_card_from_hand(game, player_idx, move.card_idx)
    game.discard_pile |> push(removed)
    draw_card_from_pile(game, player_idx)
    discard_dead_cards(game, player_idx)
    find_sequences(game)
    let w = check_win(game)
    if (w >= 0) {
        game.winner = w
        game.phase = GamePhase.game_over
    }
    game.current_player = (game.current_player + 1) % game.num_players
    return true
}

// ============================================================================
// Sequence detection and win condition
// ============================================================================

let SEQUENCE_LENGTH = 5

let DIR_DR = 0
let DIR_DC = 1
let DIR_DD = 2
let DIR_DA = 3

let DIR_ROWS = fixed_array(0, 1, 1, 1)
let DIR_COLS = fixed_array(1, 0, 1, -1)

def cell_matches_color(game : GameState; row, col : int; color : ChipColor) : bool {
    if (is_corner(row, col)) {
        return true
    }
    return game.board_chips[row * BOARD_COLS + col] == color
}

def is_in_sequence(game : GameState; row, col : int) : bool {
    for (seq in game.sequences) {
        for (c in seq.cells) {
            if (c.x == col && c.y == row) {
                return true
            }
        }
    }
    return false
}

def find_sequences(var game : GameState) {
    game.sequences |> clear()
    for (p in range(game.num_players)) {
        let color = player_chip_color(p)
        for (dir in range(4)) {
            let dr = DIR_ROWS[dir]
            let dc = DIR_COLS[dir]
            for (row in range(BOARD_ROWS)) {
                for (col in range(BOARD_COLS)) {
                    let end_row = row + dr * (SEQUENCE_LENGTH - 1)
                    let end_col = col + dc * (SEQUENCE_LENGTH - 1)
                    if (end_row < 0 || end_row >= BOARD_ROWS || end_col < 0 || end_col >= BOARD_COLS) {
                        continue
                    }
                    var all_match = true
                    var shared_count = 0
                    var cells : int2[5]
                    for (s in range(SEQUENCE_LENGTH)) {
                        let r = row + dr * s
                        let c = col + dc * s
                        cells[s] = int2(c, r)
                        if (!cell_matches_color(game, r, c, color)) {
                            all_match = false
                            break
                        }
                        if (is_in_sequence(game, r, c)) {
                            shared_count++
                        }
                    }
                    if (all_match && shared_count <= 1) {
                        game.sequences |> push(Sequence(cells = cells, color = color))
                    }
                }
            }
        }
    }
}

def count_sequences(game : GameState; color : ChipColor) : int {
    var count = 0
    for (seq in game.sequences) {
        if (seq.color == color) {
            count++
        }
    }
    return count
}

def sequences_to_win(num_players : int) : int {
    return num_players == 2 ? 2 : 1
}

def check_win(game : GameState) : int {
    let needed = sequences_to_win(game.num_players)
    for (p in range(game.num_players)) {
        if (count_sequences(game, player_chip_color(p)) >= needed) {
            return p
        }
    }
    return -1
}

// ============================================================================
// Hand management
// ============================================================================

def set_hand(var game : GameState; player_idx : int; cards : array<Card>) {
    if (player_idx < 0 || player_idx >= MAX_PLAYERS) {
        return
    }
    let n = min(length(cards), MAX_HAND_SIZE)
    for (c in range(MAX_HAND_SIZE)) {
        game.hands[player_idx][c] = c < n ? cards[c] : Card.card_none
    }
    game.hand_sizes[player_idx] = n
}

def sort_hand(var game : GameState; player_idx : int) {
    let n = game.hand_sizes[player_idx]
    for (i in range(1, n)) {
        let card = game.hands[player_idx][i]
        let key = int(card)  // enum order is already suit-then-rank
        var j = i - 1
        while (j >= 0) {
            if (int(game.hands[player_idx][j]) > key) {
                game.hands[player_idx][j + 1] = game.hands[player_idx][j]
                j--
            } else {
                break
            }
        }
        game.hands[player_idx][j + 1] = card
    }
}

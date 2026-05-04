options gen2
options persistent_heap

require live/glfw_live
require live/opengl_live
require live/live_api
require live/live_commands
require live/live_vars
require live/live_watch_boost
require live_host
require opengl/opengl_boost
require glfw/glfw_boost
require cards/opengl_cards
require daslib/json_boost
require daslib/safe_addr
require daslib/math_boost
require opengl/opengl_ttf
require daslib/fio
require gameplay
require bot_random
require bot_greedy
require bot_lookahead
require sfx

// --- Chip shader ---

var @in @location chip_v_position : float2
var @inout chip_f_color : float4
var @uniform chip_u_mvp : float4x4
var @uniform chip_u_color : float4

[vertex_program]
def vs_chip {
    chip_f_color = chip_u_color
    gl_Position = chip_u_mvp * float4(chip_v_position, 0.0, 1.0)
}

var @out chip_frag_color : float4

[fragment_program]
def fs_chip {
    chip_frag_color = chip_f_color
}

[vertex_buffer]
struct ChipVertex {
    xy : float2
}

let CHIP_SEGMENTS = 32

struct ChipRenderer {
    program : uint
    fill_vao : uint
    fill_vbo : uint
    outline_vao : uint
    outline_vbo : uint
}

def create_chip_renderer() : ChipRenderer {
    var cr : ChipRenderer
    cr.program = create_shader_program(@@vs_chip, @@fs_chip)
    // Fill circle: triangle fan (center + CHIP_SEGMENTS + 1 vertices)
    let num_fill = CHIP_SEGMENTS + 2
    var fill_vao_tmp : uint
    var fill_vbo_tmp : uint
    glGenVertexArrays(1, safe_addr(fill_vao_tmp))
    glBindVertexArray(fill_vao_tmp)
    glGenBuffers(1, safe_addr(fill_vbo_tmp))
    glBindBuffer(GL_ARRAY_BUFFER, fill_vbo_tmp)
    var fill : array<ChipVertex>
    fill |> resize(num_fill)
    fill[0] = ChipVertex(xy = float2(0.0, 0.0))
    for (i in range(CHIP_SEGMENTS + 1)) {
        let a = 2.0 * PI * float(i) / float(CHIP_SEGMENTS)
        fill[i + 1] = ChipVertex(xy = float2(cos(a), sin(a)))
    }
    glBufferData(GL_ARRAY_BUFFER, fill, GL_STATIC_DRAW)
    bind_vertex_buffer(null, type<ChipVertex>)
    cr.fill_vao = fill_vao_tmp
    cr.fill_vbo = fill_vbo_tmp
    // Outline circle: line loop (CHIP_SEGMENTS vertices)
    var outline_vao_tmp : uint
    var outline_vbo_tmp : uint
    glGenVertexArrays(1, safe_addr(outline_vao_tmp))
    glBindVertexArray(outline_vao_tmp)
    glGenBuffers(1, safe_addr(outline_vbo_tmp))
    glBindBuffer(GL_ARRAY_BUFFER, outline_vbo_tmp)
    var outline : array<ChipVertex>
    outline |> resize(CHIP_SEGMENTS)
    for (i in range(CHIP_SEGMENTS)) {
        let a = 2.0 * PI * float(i) / float(CHIP_SEGMENTS)
        outline[i] = ChipVertex(xy = float2(cos(a), sin(a)))
    }
    glBufferData(GL_ARRAY_BUFFER, outline, GL_STATIC_DRAW)
    bind_vertex_buffer(null, type<ChipVertex>)
    cr.outline_vao = outline_vao_tmp
    cr.outline_vbo = outline_vbo_tmp
    glBindVertexArray(0u)
    return cr
}

def delete_chip_renderer(var cr : ChipRenderer&) {
    if (cr.fill_vao != 0u) {
        var v = cr.fill_vao
        var b = cr.fill_vbo
        glDeleteVertexArrays(1, safe_addr(v))
        glDeleteBuffers(1, safe_addr(b))
        cr.fill_vao = 0u
        cr.fill_vbo = 0u
    }
    if (cr.outline_vao != 0u) {
        var v = cr.outline_vao
        var b = cr.outline_vbo
        glDeleteVertexArrays(1, safe_addr(v))
        glDeleteBuffers(1, safe_addr(b))
        cr.outline_vao = 0u
        cr.outline_vbo = 0u
    }
}

def draw_chip(cr : ChipRenderer; cx, cy, radius : float; color : float4; outline_color : float4;
              screen_w, screen_h : int) {
    let sx = 2.0 / float(screen_w)
    let sy = 2.0 / float(screen_h)
    // MVP: scale by radius, then translate to center, then to NDC
    var mvp : float4x4
    mvp[0] = float4(radius * sx, 0.0, 0.0, 0.0)
    mvp[1] = float4(0.0, -radius * sy, 0.0, 0.0)
    mvp[2] = float4(0.0, 0.0, 1.0, 0.0)
    mvp[3] = float4(cx * sx - 1.0, 1.0 - cy * sy, 0.0, 1.0)
    glUseProgram(cr.program)
    chip_u_mvp = mvp
    // Fill
    chip_u_color = color
    vs_chip_bind_uniform(cr.program)
    fs_chip_bind_uniform(cr.program)
    glBindVertexArray(cr.fill_vao)
    glDrawArrays(GL_TRIANGLE_FAN, 0, CHIP_SEGMENTS + 2)
    gl_stats_draw(int(GL_TRIANGLE_FAN), CHIP_SEGMENTS + 2)
    // Outline
    chip_u_color = outline_color
    vs_chip_bind_uniform(cr.program)
    glBindVertexArray(cr.outline_vao)
    glLineWidth(2.0)
    glDrawArrays(GL_LINE_LOOP, 0, CHIP_SEGMENTS)
    gl_stats_draw(int(GL_LINE_LOOP), CHIP_SEGMENTS)
}

// --- Tunable parameters ---

var @live board_pad = 2.0
var @live bg_color = float3(0.1, 0.15, 0.1)
var @live hover_tint = float4(1.3, 1.3, 1.5, 1.0)
var @live chip_radius_frac = 0.3  // chip radius as fraction of card width
var @live hand_margin = 4.0       // margin from screen edges for hand display
var @live hand_row_gap = 2.0      // gap between suit rows in hand display
var @live bot_stack_offset = 8.0  // pixel offset between stacked bot cards
var @live draw_pile_offset = 3.0 // pixel offset between stacked draw pile cards
var @live draw_pile_max_cards = 8 // max visible cards in draw pile stack
var @live cheat_mode = false      // show all player hands
var @live seq_pulse_speed = 2.0  // speed of sequence highlight pulsing
var @live seq_pulse_min = 0.7   // minimum brightness of pulse
var @live seq_card_tint = 0.35  // how strongly sequence cards are tinted with chip color
var @live bot_delay = 0.5       // seconds between bot moves
var @live bot_mode = 1          // 0=greedy, 1=lookahead
var @live anim_chip_duration = 0.25   // chip bounce duration
var @live anim_remove_duration = 0.3  // chip removal flash duration
var @live anim_card_duration = 0.3    // card fly duration
var @live last_drawn_card = Card.card_none  // last card human received (for highlight)
var @live last_drawn_pulse_speed = 1.5  // pulse speed for last drawn card
var @live last_drawn_pulse_min = 0.7    // minimum brightness (dimmed)
var @live last_drawn_pulse_max = 0.9    // maximum brightness
var @live selected_card = -1      // index of selected card in human hand (-1 = none
var @live bot_timer = 0.0         // countdown until next bot move
var @live bot_rng_seed = 12345    // RNG seed for bot moves, incremented each move

// --- Animation system ---

enum AnimType {
    chip_place    // chip scale-up bounce
    chip_remove   // chip flash and shrink out
    card_fly      // card flies from hand to board cell
    card_draw     // face-down card flies from draw pile to hand
    card_discard  // card flies from hand to discard pile
}

struct Anim {
    atype : AnimType
    row : int
    col : int
    color : ChipColor         // chip color for rendering
    progress : float = 0.0    // 0..1
    duration : float = 0.25
    // Deferred move to apply when animation finishes
    has_deferred_move : bool = false
    deferred_player : int = -1
    deferred_move : Move = Move()
    // card_fly fields
    card_gpu_idx : int = -1   // GPU card index for rendering
    start_x : float = 0.0    // hand card screen position
    start_y : float = 0.0
    start_scale : float = 1.0
}

var anim_queue : array<Anim>

def is_animating() : bool {
    return length(anim_queue) > 0
}

// Compute board cell top-left screen position
def cell_screen_pos(row, col : int) : float2 {
    return float2(board_ox + board_pad_cur + float(col) * (board_cs_w + board_pad_cur),
                  board_oy + board_pad_cur + float(row) * (board_cs_h + board_pad_cur))
}

// Compute approximate hand card screen position for player 0 (human, bottom-left)
def human_card_screen_pos(card_idx : int) : float2 {
    let margin = float(hand_margin)
    let scale = board_scale
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    let left_avail = board_ox - margin * 2.0
    let step_x = compute_hand_step_x(cs_w, left_avail, MAX_HAND_SIZE)
    let row_step = cs_h + float(hand_row_gap)
    // Find which suit row and position within row
    sort_hand(game, 0)
    var suit_starts : int[5]
    var num_suits = 0
    var prev_suit = -1
    for (c in range(get_hand_size(game, 0))) {
        let card = get_hand_card(game, 0, c)
        let s = card_suit_idx(card)
        if (s != prev_suit) {
            suit_starts[num_suits] = c
            num_suits++
            prev_suit = s
        }
    }
    suit_starts[num_suits] = get_hand_size(game, 0)
    let fw = float(screen_w)
    let fh = float(screen_h)
    for (row in range(num_suits)) {
        if (card_idx >= suit_starts[row] && card_idx < suit_starts[row + 1]) {
            let ci = card_idx - suit_starts[row]
            let ry = float(row - num_suits)  // grow_down=false
            let card_x = margin + float(ci) * step_x
            let card_y = fh - margin + ry * row_step
            return float2(card_x, card_y)
        }
    }
    return float2(margin, fh - margin - cs_h)
}

// Compute approximate bot hand screen position (stack anchor)
def bot_card_screen_pos(player_idx : int) : float2 {
    let margin = float(hand_margin)
    let scale = board_scale
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    let fw = float(screen_w)
    let fh = float(screen_h)
    if (player_idx == 1) {
        return float2(margin, margin)  // top-left
    } elif (player_idx == 2) {
        return float2(fw - margin - cs_w, margin)  // top-right
    } elif (player_idx == 3) {
        return float2(fw - margin - cs_w, fh - margin - cs_h)  // bottom-right
    }
    return float2(margin, margin)
}

// Compute discard pile top position (below draw pile)
def discard_pile_screen_pos() : float2 {
    let cs_h = CARD_DOC_HEIGHT * board_scale
    let pile_pos = draw_pile_screen_pos()
    return float2(pile_pos.x, pile_pos.y + cs_h + float(hand_margin))
}

// Queue discard animations for dead cards, then discard them
def queue_discard_anims(player_idx : int) {
    // Find dead cards before discarding
    for (c in range(get_hand_size(game, player_idx))) {
        let card = get_hand_card(game, player_idx, c)
        if (is_dead_card(game, card)) {
            var start_pos : float2
            if (player_idx == 0) {
                start_pos = human_card_screen_pos(c)
            } else {
                start_pos = bot_card_screen_pos(player_idx)
            }
            var a = Anim(atype = AnimType.card_discard, duration = float(anim_card_duration))
            a.card_gpu_idx = card_gpu_index(card)
            a.start_x = start_pos.x
            a.start_y = start_pos.y
            a.start_scale = board_scale
            a.deferred_player = player_idx
            anim_queue |> emplace(a)
        }
    }
    // Actually discard (state changes immediately, animations play visually)
    discard_dead_cards(game, player_idx)
    if (player_idx == 0) {
        sort_hand(game, 0)
    }
}

def queue_chip_place(row, col : int; color : ChipColor; dur : float) {
    anim_queue |> emplace(Anim(atype = AnimType.chip_place, row = row, col = col,
        color = color, duration = dur))
}

def queue_chip_remove(row, col : int; color : ChipColor; dur : float) {
    anim_queue |> emplace(Anim(atype = AnimType.chip_remove, row = row, col = col,
        color = color, duration = dur))
}

// Queue a move that will be applied via apply_move when animation finishes
def queue_animated_move(player_idx : int; m : Move; chip_color : ChipColor;
                        is_black_jack, is_red_jack : bool; target_row, target_col : int) {
    // Card fly animation: card flies from hand to board cell
    let card = get_hand_card(game, player_idx, m.card_idx)
    let gpu_idx = card_gpu_index(card)
    var start_pos : float2
    if (player_idx == 0) {
        start_pos = human_card_screen_pos(m.card_idx)
    } else {
        start_pos = bot_card_screen_pos(player_idx)
    }
    var fly = Anim(atype = AnimType.card_fly, row = target_row, col = target_col,
        color = chip_color, duration = float(anim_card_duration))
    fly.card_gpu_idx = gpu_idx
    fly.start_x = start_pos.x
    fly.start_y = start_pos.y
    fly.start_scale = board_scale
    anim_queue |> emplace(fly)
    // Then chip animation with deferred move
    if (is_black_jack) {
        let victim_color = get_chip(game, target_row, target_col)
        var a = Anim(atype = AnimType.chip_remove, row = target_row, col = target_col,
            color = victim_color, duration = float(anim_remove_duration))
        a.has_deferred_move = true
        a.deferred_player = player_idx
        a.deferred_move = m
        anim_queue |> emplace(a)
    } else {
        var a = Anim(atype = AnimType.chip_place, row = target_row, col = target_col,
            color = chip_color, duration = float(anim_chip_duration))
        a.has_deferred_move = true
        a.deferred_player = player_idx
        a.deferred_move = m
        anim_queue |> emplace(a)
    }
}

def tick_animations(dt : float) {
    if (length(anim_queue) == 0) {
        return
    }
    anim_queue[0].progress += dt / anim_queue[0].duration
    if (anim_queue[0].progress >= 1.0) {
        let a = anim_queue[0]
        // Sound for completed animation
        if (a.atype == AnimType.card_fly) {
            sfx_card_play()
        }
        if (a.has_deferred_move) {
            let player = a.deferred_player
            let seq_before = length(game.sequences)
            if (a.atype == AnimType.chip_place) {
                sfx_chip_place()
            } elif (a.atype == AnimType.chip_remove) {
                sfx_chip_remove()
            }
            // Capture the card about to be drawn (top of draw pile)
            var drawn = Card.card_none
            if (length(game.draw_pile) > 0) {
                drawn = game.draw_pile[length(game.draw_pile) - 1]
            }
            apply_move(game, player, a.deferred_move)
            // Track last drawn card for human player highlight
            if (player == 0 && drawn != Card.card_none) {
                // Check if the drawn card is still in hand (not dead-discarded)
                var found = false
                for (ci in range(get_hand_size(game, 0))) {
                    if (get_hand_card(game, 0, ci) == drawn) {
                        found = true
                        break
                    }
                }
                last_drawn_card = found ? drawn : Card.card_none
            }
            // Check for new sequences or win
            if (game.phase == GamePhase.game_over) {
                sfx_game_win()
            } elif (length(game.sequences) > seq_before) {
                sfx_sequence_complete()
            }
            if (player == 0) {
                sort_hand(game, 0)
            }
            // Queue card draw animation (card flies from draw pile to hand)
            if (length(game.draw_pile) > 0) {
                let pile_pos = draw_pile_screen_pos()
                var target_pos : float2
                if (player == 0) {
                    // Fly to human hand area (bottom-left)
                    target_pos = float2(float(hand_margin), float(screen_h) - float(hand_margin) - CARD_DOC_HEIGHT * board_scale)
                } else {
                    target_pos = bot_card_screen_pos(player)
                }
                var draw_anim = Anim(atype = AnimType.card_draw, duration = float(anim_card_duration))
                draw_anim.start_x = pile_pos.x
                draw_anim.start_y = pile_pos.y
                draw_anim.card_gpu_idx = back_index  // face-down
                // Store target in row/col fields as encoded floats — or use start for source, target goes in separate fields
                // Actually, reuse: start = pile pos, target stored via row/col trick won't work. Let me use the Anim struct differently:
                // start_x/y = draw pile pos, row/col encodes nothing useful here. We'll compute target in rendering.
                draw_anim.deferred_player = player  // to know where to fly to
                anim_queue |> emplace(draw_anim)
                sfx_card_draw()
            }
        }
        // Turn chime after card draw completes (bot just finished its turn)
        if (a.atype == AnimType.card_draw && game.phase == GamePhase.playing && game.current_player == 0) {
            sfx_turn_chime()
        }
        anim_queue |> erase(0)
    }
}

// Returns animation scale for a chip at (row,col), or -1 if no active animation
def get_chip_anim_scale(row, col : int) : float {
    if (length(anim_queue) == 0) {
        return -1.0
    }
    let a = anim_queue[0]
    if (a.row != row || a.col != col) {
        return -1.0
    }
    let t = clamp(a.progress, 0.0, 1.0)
    if (a.atype == AnimType.chip_place) {
        // Bounce: overshoot to 1.2 then settle to 1.0
        if (t < 0.6) {
            return t / 0.6 * 1.2  // grow to 1.2
        } else {
            return 1.2 - (t - 0.6) / 0.4 * 0.2  // settle to 1.0
        }
    } elif (a.atype == AnimType.chip_remove) {
        // Shrink out: 1.0 -> 0.0 with initial flash (scale up briefly)
        if (t < 0.2) {
            return 1.0 + t / 0.2 * 0.3  // flash up to 1.3
        } else {
            return 1.3 * (1.0 - (t - 0.2) / 0.8)  // shrink to 0
        }
    }
    return -1.0
}

// Returns animation alpha multiplier for chip_remove, or -1 if not applicable
def get_chip_anim_alpha(row, col : int) : float {
    if (length(anim_queue) == 0) {
        return -1.0
    }
    let a = anim_queue[0]
    if (a.row != row || a.col != col) {
        return -1.0
    }
    if (a.atype == AnimType.chip_remove) {
        let t = clamp(a.progress, 0.0, 1.0)
        return 1.0 - t  // fade out
    }
    return -1.0
}

// --- State ---

var @live game = GameState()
var renderer : CardRenderer
var chip_renderer : ChipRenderer
var deck = GpuDeck()
var back_index : int = -1
var ui_font : Font
var ui_font_small : Font
var ui_font_loaded : bool = false

// Precomputed card indices for each board cell (100 entries, -1 for FREE corners using back)
var board_card_indices : int[100]

// Board layout (computed each frame for hit-testing)
var board_ox : float = 0.0
var board_oy : float = 0.0
var board_scale : float = 1.0
var board_cs_w : float = 0.0
var board_cs_h : float = 0.0
var board_pad_cur : float = 0.0

// Screen size (updated each frame)
var screen_w : int = 1280
var screen_h : int = 800

// Mouse state
var hover_row : int = -1
var hover_col : int = -1
var mouse_was_pressed : bool = false
var mouse_clicked : bool = false  // true on the frame mouse was first pressed

// --- Card → GPU index lookup ---

// Maps Card enum value → GPU deck index. Computed once at init.
var card_gpu_lut : int[53]  // card_none=0, 52 cards

def compute_card_gpu_lut() {
    card_gpu_lut[0] = -1  // card_none
    for (s in range(SUIT_COUNT)) {
        for (r in range(RANK_COUNT)) {
            let card = make_card(s, r)
            card_gpu_lut[int(card)] = find_card(deck, card_to_string(card))
        }
    }
}

def card_gpu_index(card : Card) : int {
    if (card == Card.card_none) {
        return -1
    }
    return card_gpu_lut[int(card)]
}

// --- Helpers ---

def compute_board_card_indices() {
    for (i in range(BOARD_SIZE)) {
        let bc = BOARD_LAYOUT[i]
        if (bc == Card.card_none) {
            board_card_indices[i] = -1
        } else {
            board_card_indices[i] = card_gpu_index(bc)
        }
    }
}

// Convert screen coords to board cell. Returns (-1,-1) if outside board.
def screen_to_cell(mx, my : float) : int2 {
    let rel_x = mx - board_ox - board_pad_cur
    let rel_y = my - board_oy - board_pad_cur
    if (rel_x < 0.0 || rel_y < 0.0) {
        return int2(-1, -1)
    }
    let step_x = board_cs_w + board_pad_cur
    let step_y = board_cs_h + board_pad_cur
    let col = int(rel_x / step_x)
    let row = int(rel_y / step_y)
    if (col < 0 || col >= BOARD_COLS || row < 0 || row >= BOARD_ROWS) {
        return int2(-1, -1)
    }
    // Check we're within the card, not in padding
    let in_x = rel_x - float(col) * step_x
    let in_y = rel_y - float(row) * step_y
    if (in_x > board_cs_w || in_y > board_cs_h) {
        return int2(-1, -1)
    }
    return int2(col, row)
}

def handle_mouse() {
    let mpos = glfwGetCursorPos(live_window)
    let cell = screen_to_cell(mpos.x, mpos.y)
    hover_col = cell.x
    hover_row = cell.y
    let pressed = glfwGetMouseButton(live_window, int(GLFW_MOUSE_BUTTON_1)) == int(GLFW_PRESS)
    mouse_clicked = pressed && !mouse_was_pressed
    mouse_was_pressed = pressed
    // Right-click deselects
    let right_pressed = glfwGetMouseButton(live_window, int(GLFW_MOUSE_BUTTON_2)) == int(GLFW_PRESS)
    if (right_pressed && selected_card >= 0) {
        selected_card = -1
    }
    if (mouse_clicked) {
        if (game.phase == GamePhase.game_over) {
            start_new_game(game.num_players)
            return
        }
        // Discard dead cards for human before they play
        if (game.phase == GamePhase.playing && game.current_player == 0 && !is_animating()) {
            queue_discard_anims(0)
        }
        if (game.phase == GamePhase.playing && game.current_player == 0 && !is_animating() && selected_card >= 0) {
            // Try to play selected card on hovered board cell
            if (hover_row >= 0 && hover_col >= 0) {
                let m = Move(card_idx = selected_card, row = hover_row, col = hover_col)
                if (is_legal_move(game, 0, m)) {
                    let card = get_hand_card(game, 0, m.card_idx)
                    let is_bj = is_black_jack(card)
                    let is_rj = is_red_jack(card)
                    queue_animated_move(0, m, player_chip_color(0), is_bj, is_rj, hover_row, hover_col)
                    selected_card = -1
                    last_drawn_card = Card.card_none
                    bot_timer = float(bot_delay)
                }
            }
        }
    }
}

// --- Hand display ---

// Compute horizontal step to fit max_cards cards within avail_width pixels
// Returns the x step between card left edges
def compute_hand_step_x(cs_w, avail_width : float; max_cards : int) : float {
    if (max_cards <= 1) {
        return cs_w
    }
    // Need: (max_cards - 1) * step + cs_w <= avail_width
    let step = (avail_width - cs_w) / float(max_cards - 1)
    return min(step, cs_w) * 0.93  // 7% more overlap
}

// Draw a player's hand as rows (one per suit), sorted by rank
// anchor_x, anchor_y: corner anchor point
// grow_right: cards extend right from anchor (true) or left (false)
// grow_down: suit rows extend down from anchor (true) or up (false)
def draw_hand_by_suit(player_idx : int; anchor_x, anchor_y : float; scale : float;
                      step_x : float; screen_w, screen_h : int;
                      grow_right, grow_down : bool) {
    sort_hand(game, player_idx)
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    let row_step = cs_h + float(hand_row_gap)
    let is_human = player_idx == 0
    // Group cards by suit
    var suit_starts : int[5]
    var num_suits = 0
    var prev_suit = -1
    for (c in range(get_hand_size(game, player_idx))) {
        let card = get_hand_card(game, player_idx, c)
        let s = card_suit_idx(card)
        if (s != prev_suit) {
            suit_starts[num_suits] = c
            num_suits++
            prev_suit = s
        }
    }
    suit_starts[num_suits] = get_hand_size(game, player_idx)
    // Draw each suit row
    for (row in range(num_suits)) {
        let row_start = suit_starts[row]
        let row_end = suit_starts[row + 1]
        for (c in range(row_start, row_end)) {
            let card = get_hand_card(game, player_idx, c)
            let ci = c - row_start
            let ry = grow_down ? float(row) : float(row - num_suits)
            let rx = grow_right ? float(ci) : -float(ci) - 1.0
            let card_x = anchor_x + rx * step_x + (grow_right ? 0.0 : (step_x - cs_w))
            let card_y = anchor_y + ry * row_step
            let card_idx = card_gpu_index(card)
            var tint = float4(1.0, 1.0, 1.0, 1.0)
            if (is_human && selected_card >= 0) {
                if (c == selected_card) {
                    tint = float4(1.4, 1.4, 1.0, 1.0)
                } else {
                    tint = float4(0.5, 0.5, 0.5, 1.0)
                }
            } elif (is_human && last_drawn_card != Card.card_none && card == last_drawn_card) {
                let t = 0.5 + 0.5 * sin(get_uptime() * float(last_drawn_pulse_speed))
                let rg = lerp(float(last_drawn_pulse_min), float(last_drawn_pulse_max), t)
                let b = lerp(float(last_drawn_pulse_min) * 1.2, float(last_drawn_pulse_max), t)
                tint = float4(rg, rg, b, 1.0)
            }
            if (card_idx >= 0) {
                draw_card(renderer, deck, card_idx, card_x, card_y, scale, screen_w, screen_h,
                    [tint = tint])
            }
        }
    }
    // Handle click on human hand cards
    if (is_human && game.phase == GamePhase.playing && game.current_player == 0 && mouse_clicked) {
        let mpos = glfwGetCursorPos(live_window)
        var clicked = -1
        for (row in range(num_suits)) {
            let row_start = suit_starts[row]
            let row_end = suit_starts[row + 1]
            for (c in range(row_start, row_end)) {
                let ci = c - row_start
                let ry = grow_down ? float(row) : float(row - num_suits)
                let rx = grow_right ? float(ci) : -float(ci) - 1.0
                let card_x = anchor_x + rx * step_x + (grow_right ? 0.0 : (step_x - cs_w))
                let card_y = anchor_y + ry * row_step
                if (mpos.x >= card_x && mpos.x <= card_x + cs_w && mpos.y >= card_y && mpos.y <= card_y + cs_h) {
                    clicked = c
                }
            }
        }
        if (clicked >= 0) {
            selected_card = (selected_card == clicked) ? -1 : clicked
        }
    }
}

// Draw bot hand as diagonal stack of face-down cards
def draw_hand_stack(player_idx : int; anchor_x, anchor_y : float; scale : float;
                    screen_w, screen_h : int; grow_right, grow_down : bool) {
    let n = get_hand_size(game, player_idx)
    if (n == 0 || back_index < 0) {
        return
    }
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    let ox = float(bot_stack_offset) * (grow_right ? 1.0 : -1.0)
    let oy = float(bot_stack_offset) * (grow_down ? 1.0 : -1.0)
    let color = player_chip_color(player_idx)
    let tint = chip_color_to_float4(color) * float4(1.0, 1.0, 1.0, 0.5) + float4(0.5, 0.5, 0.5, 0.5)
    for (i in range(n)) {
        let card_x = anchor_x + float(i) * ox + (grow_right ? 0.0 : -cs_w)
        let card_y = anchor_y + float(i) * oy + (grow_down ? 0.0 : -cs_h)
        draw_card(renderer, deck, back_index, card_x, card_y, scale, screen_w, screen_h,
            [tint = tint])
    }
}

// Draw all player hands in their respective corners
// Layout: P1=bottom-left, P2=top-left, P3=top-right, P4=bottom-right
// Board is centered; hands go in left/right margins at board scale
def draw_hands(screen_w, screen_h : int) {
    let fw = float(screen_w)
    let fh = float(screen_h)
    let scale = board_scale
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    let margin = float(hand_margin)
    // Available width for hands = margin from screen edge to board edge
    let left_avail = board_ox - margin * 2.0
    let right_avail = fw - (board_ox + float(BOARD_COLS) * cs_w + float(BOARD_COLS + 1) * board_pad_cur) - margin * 2.0
    // Compute step_x to fit 7 cards (max hand) in available width
    let left_step = compute_hand_step_x(cs_w, left_avail, MAX_HAND_SIZE)
    let right_step = compute_hand_step_x(cs_w, right_avail, MAX_HAND_SIZE)
    // Vertical midpoint for splitting top/bottom hands
    let mid_y = fh * 0.5
    // P1 (human): bottom-left — always show by suit, growing up from bottom
    draw_hand_by_suit(0, margin, fh - margin, scale, left_step, screen_w, screen_h, true, false)
    // Other players
    for (p in range(1, game.num_players)) {
        if (cheat_mode) {
            if (p == 1) {
                draw_hand_by_suit(1, margin, margin, scale, left_step, screen_w, screen_h, true, true)
            } elif (p == 2) {
                draw_hand_by_suit(2, fw - margin, margin, scale, right_step, screen_w, screen_h, false, true)
            } elif (p == 3) {
                draw_hand_by_suit(3, fw - margin, fh - margin, scale, right_step, screen_w, screen_h, false, false)
            }
        } else {
            if (p == 1) {
                draw_hand_stack(1, margin, margin, scale, screen_w, screen_h, true, true)
            } elif (p == 2) {
                draw_hand_stack(2, fw - margin, margin, scale, screen_w, screen_h, false, true)
            } elif (p == 3) {
                draw_hand_stack(3, fw - margin, fh - margin, scale, screen_w, screen_h, false, false)
            }
        }
    }
}

// --- Draw pile ---

// Top card of the draw pile screen position
def draw_pile_screen_pos() : float2 {
    let cs_h = CARD_DOC_HEIGHT * board_scale
    let board_right = board_ox + float(BOARD_COLS) * (board_cs_w + board_pad_cur) + board_pad_cur
    let margin = float(hand_margin)
    let pile_x = board_right + margin
    let pile_y = (float(screen_h) - cs_h) * 0.5 + cs_h * 0.5
    let n = min(length(game.draw_pile), int(draw_pile_max_cards))
    let off = float(draw_pile_offset)
    return float2(pile_x + float(n) * off, pile_y - float(n) * off)
}

def draw_draw_pile(screen_w, screen_h : int) {
    let n = length(game.draw_pile)
    if (n == 0 || back_index < 0) {
        return
    }
    let scale = board_scale
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    // Position: right of board, vertically centered
    let board_right = board_ox + float(BOARD_COLS) * (board_cs_w + board_pad_cur) + board_pad_cur
    let margin = float(hand_margin)
    let pile_x = board_right + margin
    let pile_y = (float(screen_h) - cs_h) * 0.5
    // Draw a small stack of face-down cards
    let visible = min(n, int(draw_pile_max_cards))
    let off = float(draw_pile_offset)
    for (i in range(visible)) {
        let x = pile_x + float(i) * off
        let y = pile_y - float(i) * off
        draw_card(renderer, deck, back_index, x, y, scale, screen_w, screen_h)
    }
}

// --- Discard pile ---

def draw_discard_pile(screen_w, screen_h : int) {
    let n = length(game.discard_pile)
    if (n == 0) {
        return
    }
    let scale = board_scale
    let cs_h = CARD_DOC_HEIGHT * scale
    // Position: below draw pile
    let pile_pos = draw_pile_screen_pos()
    let pile_x = pile_pos.x
    let pile_y = pile_pos.y + cs_h + float(hand_margin)
    // Just show top card face-up, with 1 dimmed card behind if pile has 2+
    if (n >= 2) {
        draw_card(renderer, deck, back_index, pile_x, pile_y, scale, screen_w, screen_h,
            [tint = float4(0.6, 0.6, 0.6, 1.0)])
    }
    let top_card = game.discard_pile[n - 1]
    let top_idx = card_gpu_index(top_card)
    if (top_idx >= 0) {
        let off = n >= 2 ? float(draw_pile_offset) : 0.0
        draw_card(renderer, deck, top_idx, pile_x + off, pile_y - off, scale, screen_w, screen_h)
    }
}

// --- UI text helper ---

def draw_ui_text(text : string; x, y : int; tint : float3 = float3(1.0)) {
    if (!ui_font_loaded) {
        return
    }
    var quads <- ui_font |> create_quads(text)
    ui_font |> draw_quads_2d(quads, screen_w, screen_h, x, y, tint)
    delete quads
}

// Draw text vertically, one char per line, going upward from (x, y)
// Draw text rotated 90° counter-clockwise, centered vertically at (x, y)
def draw_ui_text_vertical(text : string; x, y : int; tint : float3 = float3(1.0)) {
    if (!ui_font_loaded) {
        return
    }
    var quads <- ui_font |> create_quads(text)
    if (empty(quads)) {
        return
    }
    // Get text width to center it (width becomes height after rotation)
    let dim = quads_dim(quads)
    let text_width = dim.vmax.x - dim.vmin.x
    let projection = ortho_rh(0.0, float(screen_w), float(screen_h), 0.0, -1.0, 1.0)
    // Rotate 90° CCW, then translate. After rotation, text runs bottom-to-top.
    // Offset by half text width along Y to center vertically.
    var rot : float4x4
    rot[0] = float4(0.0, -1.0, 0.0, 0.0)
    rot[1] = float4(1.0, 0.0, 0.0, 0.0)
    rot[2] = float4(0.0, 0.0, 1.0, 0.0)
    rot[3] = float4(0.0, 0.0, 0.0, 1.0)
    let view = translation(float3(float(x), float(y) + text_width * 0.5, 0.0)) * rot
    let mvp = projection * view
    ui_font |> draw_quads(quads, mvp, tint)
    delete quads
}

def draw_ui_text_small(text : string; x, y : int; tint : float3 = float3(1.0)) {
    if (!ui_font_loaded) {
        return
    }
    var quads <- ui_font_small |> create_quads(text)
    ui_font_small |> draw_quads_2d(quads, screen_w, screen_h, x, y, tint)
    delete quads
}

// --- Turn indicator + HUD ---

def draw_hud(screen_w, screen_h : int) {
    if (game.phase == GamePhase.not_started) {
        return
    }
    // HUD on right side of board, above draw pile
    let board_right = board_ox + float(BOARD_COLS) * (board_cs_w + board_pad_cur) + board_pad_cur
    let margin = float(hand_margin)
    let hud_x = board_right + margin
    // Position above draw pile
    let pile_top_y = float(screen_h) * 0.5 - CARD_DOC_HEIGHT * board_scale * 0.5 - float(draw_pile_max_cards) * float(draw_pile_offset)
    let big_r = board_cs_w * float(chip_radius_frac) * 0.8
    let small_r = big_r * 0.6
    let row_h = big_r * 2.5
    let needed = sequences_to_win(game.num_players)
    let total_h = float(game.num_players) * row_h
    var cy = pile_top_y - total_h - margin
    // One row per player: chip (big if active) + sequence count
    let chip_cx = hud_x + big_r
    for (p in range(game.num_players)) {
        let color = player_chip_color(p)
        let cc = chip_color_to_float4(color)
        let is_active = game.current_player == p && game.phase == GamePhase.playing
        let r = is_active ? big_r : small_r
        let chip_cy = cy + big_r  // all centers aligned to big_r
        let outline = is_active ? float4(1.0, 1.0, 1.0, 0.9) : float4(0.0, 0.0, 0.0, 0.8)
        draw_chip(chip_renderer, chip_cx, chip_cy, r, cc, outline, screen_w, screen_h)
        let n = count_sequences(game, color)
        draw_ui_text("{n}/{needed}", int(chip_cx + big_r + 8.0), int(chip_cy + 8.0), float3(cc.x, cc.y, cc.z))
        cy += row_h
    }
    // Controls hint — left of board, vertically centered
    let hint_x = 4
    let hint_y = screen_h / 2
    draw_ui_text("N new", hint_x, hint_y, float3(0.5))
    draw_ui_text("C cheat", hint_x, hint_y + 26, float3(0.5))
    draw_ui_text("2/3/4", hint_x, hint_y + 52, float3(0.5))
    draw_ui_text("B bot", hint_x, hint_y + 78, float3(0.5))
    let bot_name = bot_mode == 0 ? "greedy" : "lookahead"
    draw_ui_text(bot_name, hint_x + 4, hint_y + 100, float3(0.8, 0.8, 0.3))
    // Jack hints — vertical text, left edge of board, vertically centered
    let board_mid_y = int(board_oy + float(BOARD_ROWS) * (board_cs_h + board_pad_cur) * 0.5)
    let jack_x = int(board_ox) - 2
    draw_ui_text_vertical("red jacks are wild  ~  black jacks remove", jack_x, board_mid_y, float3(0.4))
}

// --- Keyboard input ---

def game_seed() : int {
    return int(get_time_usec(0l))
}

def start_new_game(num_players : int) {
    make_game(game, num_players, game_seed())
    sort_hand(game, 0)
    selected_card = -1
    last_drawn_card = Card.card_none
    bot_timer = float(bot_delay)
    anim_queue |> clear()
}

var key_n_was_pressed : bool = false
var key_c_was_pressed : bool = false
var key_2_was_pressed : bool = false
var key_3_was_pressed : bool = false
var key_4_was_pressed : bool = false
var key_b_was_pressed : bool = false

def key_edge(key : int; var was_pressed : bool&) : bool {
    let pressed = glfwGetKey(live_window, key) == int(GLFW_PRESS)
    let edge = pressed && !was_pressed
    was_pressed = pressed
    return edge
}

def handle_keyboard() {
    if (key_edge(int(GLFW_KEY_N), key_n_was_pressed)) {
        start_new_game(game.num_players)
    }
    if (key_edge(int(GLFW_KEY_C), key_c_was_pressed)) {
        cheat_mode = !cheat_mode
    }
    if (key_edge(int(GLFW_KEY_2), key_2_was_pressed)) {
        start_new_game(2)
    }
    if (key_edge(int(GLFW_KEY_3), key_3_was_pressed)) {
        start_new_game(3)
    }
    if (key_edge(int(GLFW_KEY_4), key_4_was_pressed)) {
        start_new_game(4)
    }
    if (key_edge(int(GLFW_KEY_B), key_b_was_pressed)) {
        bot_mode = (bot_mode + 1) % 2
    }
}

// --- Game over overlay ---

def draw_game_over(screen_w, screen_h : int) {
    // Draw a large colored chip in the center to indicate the winner
    let cx = float(screen_w) * 0.5
    let cy = float(screen_h) * 0.5
    let r = min(float(screen_w), float(screen_h)) * 0.08
    let winner_color = player_chip_color(game.winner)
    let color = chip_color_to_float4(winner_color)
    // Pulsing winner chip
    let pulse = 0.7 + 0.3 * sin(get_uptime() * 2.0)
    let bright = color * float4(pulse, pulse, pulse, 1.0)
    draw_chip(chip_renderer, cx, cy, r * 1.5, bright, float4(1.0, 1.0, 1.0, 1.0), screen_w, screen_h)
    // Inner ring
    draw_chip(chip_renderer, cx, cy, r * 0.8, color, float4(0.0, 0.0, 0.0, 1.0), screen_w, screen_h)
    // Winner text
    let winner_name = game.winner == 0 ? "You win!" : "P{game.winner + 1} wins!"
    draw_ui_text(winner_name, int(cx) - 40, int(cy) + int(r * 1.8), float3(1.0, 1.0, 0.8))
    draw_ui_text("Press N for new game", int(cx) - 90, int(cy) + int(r * 1.8) + 24, float3(0.6, 0.6, 0.6))
}

// --- Lifecycle ---

[export]
def init() {
    live_create_window("Sequence", 1280, 800)
    renderer = create_card_renderer()
    delete_chip_renderer(chip_renderer)
    chip_renderer = create_chip_renderer()
    deck <- load_gpu_deck()
    back_index = find_card(deck, "back")
    compute_card_gpu_lut()
    compute_board_card_indices()
    if (!ui_font_loaded) {
        create_ttf_objects()
        let font_dir = dir_name(get_module_file_name("card_mesh"))
        ui_font <- load_ttf("{font_dir}/DejaVuSerif.ttf", [pixel_height = 24.0, oversampling = 2])
        if (empty(ui_font.bitmap.bytes)) {
            ui_font <- load_ttf("{get_das_root()}/modules/dasOpenGL/examples/droidsansmono.ttf",
                [pixel_height = 24.0, oversampling = 2])
        }
        if (!empty(ui_font.bitmap.bytes)) {
            upload_font_texture(ui_font)
            // Small font for hints
            let font_dir2 = dir_name(get_module_file_name("card_mesh"))
            ui_font_small <- load_ttf("{font_dir2}/DejaVuSerif.ttf", [pixel_height = 14.0, oversampling = 2])
            if (empty(ui_font_small.bitmap.bytes)) {
                ui_font_small <- load_ttf("{get_das_root()}/modules/dasOpenGL/examples/droidsansmono.ttf",
                    [pixel_height = 14.0, oversampling = 2])
            }
            if (!empty(ui_font_small.bitmap.bytes)) {
                upload_font_texture(ui_font_small)
            }
            ui_font_loaded = true
        }
    }
    if (game.phase == GamePhase.not_started) {
        make_game(game, 2, game_seed())
        sort_hand(game, 0)
        bot_timer = float(bot_delay)
    }
    init_audio()
    print("sequence: init (is_reload={is_reload()}, cards={card_count(deck)})\n")
}

[export]
def update() {
    if (!live_begin_frame()) {
        return
    }
    live_get_framebuffer_size(screen_w, screen_h)
    var w = screen_w
    var h = screen_h
    glViewport(0, 0, w, h)
    glClearColor(bg_color.x, bg_color.y, bg_color.z, 1.0)
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)
    gl_stats_reset()
    card_stats_reset()
    glDisable(GL_DEPTH_TEST)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    let dt = get_dt()
    if (deck.loaded) {
        handle_keyboard()
        handle_mouse()
        // Tick animations
        tick_animations(dt)
        // Bot auto-play
        if (game.phase == GamePhase.playing && game.current_player > 0 && !is_animating()) {
            bot_timer -= dt
            if (bot_timer <= 0.0) {
                let bot_player = game.current_player
                queue_discard_anims(bot_player)
                bot_rng_seed++
                let m = bot_mode == 0 ? bot_greedy_move(game, bot_player, bot_rng_seed) : bot_lookahead_move(game, bot_player, bot_rng_seed)
                if (m.card_idx >= 0) {
                    let card = get_hand_card(game, bot_player, m.card_idx)
                    let is_bj = is_black_jack(card)
                    let is_rj = is_red_jack(card)
                    queue_animated_move(bot_player, m, player_chip_color(bot_player),
                        is_bj, is_rj, m.row, m.col)
                }
                bot_timer = float(bot_delay)
            }
        }
        draw_board(w, h)
        if (game.phase == GamePhase.playing || game.phase == GamePhase.game_over) {
            draw_hands(w, h)
            draw_draw_pile(w, h)
            draw_discard_pile(w, h)
            draw_hud(w, h)
        }
        // Draw flying card on top of everything
        if (length(anim_queue) > 0) {
            let a = anim_queue[0]
            if (a.atype == AnimType.card_fly && a.card_gpu_idx >= 0) {
                let t = clamp(a.progress, 0.0, 1.0)
                let et = 1.0 - (1.0 - t) * (1.0 - t)  // ease-out
                let target = cell_screen_pos(a.row, a.col)
                let x = lerp(a.start_x, target.x, et)
                let y = lerp(a.start_y, target.y, et)
                draw_card(renderer, deck, a.card_gpu_idx, x, y, a.start_scale, w, h)
            } elif (a.atype == AnimType.card_discard && a.card_gpu_idx >= 0) {
                let t = clamp(a.progress, 0.0, 1.0)
                let et = 1.0 - (1.0 - t) * (1.0 - t)  // ease-out
                let target_pos = discard_pile_screen_pos()
                let x = lerp(a.start_x, target_pos.x, et)
                let y = lerp(a.start_y, target_pos.y, et)
                draw_card(renderer, deck, a.card_gpu_idx, x, y, a.start_scale, w, h)
            } elif (a.atype == AnimType.card_draw && back_index >= 0) {
                let t = clamp(a.progress, 0.0, 1.0)
                let et = 1.0 - (1.0 - t) * (1.0 - t)  // ease-out
                var target_pos : float2
                if (a.deferred_player == 0) {
                    target_pos = float2(float(hand_margin), float(h) - float(hand_margin) - CARD_DOC_HEIGHT * board_scale)
                } else {
                    target_pos = bot_card_screen_pos(a.deferred_player)
                }
                let x = lerp(a.start_x, target_pos.x, et)
                let y = lerp(a.start_y, target_pos.y, et)
                draw_card(renderer, deck, back_index, x, y, board_scale, w, h)
            }
        }
        if (game.phase == GamePhase.game_over) {
            draw_game_over(w, h)
        }
    }
    // Fold card renderer stats into gl_render_stats
    gl_render_stats.draw_calls += card_stats_get_draw_calls()
    gl_render_stats.triangles += card_stats_get_triangles()
    glDisable(GL_BLEND)
    live_end_frame()
}

def draw_board(screen_w, screen_h : int) {
    let fw = float(screen_w)
    let fh = float(screen_h)
    // Scale to fit 10x10 grid of upright cards with padding
    let pad = board_pad
    let total_pad_x = pad * float(BOARD_COLS + 1)
    let total_pad_y = pad * float(BOARD_ROWS + 1)
    let scale = min((fw - total_pad_x) / (float(BOARD_COLS) * CARD_DOC_WIDTH),
                    (fh - total_pad_y) / (float(BOARD_ROWS) * CARD_DOC_HEIGHT))
    let cs_w = CARD_DOC_WIDTH * scale
    let cs_h = CARD_DOC_HEIGHT * scale
    // Store layout for hit-testing
    board_ox = (fw - float(BOARD_COLS) * cs_w - total_pad_x) * 0.5
    board_oy = (fh - float(BOARD_ROWS) * cs_h - total_pad_y) * 0.5
    board_scale = scale
    board_cs_w = cs_w
    board_cs_h = cs_h
    board_pad_cur = pad
    // Pass 1: draw all cards
    for (row in range(BOARD_ROWS)) {
        for (col in range(BOARD_COLS)) {
            let idx = row * BOARD_COLS + col
            let card_x = board_ox + pad + float(col) * (cs_w + pad)
            let card_y = board_oy + pad + float(row) * (cs_h + pad)
            var tint = float4(1.0, 1.0, 1.0, 1.0)
            let chip = get_chip(game, row, col)
            if (chip != ChipColor.none) {
                let cc = chip_color_to_float4(chip)
                if (is_in_sequence(game, row, col)) {
                    // Stronger tint for sequence cards
                    let st = float(seq_card_tint)
                    tint = float4(lerp(1.0, cc.x, st), lerp(1.0, cc.y, st), lerp(1.0, cc.z, st), 1.0)
                } else {
                    tint = float4(lerp(1.0, cc.x, 0.15), lerp(1.0, cc.y, 0.15), lerp(1.0, cc.z, 0.15), 1.0)
                }
            }
            if (row == hover_row && col == hover_col && !is_corner(row, col)) {
                if (selected_card >= 0 && game.phase == GamePhase.playing && game.current_player == 0) {
                    let m = Move(card_idx = selected_card, row = row, col = col)
                    if (is_legal_move(game, 0, m)) {
                        tint *= float4(0.6, 1.4, 0.6, 1.0)  // green = legal
                    } else {
                        tint *= float4(1.4, 0.5, 0.5, 1.0)  // red = illegal
                    }
                } else {
                    tint *= hover_tint
                }
            }
            let card_idx = board_card_indices[idx]
            if (card_idx >= 0) {
                draw_card(renderer, deck, card_idx, card_x, card_y, scale, screen_w, screen_h,
                    [tint = tint])
            } elif (back_index >= 0) {
                draw_card(renderer, deck, back_index, card_x, card_y, scale, screen_w, screen_h,
                    [tint = tint * float4(1.0, 0.85, 0.4, 1.0)])
            }
        }
    }
    // Pass 2: draw chips on top (sequence chips pulse, animated chips bounce/fade)
    let chip_r = cs_w * float(chip_radius_frac)
    let pulse = float(seq_pulse_min) + (1.0 - float(seq_pulse_min)) * (0.5 + 0.5 * sin(get_uptime() * float(seq_pulse_speed)))
    for (row in range(BOARD_ROWS)) {
        for (col in range(BOARD_COLS)) {
            let chip = get_chip(game, row, col)
            // Check for active animation on this cell
            let anim_s = get_chip_anim_scale(row, col)
            let anim_a = get_chip_anim_alpha(row, col)
            if (anim_s >= 0.0 && length(anim_queue) > 0 && anim_queue[0].atype == AnimType.chip_place) {
                // Animated chip placement — draw the incoming chip with bounce scale
                let cx = board_ox + pad + float(col) * (cs_w + pad) + cs_w * 0.5
                let cy = board_oy + pad + float(row) * (cs_h + pad) + cs_h * 0.5
                let anim_color = chip_color_to_float4(anim_queue[0].color)
                let r = chip_r * anim_s
                draw_chip(chip_renderer, cx, cy, r, anim_color, float4(0.0, 0.0, 0.0, 0.9), screen_w, screen_h)
            } elif (anim_s >= 0.0 && length(anim_queue) > 0 && anim_queue[0].atype == AnimType.chip_remove) {
                // Animated chip removal — draw shrinking/fading chip
                let cx = board_ox + pad + float(col) * (cs_w + pad) + cs_w * 0.5
                let cy = board_oy + pad + float(row) * (cs_h + pad) + cs_h * 0.5
                let anim_color = chip_color_to_float4(anim_queue[0].color)
                let alpha = anim_a >= 0.0 ? anim_a : 1.0
                let r = chip_r * anim_s
                let c = float4(anim_color.x, anim_color.y, anim_color.z, alpha)
                draw_chip(chip_renderer, cx, cy, r, c, float4(1.0, 1.0, 1.0, alpha), screen_w, screen_h)
            } elif (chip != ChipColor.none) {
                let cx = board_ox + pad + float(col) * (cs_w + pad) + cs_w * 0.5
                let cy = board_oy + pad + float(row) * (cs_h + pad) + cs_h * 0.5
                var color = chip_color_to_float4(chip)
                var outline = float4(0.0, 0.0, 0.0, 0.9)
                var r = chip_r
                if (is_in_sequence(game, row, col)) {
                    let boost = 0.15 * pulse
                    color = float4(min(color.x + boost, 1.0), min(color.y + boost, 1.0), min(color.z + boost, 1.0), 1.0)
                    outline = float4(1.0, 1.0, 1.0, 0.6 + 0.4 * pulse)
                    r *= 1.0 + 0.1 * pulse
                }
                draw_chip(chip_renderer, cx, cy, r, color, outline, screen_w, screen_h)
            }
        }
    }
}

// --- Live commands ---

struct CmdPlaceChipArgs {
    row : int
    col : int
    color : int = 1  // 1=blue, 2=green, 3=red, 4=yellow, 0=none
}

[live_command(description="Place chip. Args: row, col, color (0=none,1=blue,2=green,3=red,4=yellow)")]
def cmd_place_chip(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdPlaceChipArgs>)
    place_chip(game, args.row, args.col, unsafe(reinterpret<ChipColor>(args.color)))
    return JV(args)
}

[live_command(description="Clear all chips from the board")]
def cmd_clear_board(input : JsonValue?) : JsonValue? {
    clear_all_chips(game)
    return JV("cleared")
}

struct CmdNewGameArgs {
    num_players : int = 2
    seed : int = -1  // -1 = use uptime as seed
}

[live_command(description="Start a new game. Args: num_players (2-4), seed (-1=random)")]
def cmd_new_game(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdNewGameArgs>)
    let seed = args.seed >= 0 ? args.seed : int(get_uptime() * 1000.0)
    make_game(game, args.num_players, seed)
    sort_hand(game, 0)
    selected_card = -1
    bot_timer = float(bot_delay)
    return JV("{game.num_players} players, seed={seed}")
}

struct CmdGameStatusResult {
    phase : string
    num_players : int
    current_player : int
    draw_pile_size : int
    cheat_mode : bool
}

[live_command(description="Get current game status")]
def cmd_game_status(input : JsonValue?) : JsonValue? {
    var phase_str = "not_started"
    if (game.phase == GamePhase.playing) {
        phase_str = "playing"
    } elif (game.phase == GamePhase.game_over) {
        phase_str = "game_over"
    }
    return JV(CmdGameStatusResult(
        phase = phase_str,
        num_players = game.num_players,
        current_player = game.current_player,
        draw_pile_size = length(game.draw_pile),
        cheat_mode = cheat_mode
    ))
}

[live_command(description="Debug hand layout info")]
def cmd_debug_hands(input : JsonValue?) : JsonValue? {
    var info = "board_scale={board_scale} board_ox={board_ox} board_cs_w={board_cs_w} board_cs_h={board_cs_h}\n"
    for (p in range(game.num_players)) {
        info += "P{p}: {get_hand_size(game, p)} cards: "
        for (c in range(get_hand_size(game, p))) {
            info += card_to_string(get_hand_card(game, p, c))
            if (c < get_hand_size(game, p) - 1) {
                info += ", "
            }
        }
        info += "\n"
    }
    return JV(info)
}

[live_command(description="Find and display all completed sequences")]
def cmd_check_sequences(input : JsonValue?) : JsonValue? {
    find_sequences(game)
    var info = "{length(game.sequences)} sequences found\n"
    for (seq in game.sequences) {
        var color_name = "unknown"
        if (seq.color == ChipColor.blue) { color_name = "blue"; }
        elif (seq.color == ChipColor.green) { color_name = "green"; }
        elif (seq.color == ChipColor.red) { color_name = "red"; }
        elif (seq.color == ChipColor.yellow) { color_name = "yellow"; }
        info += "  {color_name}: "
        for (i in range(5)) {
            if (i > 0) { info += ", "; }
            info += "({seq.cells[i].y},{seq.cells[i].x})"
        }
        info += "\n"
    }
    let w = check_win(game)
    if (w >= 0) {
        info += "WINNER: player {w}\n"
    }
    return JV(info)
}

[live_command(description="Toggle cheat mode (show all hands)")]
def cmd_toggle_cheat(input : JsonValue?) : JsonValue? {
    cheat_mode = !cheat_mode
    return JV(cheat_mode)
}

struct CmdSetHandArgs {
    player : int = 0
    cards : array<string>
}

[live_command(description="Set a player's hand. Args: player (0-3), cards (array of card names)")]
def cmd_set_hand(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdSetHandArgs>)
    if (game.phase == GamePhase.not_started) {
        game.phase = GamePhase.playing
    }
    if (args.player >= game.num_players) {
        game.num_players = args.player + 1
    }
    var card_enums : array<Card>
    for (name in args.cards) {
        card_enums |> push(string_to_card(name))
    }
    set_hand(game, args.player, card_enums)
    sort_hand(game, args.player)
    selected_card = -1
    return JV("{length(args.cards)} cards set for player {args.player}")
}

struct CmdForceMoveArgs {
    player : int = -1   // -1 = current player
    card_idx : int = 0
    row : int
    col : int
}

[live_command(description="Force a move through apply_move (triggers sequences/win). Args: player, card_idx, row, col")]
def cmd_force_move(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdForceMoveArgs>)
    let p = args.player >= 0 ? args.player : game.current_player
    let m = Move(card_idx = args.card_idx, row = args.row, col = args.col)
    let card_name = card_to_string(get_hand_card(game, p, args.card_idx))
    if (apply_move(game, p, m)) {
        return JV("P{p} played {card_name} at ({args.row},{args.col}), phase={game.phase}, winner={game.winner}")
    }
    return JV("move failed: P{p} card_idx={args.card_idx} at ({args.row},{args.col})")
}

struct CmdSetPhaseArgs {
    phase : string = "playing"  // "not_started", "playing", "game_over"
    winner : int = -1
}

[live_command(description="Set game phase. Args: phase (not_started/playing/game_over), winner (-1=none)")]
def cmd_set_phase(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdSetPhaseArgs>)
    if (args.phase == "not_started") {
        game.phase = GamePhase.not_started
    } elif (args.phase == "playing") {
        game.phase = GamePhase.playing
    } elif (args.phase == "game_over") {
        game.phase = GamePhase.game_over
    }
    game.winner = args.winner
    selected_card = -1
    return JV("phase={args.phase}, winner={game.winner}")
}

struct CmdSetPlayerArgs {
    player : int = 0
}

[live_command(description="Set current player. Args: player (0-3)")]
def cmd_set_player(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdSetPlayerArgs>)
    game.current_player = clamp(args.player, 0, game.num_players - 1)
    selected_card = -1
    return JV("current_player={game.current_player}")
}

[live_command(description="Trigger one bot move for current player (or specified player)")]
def cmd_bot_move(input : JsonValue?) : JsonValue? {
    var args = from_JV(input, type<CmdSetPlayerArgs>)  // reuse: just needs player
    let p = args.player >= 0 ? args.player : game.current_player
    if (p == 0) {
        return JV("player 0 is human, use board click")
    }
    if (game.phase != GamePhase.playing) {
        return JV("game not in playing phase")
    }
    queue_discard_anims(p)
    bot_rng_seed++
    let m = bot_mode == 0 ? bot_greedy_move(game, p, bot_rng_seed) : bot_lookahead_move(game, p, bot_rng_seed)
    if (m.card_idx >= 0) {
        let card_name = card_to_string(get_hand_card(game, p, m.card_idx))
        if (apply_move(game, p, m)) {
            bot_timer = float(bot_delay)
            return JV("P{p} played {card_name} at ({m.row},{m.col}), next=P{game.current_player}")
        }
    }
    return JV("P{p} has no legal moves")
}

[live_command(description="Reset board: clear chips, sequences, set phase to playing")]
def cmd_reset_board(input : JsonValue?) : JsonValue? {
    clear_all_chips(game)
    game.sequences |> clear()
    game.phase = GamePhase.playing
    game.winner = -1
    selected_card = -1
    return JV("board reset, phase=playing")
}

struct CardDrawOpStats {
    total_cards : int
    total_draw_ops : int
    total_stencil_groups : int
    non_stencil_batches : int
    stencil_draw_calls : int
    non_stencil_draw_calls : int
    avg_draw_calls_per_card : float
    last_frame_card_draws : int
    last_frame_draw_calls : int
    last_frame_triangles : int
}

[live_command(description="Analyze card draw op structure")]
def cmd_card_stats(input : JsonValue?) : JsonValue? {
    var stats = CardDrawOpStats()
    stats.total_cards = card_count(deck)
    for (i in range(card_count(deck))) {
        let gpu & = unsafe(deck.cards[i])
        stats.total_draw_ops += length(gpu.draw_ops)
        stats.total_stencil_groups += length(gpu.stencil_groups)
        for (op in gpu.draw_ops) {
            if (op.stencil_idx >= 0) {
                stats.stencil_draw_calls += 2
            } else {
                stats.non_stencil_batches++
                stats.non_stencil_draw_calls++
            }
        }
    }
    stats.avg_draw_calls_per_card = float(stats.stencil_draw_calls + stats.non_stencil_draw_calls) / float(stats.total_cards)
    stats.last_frame_card_draws = card_stats_get_draws()
    stats.last_frame_draw_calls = card_stats_get_draw_calls()
    stats.last_frame_triangles = card_stats_get_triangles()
    return JV(stats)
}

// --- Lifecycle ---

[live_command(description="Check font loading status")]
def cmd_font_status(input : JsonValue?) : JsonValue? {
    let font_dir = dir_name(get_module_file_name("card_mesh"))
    return JV("loaded={ui_font_loaded}, font_dir='{font_dir}', bitmap_bytes={length(ui_font.bitmap.bytes)}")
}

[export]
def shutdown() {
    print("sequence: shutdown\n")
    delete_chip_renderer(chip_renderer)
    live_destroy_window()
}

// Dual-mode: also works with regular daslang.exe
[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}

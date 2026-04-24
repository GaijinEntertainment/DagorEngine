# Sequence Board Game — Live Coding Project Plan

## Context

Build the Sequence card/board game as a live-coded daslang application in `examples/daslive/sequence/`. The primary goal is **exercising and stress-testing live coding** — the game is the vehicle, not the destination. If live-reload bugs surface, we stop and fix those first.

The existing `main.das` (68 lines) already has a working card viewer with dasCards integration. We'll grow it incrementally, keeping the app live-reloadable at every phase.

## File Structure

```
examples/daslive/sequence/
  CLAUDE.md              -- project-specific instructions (Phase 0)
  main.das               -- rendering, window, input, animations, sound
  gameplay.das           -- board model, rules, state machine, win detection (module)
  bot_random.das         -- random legal move bot
  bot_greedy.das         -- position-evaluation bot
  bot_heuristic.das      -- configurable heuristic weights bot
  bot_mcts.das           -- Monte Carlo tree search bot (or similar)
  elo.das                -- ELO rating + tournament runner (module)
  test_gameplay.das      -- tests for gameplay logic
  test_bots.das          -- tests for bot move validity
  test_elo.das           -- tests for ELO calculations
```

## Key Design Decisions

- **Board layout**: hardcoded 10x10 array in `gameplay.das` matching the original Sequence board card order (from the reference image)
- **Jack rules**: black jacks (spade, club) = remove opponent chip; red jacks (heart, diamond) = wild placement
- **Player modes**: always 1 human + 1/2/3 bots (2, 3, or 4 player games). No networking.
- **Separation**: `gameplay.das` is pure logic (no OpenGL, no GLFW); `main.das` owns all rendering + state + bot dispatch
- **Bot interface**: each bot exports `def bot_XXX_move(game : GameState; player_idx : int) : Move` — function pointers in main.das, no classes
- **State persistence**: `GameState` via `@live` vars; GPU resources guarded by `is_reload()`
- **Headless mode**: bot-vs-bot tournaments run without rendering (tight loop in `elo.das`)
- **Sound**: procedural generation (sine/noise synthesis), tunable via `@live` vars
- **Testing**: every phase adds tests for new functionality; `test_gameplay.das` grows with the rules engine

## Phases

### Phase 0: CLAUDE.md + Plan
- Write `examples/daslive/sequence/CLAUDE.md` with project-specific instructions:
  - Live coding is the priority; game is secondary
  - File structure and module boundaries
  - Jack color rules
  - Board layout source (original Sequence board)
  - Player modes (1 human + N bots)
  - Testing expectations
  - Current phase status
- Copy this plan to `examples/daslive/sequence/PLAN.md` for reference
- **Each subsequent phase**: update `CLAUDE.md` with current status, completed phases, and any new knowledge discovered during implementation

### Phase 1: Board Layout + Static Rendering
- Create `gameplay.das` module with `BOARD_LAYOUT` (10x10 `string` fixed array from the reference image), `FREE` sentinel
- Define `ChipColor` enum (`none`, `blue`, `green`, `red`, `yellow`)
- In `main.das`: replace single-card viewer with 10x10 grid. Auto-scale cards to fit window. FREE corners drawn as card backs or colored rectangles
- `@live` vars: `board_scale`, `board_pad`, `bg_color`
- **Tests**: board layout has exactly 96 non-FREE cells (48 unique cards x2), all card names valid via `is_playing_card()`, no jacks on board, 4 FREE corners
- **Result**: the Sequence board on screen, all cards face-up

### Phase 2: Chips + Hover + Click
- Draw chips as colored circles (triangle-fan mesh, created once)
- Mouse hit-testing: highlight hovered cell via `draw_card` tint
- Click places/cycles chip color (debug mode — no game rules yet)
- `[live_command]` `cmd_place_chip(row, col, color)`, `cmd_clear_board`
- `@live` vars: chip size, chip colors, hover tint
- **Tests**: hit-testing logic (screen coords to grid cell), chip state round-trip
- **Result**: interactive board with visual chip placement

### Phase 3: Game State + Turn Structure
- `gameplay.das`: `GameState` struct (board chips, hands, draw pile, discard, current player, phase enum)
- `make_game(num_players)` — shuffle deck, deal hands (7 for 2p, 6 for 3p, 5 for 4p)
- `legal_moves()`, `apply_move()`, dead card detection
- Jack logic included from the start (black=remove, red=wild)
- `main.das`: display human's hand at bottom, card selection, play by clicking board, turn indicator
- `@live game_state` for reload persistence
- `[live_command]` `cmd_new_game(num_players)`, `cmd_game_status`
- Cheat mode toggle: show all players' hands
- **Tests**: `make_game` deals correct hand sizes, deck has 104 cards (2 standard decks), `legal_moves` returns valid positions for each card, `apply_move` advances turn and draws card, dead card detection works, jack moves generate correctly (black=remove targets, red=wild targets)
- **Result**: playable game, human controls all sides (hot-seat debug)

### Phase 4: Win Detection + Sequence Highlighting
- `gameplay.das`: `find_sequences()` — scan all 4 directions, FREE corners wild for everyone, sequences can share one cell
- `check_win()` with player-count-aware condition (2 sequences for 2p, 1 for 3-4p)
- `main.das`: highlight completed sequences (pulsing tint or colored line through 5 chips), game over screen with winner
- `[live_command]` `cmd_check_sequences`
- **Tests**: horizontal/vertical/diagonal sequence detection, FREE corner inclusion, overlapping sequences count separately, win condition per player count, black jack can't remove from completed sequence
- **Result**: complete rules engine, game can end

### Phase 5: First Bot (Random)
- `bot_random.das`: pick uniformly random legal move
- `main.das`: function pointer table for bots. Player 0 = human, remaining = bots. Bot auto-plays after `@live bot_delay` (0.5s default)
- `[live_command]` `cmd_bot_move`, `cmd_set_bot(player_idx, type)`
- Support 2/3/4 player games: `@live num_players`
- **Tests**: random bot always returns a legal move, bot handles jack cards correctly, bot handles dead cards (discards them), full game simulation runs to completion without errors
- **Result**: human vs 1-3 random bots

### Phase 6: Animations
- Card play: animate from hand to board position (lerp position + scale, ~0.3s)
- Chip placement: scale-up bounce
- Card draw: slide from deck to hand
- Jack removal: flash/fade on removed chip
- Animation queue with lerp progress, `@live` durations
- Bot moves trigger same animations (move applies after animation completes)
- **Tests**: animation lerp math (0.0 = start, 1.0 = end, clamping), animation queue ordering
- **Result**: fluid, polished feel

### Phase 7: Sound Effects
- Procedural audio generation (sine sweeps, noise bursts — arcanoid pattern)
- Events: card play (click/snap), chip place (thud), card draw (swish), jack remove (descending tone), sequence complete (ascending arpeggio), game win (fanfare), turn chime
- `@live` volume, `[live_command]` `cmd_play_sound(name)`
- **Tests**: PCM buffer generation produces non-zero samples, correct sample rates/lengths
- **Result**: audio feedback on all game actions

### Phase 8: Greedy Bot
- `bot_greedy.das`: score each legal move by:
  - +high for completing a sequence
  - +N for extending partial sequences (count consecutive in each direction)
  - +bonus for blocking opponent near-completion
  - +adjacency bonus near own chips
  - For black jacks: prioritize removing from opponent's longest partial
  - Dead card awareness
- **Tests**: scoring function returns expected values for known board states, greedy bot picks sequence-completing move when available, greedy bot blocks opponent 4-in-a-row
- **Result**: noticeably stronger opponent

### Phase 9: ELO Rating + Tournaments
- `elo.das` module: `EloRating` struct, `update_elo()`, `run_tournament(bot_funcs, num_games)`, `play_game_headless()` (no rendering, tight loop)
- `[live_command]` `cmd_tournament(games, bots, num_players)`, `cmd_elo_status`
- `@live` ELO ratings for persistence across reloads
- Console output: "After 1000 games: greedy=1520, random=1480"
- **Tests**: ELO calculation math (expected score, K-factor update), headless game runs to completion, greedy beats random statistically (>60% win rate over 100+ games)
- **Result**: data-driven bot development — modify bot, reload, tournament, check ELO delta

### Phase 10: Heuristic Bot + Weight Tuning
- `bot_heuristic.das`: same evaluation categories as greedy, but with **configurable weight struct**
- Weight parameters: sequence_completion, partial_extension, blocking, adjacency, jack_offense, jack_defense, dead_card_penalty, center_bonus, corner_proximity, etc.
- Multiple preset weight configs (aggressive, defensive, balanced)
- `[live_command]` `cmd_set_weights(preset_or_json)` to swap configs at runtime
- `@live` weight values for real-time tuning during play
- Run tournaments between different weight configurations to find strongest set
- **Tests**: different weight configs produce different move choices on same board state, all presets produce legal moves, tournament between presets converges (no ties after enough games)
- **Result**: experimentally-tuned heuristic bot, ELO-validated weight selection

### Phase 11: Strategic Bot (MCTS) + Polish
- `bot_mcts.das`: Monte Carlo tree search or minimax with limited depth, opponent modeling (use greedy heuristic as proxy for opponent play)
- Run ELO tournament to validate it outperforms heuristic bot
- Polish: keyboard shortcuts (N=new game, C=cheat), turn history, window resize handling
- **Tests**: MCTS bot returns legal moves under time budget, MCTS beats greedy statistically in tournament
- **Result**: strongest AI, polished game

## Phase Workflow

**We stop between every phase and review together.** Do NOT proceed to the next phase without explicit user approval. Each phase boundary is a checkpoint:

1. Complete the phase implementation
2. Run automated tests
3. Take screenshots of the running app and review visually
4. Manual play-testing — exercise the new functionality, screenshot results
5. Stop and present results to the user
6. Wait for user review and approval before starting the next phase

If live-reload breaks at any point, that becomes the immediate priority — fix it before continuing the current phase.

## Verification (per phase)

1. **Live reload**: save file, confirm hot-reload works, `@live` vars tunable, state persists
2. **Screenshots**: take a screenshot of the running app and review it visually — check board layout, chip rendering, hand display, animations, UI elements. This is the primary way to catch rendering bugs.
3. **Tests**: run `test_gameplay.das` (and other test files) via `run_test` MCP tool or dastest
4. **MCP tools**: `compile_check` for syntax, `live_status` / `live_reload` for live testing
5. **Game logic**: `[live_command]` endpoints to inspect/manipulate state
6. **Manual play-testing**: play through at least one sequence of actions (place chips, play cards, trigger jacks, etc.) and screenshot the result
7. **Tournaments** (Phase 9+): ELO convergence confirms bots work correctly

## Critical Files

- `examples/daslive/sequence/main.das` — main app (modify)
- `examples/daslive/sequence/gameplay.das` — new, pure game logic module
- `examples/daslive/sequence/bot_*.das` — new, one per AI
- `examples/daslive/sequence/elo.das` — new, tournament system
- `examples/daslive/sequence/test_*.das` — tests, grow each phase
- `e:\dasCards\cards\opengl_cards.das` — card rendering API (read-only reference)
- `skills/daslang_live.md` — live coding patterns (read before each phase)
- `skills/das_formatting.md` — formatting rules (read before writing .das)

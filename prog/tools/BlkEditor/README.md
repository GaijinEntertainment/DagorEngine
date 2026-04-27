# BlkEditor

BlkEditor is a lightweight Qt-based inspector for `.blk` configuration trees. It can visualize the block hierarchy, inspect parameters, and edit supported value types inline.

## Text Editor

A **BLK Text** pane mirrors the raw file contents. It loads the original text (falling back to an auto-generated serialization for binary inputs) and lets you edit the document directly with a monospaced editor.

- **Apply Text Changes** (`Ctrl+Enter or Ctrl+S`) parses the editor buffer and rebuilds the tree/variable catalog from that text. Parse errors are surfaced inline so you can fix them before continuing.
- **Reload Text From Document** regenerates the textual view from the current tree, discarding any pending edits. Use this when you have been editing via the table/tree and want a fresh textual snapshot.
- Table edits are blocked while the text buffer has unapplied edits, preventing accidental loss of work—simply apply or discard the text changes when prompted.
- Saving always reuses the latest applied text, so there is no risk of writing stale data; the editor has to parse cleanly before the file hits disk.
- The editor has BLK-aware syntax highlighting (directives, types, includes, comments) and a context-sensitive completer. Completions merge parameter names (`foo` and `#foo`), directives (`@include`, `@override`, …), available BLK types, and discovered include targets so you can stitch together new snippets without leaving the keyboard.

## Variables

A dedicated **Variables** widget is on the right side of the window. It aggregates every unique parameter name across the loaded BLK document (and respective blk files found in the project/engine) and shows:

- The parameter name and inferred type (or **Mixed** if multiple types exist)
- The number of blocks that use the parameter (hover to see the matching block paths)
- Highlighting for the currently selected block, making it easy to identify variables that already exist in that scope
- Text filtering to quickly locate a specific name

All detected `.blk` files under the current document's directory are scanned (up to a safe limit) so the grid reflects *every* variable you are likely to encounter in the project, not just the ones present in the active file. The same superset feeds into autocomplete everywhere else.

You can double-click (or press Enter on) any variable entry to inject `#<name>` into the currently selected value cell. If no editable cell is active, the token is copied to the clipboard for manual use.

## Editor Assistance

Value cells now use a custom delegate that provides basic intellisense:

- Boolean parameters offer `true/false` suggestions automatically
- For every other editable parameter, the completer proposes the aggregated `#variable` tokens from the Variables widget *and* any enumerated samples detected for that specific parameter name. File/path style parameters also pick up include suggestions, making `include some/path.blk` a single keystroke away.

The completer uses popup completion with case-insensitive matching and reacts to partial substrings.

## Structure Editing

A slim toolbar now sits above the parameter table:

- **Add Parameter** (`Ctrl+Alt+P`) launches a quick dialog where you can name the field, pick one of the supported BLK types, and provide an optional initial value.
- **Add Child Block** (`Ctrl+Alt+B`) inserts a new block directly beneath the selected node.
- **Remove Parameter** (`Del`) removes the highlighted row. All operations keep the Qt table, the tree, the variable catalog, and the raw text editor perfectly synchronized.

## Build

```
cd prog/tools
build_qttools
```

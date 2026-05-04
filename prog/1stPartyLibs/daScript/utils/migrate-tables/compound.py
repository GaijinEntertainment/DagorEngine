"""
Second-pass migration for compound insert+get_value patterns.

Handles cases where get_value is the leftmost leaf of a binary expression chain:
    tab.insert(key, tab.get_value(key) + 1 + foo())  →  tab[key] += 1 + foo()

Uses ast-grep JSON output to find remaining patterns after simple rules are applied,
then does text-level analysis to extract the compound assignment.
"""

import json
import re
import subprocess
import sys
import os


def find_matching_paren(s, start):
    """Find the closing paren/bracket matching the opener at s[start]."""
    openers = {"(": ")", "[": "]", "{": "}"}
    closer = openers[s[start]]
    depth = 1
    i = start + 1
    in_string = False
    string_char = None
    while i < len(s):
        c = s[i]
        if in_string:
            if c == "\\" and i + 1 < len(s):
                i += 2
                continue
            if c == string_char:
                in_string = False
        else:
            if c in ('"', "'"):
                in_string = True
                string_char = c
            elif c == s[start]:
                depth += 1
            elif c == closer:
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    return -1


def extract_get_value_prefix(expr, table, key):
    """
    Check if expr starts with a get_value call on the given table+key.
    Returns the length of the get_value prefix if found, else -1.

    Handles:
      - table.get_value(key)       method call
      - table |> get_value(key)    pipe syntax
      - get_value(table, key)      free function
    """
    # Method call: table.get_value(key)
    method_prefix = f"{table}.get_value("
    if expr.startswith(method_prefix):
        paren_start = len(method_prefix) - 1
        paren_end = find_matching_paren(expr, paren_start)
        if paren_end >= 0:
            arg = expr[paren_start + 1 : paren_end].strip()
            if arg == key:
                return paren_end + 1

    # Pipe: table |> get_value(key)
    pipe_prefix = f"{table} |> get_value("
    if expr.startswith(pipe_prefix):
        paren_start = len(pipe_prefix) - 1
        paren_end = find_matching_paren(expr, paren_start)
        if paren_end >= 0:
            arg = expr[paren_start + 1 : paren_end].strip()
            if arg == key:
                return paren_end + 1

    # Free function: get_value(table, key)
    free_prefix = "get_value("
    if expr.startswith(free_prefix):
        paren_start = len(free_prefix) - 1
        paren_end = find_matching_paren(expr, paren_start)
        if paren_end >= 0:
            args_text = expr[paren_start + 1 : paren_end].strip()
            # split on first comma (table might contain commas in complex exprs, but unlikely)
            comma = find_top_level_comma(args_text)
            if comma >= 0:
                arg_table = args_text[:comma].strip()
                arg_key = args_text[comma + 1 :].strip()
                if arg_table == table and arg_key == key:
                    return paren_end + 1

    return -1


def find_top_level_comma(s):
    """Find the first comma not inside parens/brackets/strings."""
    depth = 0
    in_string = False
    string_char = None
    for i, c in enumerate(s):
        if in_string:
            if c == "\\" and i + 1 < len(s):
                continue
            if c == string_char:
                in_string = False
        else:
            if c in ('"', "'"):
                in_string = True
                string_char = c
            elif c in ("(", "[", "{"):
                depth += 1
            elif c in (")", "]", "}"):
                depth -= 1
            elif c == "," and depth == 0:
                return i
    return -1


# Operators that can form compound assignment
COMPOUND_OPS = {
    "+": "+=",
    "-": "-=",
    "*": "*=",
    "/": "/=",
    "|": "|=",
    "&": "&=",
    "^": "^=",
    "<<": "<<=",
    ">>": ">>=",
}


def extract_operator(s):
    """Extract the binary operator at the start of string s (after optional whitespace).
    Returns (operator, rest_after_operator) or (None, None)."""
    s = s.lstrip()
    # Try two-char operators first
    for op in ("<<", ">>"):
        if s.startswith(op):
            return op, s[len(op) :].lstrip()
    # Single-char operators
    for op in ("+", "-", "*", "/", "|", "&", "^"):
        if s.startswith(op):
            # make sure it's not |> (pipe)
            if op == "|" and len(s) > 1 and s[1] == ">":
                return None, None
            return op, s[len(op) :].lstrip()
    return None, None


def try_compound_transform(full_text, table, key, expr):
    """
    Try to transform a compound insert+get_value pattern.

    Returns the replacement text, or None if not transformable.
    """
    prefix_len = extract_get_value_prefix(expr, table, key)
    if prefix_len < 0:
        return None

    rest = expr[prefix_len:]
    op, rhs = extract_operator(rest)
    if op is None or op not in COMPOUND_OPS:
        return None

    if not rhs:
        return None

    compound_op = COMPOUND_OPS[op]
    return f"{table}[{key}] {compound_op} {rhs}"


def run_sg(pattern, selector, filepath):
    """Run ast-grep and return JSON matches."""
    cmd = [
        "sg",
        "run",
        "-p",
        pattern,
        "--selector",
        selector,
        "-l",
        "daslang",
        "--json",
        filepath,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout.strip():
        return []
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return []


def find_compound_patterns(filepath):
    """Find all insert+get_value patterns and attempt compound transformation."""
    replacements = []

    # Method call: #A.insert(#B, #EXPR)
    matches = run_sg(
        'def _() { #A.insert(#B, #EXPR) }', "expression_statement", filepath
    )
    for m in matches:
        meta = m.get("metaVariables", {}).get("single", {})
        table = meta.get("A", {}).get("text", "")
        key = meta.get("B", {}).get("text", "")
        expr = meta.get("EXPR", {}).get("text", "")
        if not table or not key or not expr or "get_value" not in expr:
            continue
        replacement = try_compound_transform(m["text"], table, key, expr)
        if replacement:
            replacements.append(
                {
                    "start": m["range"]["byteOffset"]["start"],
                    "end": m["range"]["byteOffset"]["end"],
                    "original": m["text"],
                    "replacement": replacement,
                    "line": m["range"]["start"]["line"] + 1,
                }
            )

    # Pipe: #A |> insert(#B, #EXPR)  — inner get_value also uses pipe
    matches = run_sg(
        'def _() { #A |> insert(#B, #EXPR) }', "expression_statement", filepath
    )
    for m in matches:
        meta = m.get("metaVariables", {}).get("single", {})
        table = meta.get("A", {}).get("text", "")
        key = meta.get("B", {}).get("text", "")
        expr = meta.get("EXPR", {}).get("text", "")
        if not table or not key or not expr or "get_value" not in expr:
            continue
        replacement = try_compound_transform(m["text"], table, key, expr)
        if replacement:
            replacements.append(
                {
                    "start": m["range"]["byteOffset"]["start"],
                    "end": m["range"]["byteOffset"]["end"],
                    "original": m["text"],
                    "replacement": replacement,
                    "line": m["range"]["start"]["line"] + 1,
                }
            )

    # Free function: insert(#A, #B, #EXPR)
    matches = run_sg(
        'def _() { insert(#A, #B, #EXPR) }', "expression_statement", filepath
    )
    for m in matches:
        meta = m.get("metaVariables", {}).get("single", {})
        table = meta.get("A", {}).get("text", "")
        key = meta.get("B", {}).get("text", "")
        expr = meta.get("EXPR", {}).get("text", "")
        if not table or not key or not expr or "get_value" not in expr:
            continue
        replacement = try_compound_transform(m["text"], table, key, expr)
        if replacement:
            replacements.append(
                {
                    "start": m["range"]["byteOffset"]["start"],
                    "end": m["range"]["byteOffset"]["end"],
                    "original": m["text"],
                    "replacement": replacement,
                    "line": m["range"]["start"]["line"] + 1,
                }
            )

    return replacements


def apply_replacements(filepath, replacements):
    """Apply replacements to file, processing from bottom to top to preserve offsets."""
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # deduplicate by start offset (multiple patterns may match the same location)
    seen = set()
    unique = []
    for r in replacements:
        if r["start"] not in seen:
            seen.add(r["start"])
            unique.append(r)

    # sort by start offset descending so replacements don't shift earlier offsets
    unique.sort(key=lambda r: r["start"], reverse=True)

    for r in unique:
        content = content[: r["start"]] + r["replacement"] + content[r["end"] :]

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    return unique


def main():
    if len(sys.argv) < 2:
        print("Usage: compound.py <file.das> [--apply] [--report]", file=sys.stderr)
        sys.exit(1)

    filepath = sys.argv[1]
    apply = "--apply" in sys.argv
    report_only = "--report" in sys.argv

    if not os.path.isfile(filepath):
        print(f"Error: file not found: {filepath}", file=sys.stderr)
        sys.exit(1)

    replacements = find_compound_patterns(filepath)

    if not replacements:
        if not report_only:
            print(f"  (no compound patterns found)")
        sys.exit(0)

    if apply:
        applied = apply_replacements(filepath, replacements)
        for r in sorted(applied, key=lambda r: r["line"]):
            print(f"  line {r['line']}: {r['original'].strip()}")
            print(f"       -> {r['replacement'].strip()}")
    else:
        for r in sorted(replacements, key=lambda r: r["line"]):
            print(f"  line {r['line']}: {r['original'].strip()}")
            print(f"       -> {r['replacement'].strip()}")


if __name__ == "__main__":
    main()

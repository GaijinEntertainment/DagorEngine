#!/bin/bash
# migrate-tables: Transform insert+get_value patterns to compound assignment
#
# Usage:
#   migrate.sh <file.das>                  Dry-run single file (show diff)
#   migrate.sh <file.das> <output.das>     Migrate single file to output
#   migrate.sh --dir <folder>              Dry-run all .das files recursively
#   migrate.sh --dir <folder> --apply      Migrate in-place recursively
#
# Two-pass migration:
#   Pass 1: ast-grep rules for simple patterns (get_value OP single_expr)
#   Pass 2: Python script for compound patterns (get_value OP expr OP expr ...)
#
# Requires: sg (ast-grep), python, daslang tree-sitter grammar in sgconfig.yml

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RULES="$SCRIPT_DIR/rules.yml"
COMPOUND="$SCRIPT_DIR/compound.py"

usage() {
    echo "Usage:"
    echo "  migrate.sh <file.das>                  Dry-run single file (show diff)"
    echo "  migrate.sh <file.das> <output.das>     Migrate single file to output"
    echo "  migrate.sh --dir <folder>              Dry-run all .das files recursively"
    echo "  migrate.sh --dir <folder> --apply      Migrate in-place recursively"
    echo ""
    echo "Transforms insert+get_value patterns to compound assignment (+=, -=, etc.)"
    exit 1
}

if [ $# -lt 1 ]; then
    usage
fi

if [ ! -f "$RULES" ]; then
    echo "Error: rules file not found: $RULES" >&2
    exit 1
fi

# work from repo root so sgconfig.yml is found
cd "$REPO_ROOT"

# --- detect remaining untransformable patterns in a file ---
detect_remaining() {
    local FILE="$1"
    local PREFIX="${2:-}"
    sg run \
        -p 'def _() { #A.insert(#B, #EXPR) }' \
        --selector expression_statement \
        -l daslang \
        --json "$FILE" 2>/dev/null \
        | python -c "
import sys, json
prefix = '$PREFIX'
data = json.load(sys.stdin)
for m in data:
    text = m.get('text', '')
    if 'get_value' in text:
        line = m['range']['start']['line'] + 1
        if prefix:
            print(f'  {prefix}:{line}: {text.strip()}')
        else:
            print(f'  line {line}: {text.strip()}')
" 2>/dev/null || true
}

# --- single file: dry-run ---
dry_run_file() {
    local INPUT="$1"
    echo "=== Pass 1: Simple patterns (ast-grep rules) ==="
    sg scan -r "$RULES" "$INPUT" 2>&1 || true

    # apply pass 1 to temp copy, then show pass 2 preview
    TEMP=$(mktemp --suffix=.das)
    cp "$INPUT" "$TEMP"
    sg scan -r "$RULES" --update-all "$TEMP" 2>/dev/null || true

    echo ""
    echo "=== Pass 2: Compound patterns (expression tree analysis) ==="
    COMPOUND_OUT=$(python "$COMPOUND" "$TEMP" 2>/dev/null || true)
    if [ -n "$COMPOUND_OUT" ]; then
        echo "$COMPOUND_OUT"
    else
        echo "  (none)"
    fi

    # apply pass 2 to temp copy, then check for leftovers
    python "$COMPOUND" "$TEMP" --apply 2>/dev/null || true

    echo ""
    echo "=== Remaining (manual review needed) ==="
    REMAINING=$(detect_remaining "$TEMP")
    rm -f "$TEMP"

    if [ -n "$REMAINING" ]; then
        echo "$REMAINING"
    else
        echo "  (none)"
    fi
}

# --- single file: apply to output ---
apply_file() {
    local INPUT="$1"
    local OUTPUT="$2"
    cp "$INPUT" "$OUTPUT"

    # pass 1: ast-grep simple rules
    CHANGES=$(sg scan -r "$RULES" --update-all "$OUTPUT" 2>&1 || true)
    COUNT1=$(echo "$CHANGES" | grep -oP 'Applied \K[0-9]+' 2>/dev/null | head -1 || echo "0")

    # pass 2: compound patterns
    COUNT2=0
    COMPOUND_OUT=$(python "$COMPOUND" "$OUTPUT" --apply 2>/dev/null || true)
    if [ -n "$COMPOUND_OUT" ]; then
        COUNT2=$(echo "$COMPOUND_OUT" | grep -c "^\s*line" 2>/dev/null || echo "0")
    fi

    TOTAL=$((COUNT1 + COUNT2))
    echo "Migrated: $INPUT -> $OUTPUT"
    echo "  Pass 1 (simple):   $COUNT1 transformation(s)"
    echo "  Pass 2 (compound): $COUNT2 transformation(s)"
    echo "  Total:             $TOTAL transformation(s)"

    REMAINING=$(detect_remaining "$OUTPUT")
    if [ -n "$REMAINING" ]; then
        echo ""
        echo "Remaining (manual review needed):"
        echo "$REMAINING"
    fi
}

# --- batch: in-place on a file, returns stats via globals ---
TOTAL_FILES=0
TOTAL_CHANGED=0
TOTAL_PASS1=0
TOTAL_PASS2=0
ALL_REMAINING=""

apply_file_inplace() {
    local FILE="$1"
    TOTAL_FILES=$((TOTAL_FILES + 1))

    # pass 1
    CHANGES=$(sg scan -r "$RULES" --update-all "$FILE" 2>&1 || true)
    COUNT1=$(echo "$CHANGES" | grep -oP 'Applied \K[0-9]+' 2>/dev/null | head -1 || echo "0")

    # pass 2
    COUNT2=0
    COMPOUND_OUT=$(python "$COMPOUND" "$FILE" --apply 2>/dev/null || true)
    if [ -n "$COMPOUND_OUT" ]; then
        COUNT2=$(echo "$COMPOUND_OUT" | grep -c "^\s*line" 2>/dev/null || echo "0")
    fi

    TOTAL=$((COUNT1 + COUNT2))
    if [ "$TOTAL" != "0" ]; then
        TOTAL_CHANGED=$((TOTAL_CHANGED + 1))
        TOTAL_PASS1=$((TOTAL_PASS1 + COUNT1))
        TOTAL_PASS2=$((TOTAL_PASS2 + COUNT2))
        echo "  $FILE: $COUNT1 simple + $COUNT2 compound = $TOTAL total"
    fi

    REMAINING=$(detect_remaining "$FILE" "$FILE")
    if [ -n "$REMAINING" ]; then
        ALL_REMAINING="${ALL_REMAINING}${REMAINING}"$'\n'
    fi
}

# --- parse arguments ---
if [ "$1" = "--dir" ]; then
    # batch mode
    if [ $# -lt 2 ]; then
        usage
    fi
    DIR="$2"
    APPLY=false
    if [ "${3:-}" = "--apply" ]; then
        APPLY=true
    fi

    if [ ! -d "$DIR" ]; then
        echo "Error: directory not found: $DIR" >&2
        exit 1
    fi

    # collect .das files
    FILES=$(find "$DIR" -name '*.das' -type f | sort)
    FILE_COUNT=$(echo "$FILES" | wc -l)

    if [ "$APPLY" = true ]; then
        echo "Migrating $FILE_COUNT .das files in $DIR ..."
        echo ""
        while IFS= read -r FILE; do
            apply_file_inplace "$FILE"
        done <<< "$FILES"

        TOTAL_ALL=$((TOTAL_PASS1 + TOTAL_PASS2))
        echo ""
        echo "=== Summary ==="
        echo "Files scanned:     $TOTAL_FILES"
        echo "Files changed:     $TOTAL_CHANGED"
        echo "Pass 1 (simple):   $TOTAL_PASS1"
        echo "Pass 2 (compound): $TOTAL_PASS2"
        echo "Total transforms:  $TOTAL_ALL"

        if [ -n "$ALL_REMAINING" ]; then
            echo ""
            echo "Remaining (manual review needed):"
            echo "$ALL_REMAINING"
        fi
    else
        # dry-run: just run sg scan without --update-all on the whole directory
        echo "=== Dry-run: $FILE_COUNT .das files in $DIR ==="
        echo ""
        echo "--- Pass 1: Simple patterns (ast-grep rules) ---"
        sg scan -r "$RULES" "$DIR" 2>&1 || true

        echo ""
        echo "--- Pass 2: Compound patterns ---"
        FOUND_COMPOUND=false
        FOUND_REMAINING=false
        REMAINING_TEXT=""
        while IFS= read -r FILE; do
            TEMP=$(mktemp --suffix=.das)
            cp "$FILE" "$TEMP"
            sg scan -r "$RULES" --update-all "$TEMP" 2>/dev/null || true

            COMPOUND_OUT=$(python "$COMPOUND" "$TEMP" 2>/dev/null || true)
            if [ -n "$COMPOUND_OUT" ]; then
                echo "  $FILE:"
                echo "$COMPOUND_OUT" | sed 's/^/    /'
                FOUND_COMPOUND=true
            fi

            python "$COMPOUND" "$TEMP" --apply 2>/dev/null || true
            REMAINING=$(detect_remaining "$TEMP" "$FILE")
            rm -f "$TEMP"
            if [ -n "$REMAINING" ]; then
                REMAINING_TEXT="${REMAINING_TEXT}${REMAINING}"$'\n'
                FOUND_REMAINING=true
            fi
        done <<< "$FILES"
        if [ "$FOUND_COMPOUND" = false ]; then
            echo "  (none)"
        fi

        echo ""
        echo "--- Remaining (manual review needed) ---"
        if [ "$FOUND_REMAINING" = true ]; then
            echo "$REMAINING_TEXT"
        else
            echo "  (none)"
        fi
    fi
else
    # single file mode
    INPUT="$1"
    OUTPUT="${2:-}"

    if [ ! -f "$INPUT" ]; then
        echo "Error: file not found: $INPUT" >&2
        exit 1
    fi

    if [ -z "$OUTPUT" ]; then
        dry_run_file "$INPUT"
    else
        apply_file "$INPUT" "$OUTPUT"
    fi
fi

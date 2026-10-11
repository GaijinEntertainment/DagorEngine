options gen2
options indenting = 4

require strings
require offsets
require comments


enum public EditKind {
    //! What an edit does to its range — the engine checks each kind differently.
    INSERT      //!< zero-length: text goes in at `from`
    REPLACE     //!< [from, to) becomes `text`; may carry comments through in `text`
    DELETE      //!< [from, to) goes away; refused if it holds a comment or string
}

struct public Edit {
    //! One rewrite in original coordinates.
    from : int
    to : int
    text : string
    kind : EditKind = EditKind.INSERT
    rank : int = RANK_CLOSE_INNER
    rule : string
    origin : int = -1
}

let public RANK_CLOSE_INNER = 0
let public RANK_CLOSE_OUTER = 10
let public RANK_BLOCK_OPEN = 15
let public RANK_TERMINATOR = 20

struct public EditSet {
    //! Edits plus whatever the engine refused, with the reason.
    edits : array<Edit>
    refused : array<string>
}


def private push_edit(var set : EditSet; e : Edit) {
    for (x in set.edits) {
        if (x.from == e.from && x.to == e.to && x.kind == e.kind && x.rank == e.rank
                && x.origin == e.origin && x.rule == e.rule && x.text == e.text) return
    }
    set.edits |> push(e)
}


def public insert(var set : EditSet; at : int; text : string; rule : string;
                  rank : int = RANK_CLOSE_INNER; origin : int = -1) {
    //! Records "put `text` at `at`". Independent of how much the surrounding text grows.
    //!
    //! `origin` is the start of the construct being closed; two closers landing on one offset
    //! use it to nest correctly.
    set |> push_edit(Edit(from = at, to = at, text = text, kind = EditKind.INSERT, rank = rank,
        rule = rule, origin = origin))
}


def public replace(var set : EditSet; from, to : int; text : string; rule : string; rank : int = RANK_CLOSE_INNER) {
    //! Records "[from, to) becomes `text`". Use for rearrangement: the comments of the old
    //! range have to reappear in `text`, and the conservation gate checks that they do.
    set |> push_edit(Edit(from = from, to = to, text = text, kind = EditKind.REPLACE, rank = rank, rule = rule))
}


def public remove(var set : EditSet; from, to : int; rule : string; rank : int = RANK_CLOSE_INNER) {
    //! Records "[from, to) goes away". Only for delimiter and keyword tokens — `apply` refuses
    //! a deletion that would take a comment or a string literal with it.
    set |> push_edit(Edit(from = from, to = to, text = "", kind = EditKind.DELETE, rank = rank, rule = rule))
}


def private before(a, b : Edit) : bool {
    if (a.from != b.from) return a.from < b.from
    let aInsert = a.to == a.from
    let bInsert = b.to == b.from
    if (aInsert != bInsert) return aInsert
    if (a.to != b.to) return a.to > b.to
    if (a.rank != b.rank) return a.rank < b.rank
    return a.origin > b.origin
}


def public apply(src : Source; lex : Lexical; var set : EditSet) : string {
    //! Emits the rewritten source: one forward walk over the original, substituting edits.
    //!
    //! Two edits that overlap without nesting are a contradiction, not something to merge —
    //! the later one is refused and reported. A deletion that holds a comment is refused the
    //! same way, so a rule can never lose a comment by widening its range.
    var ordered <- [for (e in set.edits); e]
    sort(ordered) $(a, b) => before(a, b)

    return build_string() $(writer) {
        var cursor = 0
        var prev = ""
        for (e in ordered) {
            if (e.from < cursor) {
                set.refused |> push("{e.rule}: edit at {e.from}..{e.to} overlaps an earlier edit {prev} (cursor {cursor})")
                continue
            }
            if (e.kind == EditKind.DELETE && has_comment_or_string(lex, e.from, e.to)) {
                set.refused |> push("{e.rule}: deletion of {e.from}..{e.to} would remove a comment or string")
                continue
            }
            writer |> write(text_of(src, cursor, e.from))
            writer |> write(e.text)
            cursor = e.to
            prev = "{e.rule}[{e.from}..{e.to}]"
        }
        writer |> write(text_of(src, cursor, src.size))
    }
}


struct public Conservation {
    //! Result of the comment-conservation gate.
    ok : bool
    detail : string
}


def public check_comments_preserved(before_text, after_text : string) : Conservation {
    //! Every comment of the input must appear in the output, in the same order, once each.
    //!
    //! This is a release gate rather than a diagnostic: a comment is the one thing no tool can
    //! regenerate. It catches loss, duplication (a rule emitting a range twice) and reordering
    //! in a single comparison. Comments may MOVE to an adjacent anchor — the text is compared,
    //! not the position.
    let src_before = make_source(before_text)
    let src_after = make_source(after_text)
    let was <- comment_texts(src_before, scan_lexical(src_before))
    let now <- comment_texts(src_after, scan_lexical(src_after))
    if (length(was) != length(now)) {
        return Conservation(ok = false, detail = "comment count {length(was)} -> {length(now)}")
    }
    for (i in range(length(was))) {
        if (was[i] != now[i]) {
            return Conservation(ok = false, detail = "comment {i + 1} changed: '{was[i]}' -> '{now[i]}'")
        }
    }
    return Conservation(ok = true, detail = "")
}

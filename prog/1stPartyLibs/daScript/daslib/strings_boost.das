options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module strings_boost shared public

//! Extended string manipulation functions.
//!
//! Adds ``wide``, ``split``, ``join``, ``escape``, ``unescape``,
//! ``starts_with``, ``ends_with``, ``replace``, string-builder helpers,
//! and many other utilities on top of the built-in ``strings`` module.

require math
require strings public

def wide(text : string implicit; width : int) {
    //! pads text with spaces to the right to achieve desired width
    let extra = width - length(text)
    if (extra > 0) {
        return text + repeat(" ", extra)
    } else {
        return text
    }
}


def join(it; separator : string implicit) {
    //! joins iterable `it` elements into a single string with `separator` between elements
    static_if (!typeinfo is_iterable(it)) {
        concept_assert(false, "can't join non-iterable type")
        return ""
    } else {
        let st = build_string() $(var writer) {
            var skip_first = true
            for (elem in it) {
                if (skip_first) {
                    skip_first = false
                } else {
                    write(writer, separator)
                }
                if (typeinfo is_string(it)) {
                    write_char(writer, elem)
                } else {
                    write(writer, elem)
                }
            }
        }
        return st
    }
}

def join(var it : iterator<auto(TT)>; separator : string implicit) {
    //! joins iterable `it` elements into a single string with `separator` between elements
    let st = build_string() $(var writer) {
        var skip_first = true
        for (elem in it) {
            if (skip_first) {
                skip_first = false
            } else {
                write(writer, separator)
            }
            if (typeinfo is_string(it)) {
                write_char(writer, elem)
            } else {
                write(writer, elem)
            }
        }
    }
    return st
}

def private join_implement(iterable; separator : string; blk) : string {
    //! joins iterable `it` elements into a single string with `separator` between elements
    let st = build_string() $(var writer) {
        var skip_first = true
        for (elem in iterable) {
            if (skip_first) {
                skip_first = false
            } else {
                write(writer, separator)
            }
            blk |> invoke(writer, elem)
        }
    }
    return st
}

def join(iterable : array<auto(TT)>; separator : string; blk : block<(var writer : StringBuilderWriter; elem : TT) : void>) : string {
    //! joins iterable `it` elements into a single string with `separator` between elements
    return join_implement(iterable, separator, blk)
}

def join(var iterable : iterator<auto(TT)>; separator : string; blk : block<(var writer : StringBuilderWriter; elem : TT) : void>) : string {
    //! joins iterable `it` elements into a single string with `separator` between elements
    return join_implement(iterable, separator, blk)
}

def join(iterable : auto(TT)[]; separator : string; blk : block<(var writer : StringBuilderWriter; elem : TT) : void>) : string {
    //! joins iterable `it` elements into a single string with `separator` between elements
    return join_implement(iterable, separator, blk)
}

def split(text, delim : string implicit; blk : block< (arg : array<string>#) >) {
    //! splits text by delimiter and calls blk with the resulting array
    builtin_string_split(text, delim, blk)
}

def split(text, delim : string implicit) : array<string> {
    //! splits text by delimiter and returns the resulting array
    var res : array<string>
    builtin_string_split(text, delim) $(arr : array<string>#) : void {
        res := arr
    }
    return <- res
}

def split_by_chars(text, delim : string implicit; blk : block< (arg : array<string>#) >) {
    //! splits text by any of the characters in delim and calls blk with the resulting array
    builtin_string_split_by_char(text, delim, blk)
}

def split_by_chars(text, delim : string implicit) : array<string> {
    //! splits text by any of the characters in delim and returns the resulting array
    var res : array<string>
    builtin_string_split_by_char(text, delim) $(arr : array<string>#) : void {
        res := arr
    }
    return <- res
}

[generic]
def is_character_at(foo : array<uint8> implicit; idx : int; ch : int) {
    //! returns true if character at position `idx` in `foo` equals `ch`
    return int(foo[idx]) == ch
}

[generic]
def eq(a : string implicit; b : das_string) {
    //! returns true if strings are equal
    return a == b
}

[generic]
def eq(b : das_string; a : string implicit) {
    //! returns true if strings are equal
    return b == a
}


// https://en.wikipedia.org/wiki/Levenshtein_distance
def levenshtein_distance(s, t : string implicit) : int {
    //! computes Levenshtein distance between strings s and t
    let sLen = length(s)
    let tLen = length(t)
    var inscope d : array<array<int>>
    d |> resize(sLen + 1)
    for (i in range(sLen + 1)) {
        d[i] |> resize(tLen + 1)
        d[i][0] = i
    }
    for (j in range(tLen + 1)) {
        d[0][j] = j
    }

    for (i in range(1, sLen + 1)) {
        for (j in range(1, tLen + 1)) {
            let substitutionCost = unsafe(character_uat(s, i - 1)) == unsafe(character_uat(t, j - 1)) ? 0 : 1
            d[i][j] = min(d[i - 1][j] + 1, d[i][j - 1] + 1)
            d[i][j] = min(d[i][j], d[i - 1][j - 1] + substitutionCost)
        }
    }

    return d[sLen][tLen]
}

// https://en.wikipedia.org/wiki/Levenshtein_distance#Iterative_with_two_matrix_rows
// https://www.codeproject.com/Articles/13525/Fast-memory-efficient-Levenshtein-algorithm-2
def levenshtein_distance_fast(s, t : string implicit) : int {
    //! computes Levenshtein distance between strings s and t using optimized memory
    let sLen = length(s)
    let tLen = length(t)
    var inscope v0 : array<int>
    v0 |> resize(tLen + 1)
    var inscope v1 : array<int>
    v1 |> resize(tLen + 1)
    for (i in range(tLen + 1)) {
        v0[i] = i
    }

    for (i in range(0, sLen)) {
        v1[0] = i + 1
        for (j in range(0, tLen)) {
            let deletionCost = v0[j + 1] + 1
            let insertionCost = v1[j] + 1
            let substitutionCost = v0[j] + (unsafe(character_uat(s, i)) == unsafe(character_uat(t, j)) ? 0 : 1)
            v1[j + 1] = min(deletionCost, insertionCost)
            v1[j + 1] = min(v1[j + 1], substitutionCost)
        }

        swap(v0, v1)
    }

    return v0[tLen]
}

def replace_multiple(source : string; replaces : array<tuple<text : string; replacement : string>>) {
    //! replaces occurances of multiple strings in a string. does not support overlap
    if (empty(source) || empty(replaces)) {
        return source
    }
    //! replaces occurances of multiple strings in a string. does not support overlap
    return build_string() $(writer) {
        var index = 0
        let len = source |> length
        while (index < len) {
            var found = false
            for (replace in replaces) {
                if (source |> starts_with(index, replace.text)) {
                    writer |> write(replace.replacement)
                    index += replace.text |> length
                    found = true
                    break
                }
            }
            if (!found) {
                writer |> write_char(unsafe(source |> character_uat(index)))
                index ++
            }
        }
    }
}

def public contains(str, sub : string) : bool {
    //! Returns true if ``sub`` is found anywhere within ``str``.
    return find(str, sub) >= 0
}

def public count(str, sub : string) : int {
    //! Counts non-overlapping occurrences of ``sub`` in ``str``.
    //! Returns 0 if ``sub`` is empty or not found.
    if (sub |> empty()) {
        return 0
    }
    var n = 0
    var pos = find(str, sub)
    let subLen = sub |> length()
    while (pos >= 0) {
        n ++
        pos = find(str, sub, pos + subLen)
    }
    return n
}

def public last_index_of(str, sub : string) : int {
    //! Returns the index of the last occurrence of ``sub`` in ``str``, or -1 if not found.
    if (sub |> empty() || str |> empty()) {
        return -1
    }
    var lastPos = -1
    var pos = find(str, sub)
    while (pos >= 0) {
        lastPos = pos
        pos = find(str, sub, pos + 1)
    }
    return lastPos
}

def public last_index_of(str, sub : string; start : int) : int {
    //! Returns the index of the last occurrence of ``sub`` in ``str`` searching only up to position ``start`` (exclusive), or -1 if not found.
    if (sub |> empty() || str |> empty()) {
        return -1
    }
    var lastPos = -1
    var pos = find(str, sub)
    while (pos >= 0 && pos < start) {
        lastPos = pos
        pos = find(str, sub, pos + 1)
    }
    return lastPos
}

def public pad_right(str : string; width : int; ch : int = ' ') : string {
    //! Pads the string with the character ``ch`` on the right to reach the specified minimum ``width``.
    //! If the string is already at least ``width`` characters, it is returned unchanged.
    let l = str |> length()
    if (l >= width) {
        return str
    }
    return build_string() $(writer) {
        writer |> write(str)
        writer |> write_chars(ch, width - l)
    }
}

def public pad_left(str : string; width : int; ch : int = ' ') : string {
    //! Pads the string with the character ``ch`` on the left to reach the specified minimum ``width``.
    //! If the string is already at least ``width`` characters, it is returned unchanged.
    let l = str |> length()
    if (l >= width) {
        return str
    }
    return build_string() $(writer) {
        writer |> write_chars(ch, width - l)
        writer |> write(str)
    }
}

def public trim_prefix(str, prefix : string) : string {
    //! Removes ``prefix`` from the beginning of ``str`` if present.
    //! Returns the string unchanged if it does not start with ``prefix``.
    if (str |> starts_with(prefix)) {
        return str |> slice(prefix |> length())
    }
    return str
}

def public trim_suffix(str, suffix : string) : string {
    //! Removes ``suffix`` from the end of ``str`` if present.
    //! Returns the string unchanged if it does not end with ``suffix``.
    if (str |> ends_with(suffix)) {
        return str |> slice(0, (str |> length()) - (suffix |> length()))
    }
    return str
}

def public capitalize(str : string) : string {
    //! Returns a copy of ``str`` with the first character converted to uppercase.
    //! The rest of the string is unchanged.
    if (str |> empty()) {
        return str
    }
    let first = first_character(str)
    if (first >= 'a' && first <= 'z') {
        return build_string() $(writer) {
            writer |> write_char(first - 'a' + 'A')
            if (str |> length() > 1) {
                writer |> write(str |> slice(1))
            }
        }
    }
    return str
}

def public is_null_or_whitespace(str : string) : bool {
    //! Returns true if the string is null, empty, or contains only whitespace characters (space, tab, CR, LF).
    if (str |> empty()) {
        return true
    }
    var result = true
    peek_data(str) $(data) {
        for (i in range(length(data))) {
            if (!is_white_space(int(data[i]))) {
                result = false
                return
            }
        }
    }
    return result
}

def public glob_match(pattern, text : string) : bool {
    //! Matches ``text`` against a shell-style glob ``pattern``.
    //! Supports ``*`` (matches any sequence of characters, including empty)
    //! and ``?`` (matches exactly one character). All other characters are
    //! compared literally.  Returns true if the entire ``text`` matches.
    if (empty(pattern)) {
        return empty(text)
    }
    if (empty(text)) {
        // text is empty — only matches if pattern is all *'s
        unsafe {
            var pp = reinterpret<uint8?> pattern
            while (*pp == uint8('*')) {
                pp++
            }
            return *pp == 0u8
        }
    }
    unsafe {
        var pp = reinterpret<uint8?> pattern
        var tp = reinterpret<uint8?> text
        var star_pp : uint8?
        var star_tp : uint8?
        while (*tp != 0u8) {
            if (*pp == uint8('?')) {
                pp++
                tp++
            } elif (*pp == uint8('*')) {
                star_pp = pp
                star_tp = tp
                pp++
            } elif (*pp == *tp) {
                pp++
                tp++
            } elif (star_pp != null) {
                pp = star_pp + 1
                star_tp++
                tp = star_tp
            } else {
                return false
            }
        }
        while (*pp == uint8('*')) {
            pp++
        }
        return *pp == 0u8
    }
}
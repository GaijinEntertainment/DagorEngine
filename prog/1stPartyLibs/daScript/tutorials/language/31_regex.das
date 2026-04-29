// Tutorial 31: Regular Expressions
//
// This tutorial covers:
//   - Compiling regex patterns with regex_compile
//   - Matching with regex_match
//   - Character classes: \w \W \d \D \s \S
//   - Anchors: ^ (beginning) and $ (end)
//   - Quantifiers: + * ? and {n,m}
//   - Groups and captures with regex_group
//   - Character sets: [abc], [a-z], [^abc]
//   - Alternation: a|b
//   - Word boundaries: \b and \B
//   - Finding all matches with regex_foreach
//   - Search-and-replace with regex_replace
//   - Compile-time regex with %regex~ reader macro
//   - Hex escapes: \x41
//   - Escaped metacharacters: \. \+ \* etc.
//   - Searching with regex_search
//   - Splitting with regex_split
//   - Collecting all matches with regex_match_all
//   - Non-capturing groups (?:...)
//   - Named groups (?P<name>...) and regex_group_by_name
//   - Lazy quantifiers: *? +? ?? {n,m}?
//   - Case-insensitive matching
//   - Dot/newline behavior and dot_all mode
//   - Lookahead assertions (?=...) (?!...)
//   - Template-string regex_replace with $1/${name}
//   - Reader macro flags (~i, ~s)
//
// requires:  daslib/regex, daslib/regex_boost
// Run: daslang.exe tutorials/language/31_regex.das

options gen2
options persistent_heap

require daslib/regex_boost

// === Compiling and Matching ===
// regex_compile creates a Regex from a pattern string.
// regex_match returns the end position of the match from position 0, or -1.

def compiling_and_matching() {
    print("=== Compiling and matching ===\n")

    // compile a pattern
    var re <- regex_compile("hello")

    // match returns end position or -1
    let pos = regex_match(re, "hello world")
    print("match 'hello' in 'hello world': {pos}\n")
    // output: match 'hello' in 'hello world': 5

    // no match returns -1
    let no = regex_match(re, "goodbye")
    print("match 'hello' in 'goodbye': {no}\n")
    // output: match 'hello' in 'goodbye': -1

    // match with offset — skip first N characters
    var re2 <- regex_compile("world")
    let pos2 = regex_match(re2, "hello world", 6)
    print("match 'world' at offset 6: {pos2}\n")
    // output: match 'world' at offset 6: 11
}

// === Character Classes ===
// \w = word chars [a-zA-Z0-9_]   \W = non-word chars
// \d = digits [0-9]               \D = non-digits
// \s = whitespace (space, \t, \n, \r, \f, \v)  \S = non-whitespace

def character_classes() {
    print("\n=== Character classes ===\n")

    // \d+ matches one or more digits
    var re_num <- regex_compile("\\d+")
    print("digits: {regex_match(re_num, "12345")}\n")
    // output: digits: 5

    // \w+ matches word characters
    var re_word <- regex_compile("\\w+")
    print("word: {regex_match(re_word, "hello_42")}\n")
    // output: word: 8

    // \s+ matches whitespace
    var re_ws <- regex_compile("\\s+")
    print("space: {regex_match(re_ws, "   x")}\n")
    // output: space: 3

    // \S+ matches non-whitespace
    var re_nws <- regex_compile("\\S+")
    print("non-space: {regex_match(re_nws, "abc def")}\n")
    // output: non-space: 3
}

// === Anchors ===
// ^ anchors to the beginning of the string
// $ anchors to the end of the string

def anchors() {
    print("\n=== Anchors ===\n")

    // ^ only matches at position 0
    var re_start <- regex_compile("^hello")
    print("^hello on 'hello': {regex_match(re_start, "hello")}\n")
    // output: ^hello on 'hello': 5

    print("^hello on 'say hello': {regex_match(re_start, "say hello")}\n")
    // output: ^hello on 'say hello': -1

    // $ only matches if the pattern ends at the string end
    var re_end <- regex_compile("end$")
    print("end$ on 'end': {regex_match(re_end, "end")}\n")
    // output: end$ on 'end': 3

    // ^...$ together require a full match
    var re_full <- regex_compile("^abc$")
    print("^abc$ on 'abc': {regex_match(re_full, "abc")}\n")
    // output: ^abc$ on 'abc': 3

    print("^abc$ on 'abcd': {regex_match(re_full, "abcd")}\n")
    // output: ^abc$ on 'abcd': -1
}

// === Quantifiers ===
// +  one or more (greedy)
// *  zero or more (greedy)
// ?  zero or one
// {n}   exactly n
// {n,}  n or more
// {n,m} between n and m (greedy)

def quantifiers() {
    print("\n=== Quantifiers ===\n")

    var re_plus <- regex_compile("a+")
    print("a+ on 'aaa': {regex_match(re_plus, "aaa")}\n")
    // output: a+ on 'aaa': 3

    var re_star <- regex_compile("a*b")
    print("a*b on 'b': {regex_match(re_star, "b")}\n")
    // output: a*b on 'b': 1
    print("a*b on 'aaab': {regex_match(re_star, "aaab")}\n")
    // output: a*b on 'aaab': 4

    var re_q <- regex_compile("colou?r")
    print("colou?r on 'color': {regex_match(re_q, "color")}\n")
    // output: colou?r on 'color': 5
    print("colou?r on 'colour': {regex_match(re_q, "colour")}\n")
    // output: colou?r on 'colour': 6

    // counted quantifiers: {n}, {n,}, {n,m}
    var re_exact <- regex_compile("\\d\{4}")
    print("\\d 4 on '1234': {regex_match(re_exact, "1234")}\n")
    // output: \d 4 on '1234': 4

    var re_range <- regex_compile("a\{2,4}")
    print("a 2-4 on 'a': {regex_match(re_range, "a")}\n")
    // output: a 2-4 on 'a': -1
    print("a 2-4 on 'aaa': {regex_match(re_range, "aaa")}\n")
    // output: a 2-4 on 'aaa': 3
    print("a 2-4 on 'aaaaa': {regex_match(re_range, "aaaaa")}\n")
    // output: a 2-4 on 'aaaaa': 4

    var re_min <- regex_compile("x\{2,}")
    print("x 2+ on 'xxxxx': {regex_match(re_min, "xxxxx")}\n")
    // output: x 2+ on 'xxxxx': 5
}

// === Groups and Alternation ===
// (...) creates a capturing group
// a|b alternation — matches either a or b

def groups_and_alternation() {
    print("\n=== Groups and alternation ===\n")

    // alternation
    var re_alt <- regex_compile("cat|dog")
    print("cat|dog on 'cat': {regex_match(re_alt, "cat")}\n")
    // output: cat|dog on 'cat': 3
    print("cat|dog on 'dog': {regex_match(re_alt, "dog")}\n")
    // output: cat|dog on 'dog': 3

    // capturing groups
    var re_grp <- regex_compile("(\\w+)@(\\w+)")
    let inp = "user@host"
    let m = regex_match(re_grp, inp)
    print("match: {m}\n")
    // output: match: 9
    print("group 1: {regex_group(re_grp, 1, inp)}\n")
    // output: group 1: user
    print("group 2: {regex_group(re_grp, 2, inp)}\n")
    // output: group 2: host
}

// === Character Sets ===
// [abc]   matches any of a, b, c
// [a-z]   matches range a through z
// [^abc]  matches any char NOT in the set
// [\d_]   character classes work inside sets

def character_sets() {
    print("\n=== Character sets ===\n")

    var re_vowel <- regex_compile("[aeiou]+")
    print("vowels: {regex_match(re_vowel, "aeiou")}\n")
    // output: vowels: 5

    var re_range <- regex_compile("[A-Z]+")
    print("upper: {regex_match(re_range, "HELLO")}\n")
    // output: upper: 5

    var re_neg <- regex_compile("[^0-9]+")
    print("non-digits: {regex_match(re_neg, "abc")}\n")
    // output: non-digits: 3

    var re_mixed <- regex_compile("[\\d_]+")
    print("digits or underscore: {regex_match(re_mixed, "12_34")}\n")
    // output: digits or underscore: 5
}

// === Word Boundaries ===
// \b matches at a word boundary (between \w and \W or start/end)
// \B matches at a non-word boundary

def word_boundaries() {
    print("\n=== Word boundaries ===\n")

    // \b at word start — use offset to search for "world"
    var re_bnd <- regex_compile("\\bworld")
    print("word start: {regex_match(re_bnd, "hello world", 6)}\n")
    // output: word start: 11

    // \b at word end
    var re_end <- regex_compile("hello\\b")
    print("word end: {regex_match(re_end, "hello world")}\n")
    // output: word end: 5

    // \b at string boundary
    var re_str <- regex_compile("\\bhello\\b")
    print("whole word: {regex_match(re_str, "hello")}\n")
    // output: whole word: 5

    // \B matches inside a word
    var re_nb <- regex_compile("\\Bell")
    print("interior: {regex_match(re_nb, "hello", 1)}\n")
    // output: interior: 4
}

// === Foreach and Replace ===
// regex_foreach iterates all matches, calling a block with the range
// regex_replace replaces all matches using a block

def foreach_and_replace() {
    print("\n=== Foreach and replace ===\n")

    // find all digit sequences
    var re_num <- regex_compile("\\d+")
    let src = "a12b34c56"
    print("matches in '{src}': ")
    regex_foreach(re_num, src) $(at) {
        print("[{at.x},{at.y}]='{slice(src, at.x, at.y)}' ")
        return true  // continue
    }
    print("\n")
    // output: matches in 'a12b34c56': [1,3]='12' [4,6]='34' [7,9]='56'

    // replace all digit sequences with "X"
    let result = regex_replace(re_num, src) $(match_str) {
        return "X"
    }
    print("replaced: {result}\n")
    // output: replaced: aXbXcX
}

// === Escaped Metacharacters ===
// Use backslash to match literal metacharacters: \. \+ \* \( \) etc.

def escaped_meta() {
    print("\n=== Escaped metacharacters ===\n")

    var re_dot <- regex_compile("\\d+\\.\\d+")
    print("decimal: {regex_match(re_dot, "3.14")}\n")
    // output: decimal: 4

    var re_parens <- regex_compile("\\(\\w+\\)")
    print("parens: {regex_match(re_parens, "(hello)")}\n")
    // output: parens: 7
}

// === Reader Macro ===
// %regex~pattern%% compiles a regex at compile time
// No double-escaping needed — backslashes are literal

def reader_macro() {
    print("\n=== Reader macro ===\n")

    var re <- %regex~\d+%%
    print("reader macro digits: {regex_match(re, "42abc")}\n")
    // output: reader macro digits: 2

    var re2 <- %regex~[a-z]+%%
    print("reader macro lower: {regex_match(re2, "hello")}\n")
    // output: reader macro lower: 5
}

// === Search, Split, Match All ===
// regex_search finds the first match anywhere (unlike regex_match which only matches at offset 0).
// regex_split splits a string by regex matches.
// regex_match_all collects all match ranges.

def search_split_matchall() {
    print("\n=== Search, split, match_all ===\n")

    // regex_search returns int2(start, end) or int2(-1,-1)
    var re_num <- regex_compile("\\d+")
    let pos = regex_search(re_num, "abc 123 def")
    print("search digits: start={pos.x}, end={pos.y}\n")
    // output: search digits: start=4, end=7

    // regex_split splits by matches
    var re_comma <- regex_compile(",\\s*")
    var parts <- regex_split(re_comma, "a, b,c, d")
    print("split: ")
    for (p in parts) {
        print("'{p}' ")
    }
    print("\n")
    // output: split: 'a' 'b' 'c' 'd'

    // regex_match_all returns array<range>
    var re_word <- regex_compile("\\w+")
    var matches <- regex_match_all(re_word, "foo bar baz")
    print("match_all count: {length(matches)}\n")
    // output: match_all count: 3
}

// === Non-Capturing Groups ===
// (?:...) groups without capturing — useful for alternation/quantifiers
// without adding to the group count.

def non_capturing_groups() {
    print("\n=== Non-capturing groups ===\n")

    // (?:cat|dog) matches either but doesn't capture
    var re <- regex_compile("(?:cat|dog)fish")
    print("catfish: {regex_match(re, "catfish")}\n")
    // output: catfish: 7
    print("dogfish: {regex_match(re, "dogfish")}\n")
    // output: dogfish: 7
    print("group count: {length(re.groups)}\n")
    // output: group count: 1

    // non-capturing with quantifier
    var re2 <- regex_compile("(?:ab)\{3}")
    print("ababab: {regex_match(re2, "ababab")}\n")
    // output: ababab: 6
}

// === Named Groups ===
// (?P<name>...) — capturing group accessible by name via regex_group_by_name.

def named_groups() {
    print("\n=== Named groups ===\n")

    var re <- regex_compile("(?P<user>\\w+)@(?P<host>\\w+)")
    let email = "alice@example"
    let m = regex_match(re, email)
    if (m != -1) {
        print("user: {regex_group_by_name(re, "user", email)}\n")
        // output: user: alice
        print("host: {regex_group_by_name(re, "host", email)}\n")
        // output: host: example
        // named groups are also accessible by index
        print("group 1: {regex_group(re, 1, email)}\n")
        // output: group 1: alice
    }
}

// === Lazy Quantifiers ===
// Greedy quantifiers (+, *, ?, {n,m}) match as much as possible.
// Lazy quantifiers (+?, *?, ??, {n,m}?) match as little as possible.

def lazy_quantifiers() {
    print("\n=== Lazy quantifiers ===\n")

    // lazy .*? — matches the shortest possible
    var re_lazy <- regex_compile("\".*?\"")
    let text = "say \"hi\" and \"bye\""
    let lazy_pos = regex_search(re_lazy, text)
    print("lazy .*?: '{slice(text, lazy_pos.x, lazy_pos.y)}'\n")
    // output: lazy .*?: '"hi"'

    // lazy +? — match one or more, but as few as possible
    var re_plus <- regex_compile("<.+?>")
    let html = "<b>bold</b>"
    let tag_pos = regex_search(re_plus, html)
    print("lazy tag: '{slice(html, tag_pos.x, tag_pos.y)}'\n")
    // output: lazy tag: '<b>'

    // greedy vs lazy with quantifiers at end of pattern
    var re_g <- regex_compile("a+")
    var re_l <- regex_compile("a+?")
    print("greedy a+:  {regex_match(re_g, "aaa")}\n")
    // output: greedy a+:  3
    print("lazy   a+?: {regex_match(re_l, "aaa")}\n")
    // output: lazy   a+?: 1

    // lazy {n,m}? — prefer minimum repetitions
    var re_rep_g <- regex_compile("a\{2,5}")
    var re_rep_l <- regex_compile("a\{2,5}?")
    print("greedy a\{2,5\}:  ")
    print("{regex_match(re_rep_g, "aaaaa")}\n")
    // output: greedy a{2,5}:  5
    print("lazy   a\{2,5\}?: ")
    print("{regex_match(re_rep_l, "aaaaa")}\n")
    // output: lazy   a{2,5}?: 2
}

// === Practical Examples ===

def practical_examples() {
    print("\n=== Practical examples ===\n")

    // validate a phone pattern: 3 digits, dash, 4 digits
    var re_phone <- %regex~^\d{3}-\d{4}$%%
    print("555-1234 valid: {regex_match(re_phone, "555-1234") != -1}\n")
    // output: 555-1234 valid: true
    print("55-1234 valid: {regex_match(re_phone, "55-1234") != -1}\n")
    // output: 55-1234 valid: false

    // strip non-word characters from a string
    var re_strip <- %regex~[^\w]+%%
    let cleaned = regex_replace(re_strip, "he!l@l#o") $(match_str) {
        return ""
    }
    print("cleaned: {cleaned}\n")
    // output: cleaned: hello

    // extract email-like pattern with groups
    var re_email <- %regex~([\w.]+)@([\w.]+)%%
    let email = "user@example.com"
    let m = regex_match(re_email, email)
    if (m != -1) {
        print("user: {regex_group(re_email, 1, email)}\n")
        // output: user: user
        print("domain: {regex_group(re_email, 2, email)}\n")
        // output: domain: example.com
    }

    // IP-like pattern with counted quantifiers
    var re_ip <- %regex~\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}%%
    print("IP match: {regex_match(re_ip, "192.168.1.1")}\n")
    // output: IP match: 11
}

// === Case-Insensitive Matching ===
// Pass case_insensitive=true to regex_compile for ASCII case-insensitive matching.

def case_insensitive_matching() {
    print("\n=== Case-insensitive matching ===\n")

    // by default, matching is case-sensitive
    var re <- regex_compile("hello")
    print("sensitive 'HELLO': {regex_match(re, "HELLO")}\n")
    // output: sensitive 'HELLO': -1

    // case-insensitive matching
    var re_ci <- regex_compile("hello", [case_insensitive=true])
    print("insensitive 'HELLO': {regex_match(re_ci, "HELLO")}\n")
    // output: insensitive 'HELLO': 5
    print("insensitive 'HeLLo': {regex_match(re_ci, "HeLLo")}\n")
    // output: insensitive 'HeLLo': 5

    // character classes are also case-insensitive
    var re_set <- regex_compile("[a-z]+", [case_insensitive=true])
    print("insensitive [a-z]+: {regex_match(re_set, "AbCdE")}\n")
    // output: insensitive [a-z]+: 5

    // search is also case-insensitive
    var re_search <- regex_compile("world", [case_insensitive=true])
    let pos = regex_search(re_search, "Hello WORLD!")
    print("search WORLD: [{pos.x},{pos.y}]\n")
    // output: search WORLD: [6,11]
}

// === Dot and Newline Behavior ===
// By default, '.' matches any character EXCEPT newline (\n).
// Pass dot_all=true to regex_compile to make '.' match newlines too.

def dot_and_newline() {
    print("\n=== Dot and newline ===\n")

    // default: '.' does NOT match newline
    var re <- regex_compile(".+")
    print("default dot on 'ab\\nc': {regex_match(re, "ab\nc")}\n")
    // output: default dot on 'ab\nc': 2

    // dot_all=true: '.' also matches newline
    var re_all <- regex_compile(".+", [dot_all=true])
    print("dot_all on 'ab\\nc': {regex_match(re_all, "ab\nc")}\n")
    // output: dot_all on 'ab\nc': 4

    // useful for multi-line content extraction
    var re_multi <- regex_compile("START(.+?)END", [dot_all=true])
    let text = "START\nhello\nEND"
    let m = regex_match(re_multi, text)
    if (m != -1) {
        print("multi-line capture: '{regex_group(re_multi, 1, text)}'\n")
        // output: multi-line capture: '\nhello\n'
    }
}

// === Lookahead Assertions ===
// (?=...) positive lookahead — matches if ... follows, without consuming it
// (?!...) negative lookahead — matches if ... does NOT follow

def lookahead_assertions() {
    print("\n=== Lookahead assertions ===\n")

    // positive lookahead: match "foo" only if followed by "bar"
    var re_pos <- regex_compile("foo(?=bar)")
    print("foo(?=bar) on 'foobar': {regex_match(re_pos, "foobar")}\n")
    // output: foo(?=bar) on 'foobar': 3
    print("foo(?=bar) on 'foobaz': {regex_match(re_pos, "foobaz")}\n")
    // output: foo(?=bar) on 'foobaz': -1

    // the lookahead doesn't consume input — the match stops at "foo"
    var re_pos2 <- regex_compile("\\d+(?= dollars)")
    let text = "100 dollars"
    let pos = regex_search(re_pos2, text)
    print("amount: '{slice(text, pos.x, pos.y)}'\n")
    // output: amount: '100'

    // negative lookahead: match "foo" only if NOT followed by "bar"
    var re_neg <- regex_compile("foo(?!bar)")
    print("foo(?!bar) on 'foobar': {regex_match(re_neg, "foobar")}\n")
    // output: foo(?!bar) on 'foobar': -1
    print("foo(?!bar) on 'foobaz': {regex_match(re_neg, "foobaz")}\n")
    // output: foo(?!bar) on 'foobaz': 3

    // practical: match a word only if NOT followed by an exclamation mark
    var re_calm <- regex_compile("\\w(?!!)")
    print("calm on 'a!': {regex_match(re_calm, "a!")}\n")
    // output: calm on 'a!': -1
    print("calm on 'a.': {regex_match(re_calm, "a.")}\n")
    // output: calm on 'a.': 1
}

// === Template-String Replace ===
// regex_replace(re, str, replacement) replaces matches with a template string.
// $0 or $& = whole match, $1-$9 = numbered groups,
// ${name} = named groups, $$ = literal $

def template_replace() {
    print("\n=== Template-string replace ===\n")

    // swap first and last name using numbered groups
    var re_name <- regex_compile("(\\w+) (\\w+)")
    let swapped = regex_replace(re_name, "John Smith", "$2 $1")
    print("swapped: {swapped}\n")
    // output: swapped: Smith John

    // $0 or $& references the whole match
    var re_word <- regex_compile("\\w+")
    let wrapped = regex_replace(re_word, "hello world", "[$0]")
    print("wrapped: {wrapped}\n")
    // output: wrapped: [hello] [world]

    // named groups with ${name}
    var re_date <- regex_compile("(?P<month>\\d+)/(?P<day>\\d+)/(?P<year>\\d+)")
    let reformatted = regex_replace(re_date, "12/25/2024", "$\{year\}-$\{month\}-$\{day\}")
    print("reformatted: {reformatted}\n")
    // output: reformatted: 2024-12-25

    // $$ for literal dollar sign
    var re_num <- regex_compile("\\d+")
    let priced = regex_replace(re_num, "Price: 42", "$$$0.00")
    print("priced: {priced}\n")
    // output: priced: Price: $42.00
}

// === Reader Macro Flags ===
// Flags are appended after a second ~ in the reader macro:
//   %regex~pattern~i%%  — case-insensitive
//   %regex~pattern~s%%  — dot matches newline (dot_all)
//   %regex~pattern~is%% — both flags combined

def reader_macro_flags() {
    print("\n=== Reader macro flags ===\n")

    // case-insensitive via ~i flag
    var re_ci <- %regex~hello~i%%
    print("~i flag: {regex_match(re_ci, "HELLO")}\n")
    // output: ~i flag: 5

    // dot-all via ~s flag
    var re_s <- %regex~.+~s%%
    print("~s flag on 'ab\\nc': {regex_match(re_s, "ab\nc")}\n")
    // output: ~s flag on 'ab\nc': 4

    // both flags combined
    var re_is <- %regex~hello.+world~is%%
    print("~is flag: {regex_match(re_is, "Hello\nWorld")}\n")
    // output: ~is flag: 11
}

[export]
def main() {
    compiling_and_matching()
    character_classes()
    anchors()
    quantifiers()
    groups_and_alternation()
    character_sets()
    word_boundaries()
    foreach_and_replace()
    escaped_meta()
    reader_macro()
    search_split_matchall()
    non_capturing_groups()
    named_groups()
    lazy_quantifiers()
    practical_examples()
    case_insensitive_matching()
    dot_and_newline()
    lookahead_assertions()
    template_replace()
    reader_macro_flags()
}

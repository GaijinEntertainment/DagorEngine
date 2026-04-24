.. _tutorial_regex:

========================
Regular Expressions
========================

.. index::
    single: Tutorial; Regular Expressions
    single: Tutorial; Regex
    single: Tutorial; Pattern Matching (Regex)
    single: Tutorial; regex_boost

This tutorial covers ``daslib/regex`` and ``daslib/regex_boost`` — compiling,
matching, and replacing text with regular expressions in daslang.

``regex`` provides the core compiler, matcher, and iterator APIs.
``regex_boost`` adds the ``%regex~`` reader macro for compile-time patterns.

Compiling and matching
======================

``regex_compile`` creates a ``Regex`` from a pattern string.
``regex_match`` returns the end position of the match (from position 0),
or ``-1`` on failure::

  var re <- regex_compile("hello")
  let pos = regex_match(re, "hello world")   // 5
  let no  = regex_match(re, "goodbye")       // -1

An optional third argument specifies a starting offset::

  var re2 <- regex_compile("world")
  let pos2 = regex_match(re2, "hello world", 6)   // 11

Character classes
=================

Built-in shorthand classes match common character categories:

========  ================================
Escape    Meaning
========  ================================
``\w``    Word chars ``[a-zA-Z0-9_]``
``\W``    Non-word chars
``\d``    Digits ``[0-9]``
``\D``    Non-digits
``\s``    Whitespace (space, tab, newline, CR, form-feed, vertical-tab)
``\S``    Non-whitespace
``\t``    Tab
``\n``    Newline
``\r``    Carriage return
========  ================================

Example::

  var re_num <- regex_compile("\\d+")
  regex_match(re_num, "12345")    // 5

  var re_ws <- regex_compile("\\s+")
  regex_match(re_ws, "   x")     // 3

Anchors
=======

``^`` anchors the match to the beginning of the string (or offset position).
``$`` anchors to the end::

  var re_start <- regex_compile("^hello")
  regex_match(re_start, "hello")       // 5
  regex_match(re_start, "say hello")   // -1

  var re_full <- regex_compile("^abc$")
  regex_match(re_full, "abc")          // 3
  regex_match(re_full, "abcd")         // -1

Quantifiers
===========

==========  ====================================
Syntax      Meaning
==========  ====================================
``+``       One or more (greedy)
``*``       Zero or more (greedy)
``?``       Zero or one
``{n}``     Exactly *n* repetitions
``{n,}``    *n* or more repetitions (greedy)
``{n,m}``   Between *n* and *m* repetitions (greedy)
==========  ====================================

::

  var re_plus <- regex_compile("a+")
  regex_match(re_plus, "aaa")       // 3

  var re_q <- regex_compile("colou?r")
  regex_match(re_q, "color")        // 5
  regex_match(re_q, "colour")       // 6

Counted quantifiers use braces (escaped as ``\{`` in daslang strings)::

  var re_exact <- regex_compile("\\d\{4}")
  regex_match(re_exact, "1234")     // 4

  var re_range <- regex_compile("a\{2,4}")
  regex_match(re_range, "a")        // -1
  regex_match(re_range, "aaa")      // 3
  regex_match(re_range, "aaaaa")    // 4

Groups and alternation
======================

Parentheses create capturing groups. ``|`` separates alternatives::

  var re_alt <- regex_compile("cat|dog")
  regex_match(re_alt, "cat")    // 3
  regex_match(re_alt, "dog")    // 3

``regex_group`` retrieves group captures after a successful match::

  var re_grp <- regex_compile("(\\w+)@(\\w+)")
  let inp = "user@host"
  regex_match(re_grp, inp)                  // 9
  print("{regex_group(re_grp, 1, inp)}\n")  // user
  print("{regex_group(re_grp, 2, inp)}\n")  // host

Character sets
==============

Square brackets define a set of characters to match:

- ``[abc]`` — matches ``a``, ``b``, or ``c``
- ``[a-z]`` — matches a range
- ``[^abc]`` — negated set (matches anything NOT listed)
- ``[\d_]`` — shorthand classes work inside sets

::

  var re_vowel <- regex_compile("[aeiou]+")
  regex_match(re_vowel, "aeiou")    // 5

  var re_neg <- regex_compile("[^0-9]+")
  regex_match(re_neg, "abc")        // 3

Word boundaries
===============

``\b`` matches at a word boundary — the transition between ``\w`` and ``\W``
characters, or at the start/end of the string.
``\B`` matches at a non-boundary position::

  var re_bnd <- regex_compile("\\bhello\\b")
  regex_match(re_bnd, "hello")       // 5

  var re_nb <- regex_compile("\\Bell")
  regex_match(re_nb, "hello", 1)     // 4

Foreach and replace
===================

``regex_foreach`` iterates over all non-overlapping matches, passing each
match range (as ``int2``) to a block. Return ``true`` to continue::

  var re_num <- regex_compile("\\d+")
  regex_foreach(re_num, "a12b34c56") $(at) {
      print("[{at.x},{at.y}] ")   // [1,3] [4,6] [7,9]
      return true
  }

``regex_replace`` replaces every match using a block that receives the
matched substring and returns the replacement::

  let result = regex_replace(re_num, "a12b34c56") $(match_str) {
      return "X"
  }
  print("{result}\n")   // aXbXcX

Escaped metacharacters
======================

Backslash escapes literal metacharacters: ``\.`` ``\+`` ``\*`` ``\(`` ``\)``
``\[`` ``\]`` ``\|`` ``\\`` ``\^`` ``\{`` ``\}``::

  var re_dot <- regex_compile("\\d+\\.\\d+")
  regex_match(re_dot, "3.14")       // 4

  var re_parens <- regex_compile("\\(\\w+\\)")
  regex_match(re_parens, "(hello)")   // 7

Hex escapes
===========

``\xHH`` matches a character by its hexadecimal code::

  var re_hex <- regex_compile("\\x41")
  regex_match(re_hex, "A")    // 1  (0x41 = 'A')

Reader macro
============

``regex_boost`` provides the ``%regex~`` reader macro which compiles
a pattern at compile time. No double-escaping is needed — backslashes
are literal in the macro body::

  require daslib/regex_boost

  var re <- %regex~\d+%%
  regex_match(re, "42abc")    // 2

  var re2 <- %regex~[a-z]+%%
  regex_match(re2, "hello")   // 5

Search, split, match_all
========================

``regex_search`` finds the first match anywhere in the string (unlike
``regex_match`` which only matches at position 0). Returns ``int2(start, end)``
or ``int2(-1, -1)``::

  var re_num <- regex_compile("\\d+")
  let pos = regex_search(re_num, "abc 123 def")   // int2(4, 7)

``regex_split`` splits a string by pattern matches::

  var re_comma <- regex_compile(",\\s*")
  var parts <- regex_split(re_comma, "a, b,c, d")
  // parts == ["a", "b", "c", "d"]

``regex_match_all`` collects all match ranges::

  var re_word <- regex_compile("\\w+")
  var matches <- regex_match_all(re_word, "foo bar baz")
  // length(matches) == 3

Non-capturing groups
====================

``(?:...)`` groups without creating a capture. Useful for applying
quantifiers or alternation without increasing the group count::

  var re <- regex_compile("(?:cat|dog)fish")
  regex_match(re, "catfish")     // 7
  length(re.groups)              // 1 (only group 0)

  var re2 <- regex_compile("(?:ab)\{3}")
  regex_match(re2, "ababab")     // 6

Named groups
============

``(?P<name>...)`` creates a named capturing group, accessible via
``regex_group_by_name``::

  var re <- regex_compile("(?P<user>\\w+)@(?P<host>\\w+)")
  let email = "alice@example"
  regex_match(re, email)
  regex_group_by_name(re, "user", email)   // "alice"
  regex_group_by_name(re, "host", email)   // "example"

Named groups are also accessible by their numeric index (1, 2, ...)
via ``regex_group``.

Lazy quantifiers
================

Greedy quantifiers (``+``, ``*``, ``?``, ``{n,m}``) match as much as possible.
Appending ``?`` makes them lazy — matching as little as possible:

============  ==========  ====================================
Greedy        Lazy        Meaning
============  ==========  ====================================
``+``         ``+?``      One or more (prefer fewer)
``*``         ``*?``      Zero or more (prefer fewer)
``?``         ``??``      Zero or one (prefer zero)
``{n,m}``     ``{n,m}?``  Counted (prefer *n*)
============  ==========  ====================================

::

  // lazy +? takes the shortest match
  var re <- regex_compile("<.+?>")
  let pos = regex_search(re, "<b>bold</b>")
  // pos == int2(0, 3) — matches "<b>" not "<b>bold</b>"

  // greedy vs lazy at end of pattern
  regex_match(regex_compile("a+"),  "aaa")   // 3 (greedy: all)
  regex_match(regex_compile("a+?"), "aaa")   // 1 (lazy: one)

Practical examples
==================

The ``%regex~`` reader macro avoids double-escaping, making real-world
patterns much more readable.

Phone number validation::

  var re_phone <- %regex~^\d{3}-\d{4}$%%
  regex_match(re_phone, "555-1234") != -1   // true
  regex_match(re_phone, "55-1234")  != -1   // false

Strip non-word characters::

  var re_strip <- %regex~[^\w]+%%
  let cleaned = regex_replace(re_strip, "he!l@l#o") $(m) {
      return ""
  }
  // cleaned == "hello"

Extract email parts::

  var re_email <- %regex~([\w.]+)@([\w.]+)%%
  let email = "user@example.com"
  regex_match(re_email, email)
  regex_group(re_email, 1, email)   // "user"
  regex_group(re_email, 2, email)   // "example.com"

IP address pattern::

  var re_ip <- %regex~\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}%%
  regex_match(re_ip, "192.168.1.1")   // 11

Case-insensitive matching
=========================

Pass ``case_insensitive=true`` to ``regex_compile`` for ASCII case-insensitive
matching. Character classes and sets are also affected::

  // default: case-sensitive
  var re <- regex_compile("hello")
  regex_match(re, "HELLO")    // -1

  // case-insensitive
  var re_ci <- regex_compile("hello", [case_insensitive=true])
  regex_match(re_ci, "HELLO")    // 5
  regex_match(re_ci, "HeLLo")   // 5

  // character sets are also case-insensitive
  var re_set <- regex_compile("[a-z]+", [case_insensitive=true])
  regex_match(re_set, "AbCdE")   // 5

Dot and newline
===============

By default, ``.`` matches any character **except** newline (``\n``).
Pass ``dot_all=true`` to ``regex_compile`` to make ``.`` match newlines too::

  // default: '.' does NOT match newline
  var re <- regex_compile(".+")
  regex_match(re, "ab\nc")     // 2

  // dot_all=true: '.' also matches newline
  var re_all <- regex_compile(".+", [dot_all=true])
  regex_match(re_all, "ab\nc")   // 4

This is useful for multi-line content extraction::

  var re <- regex_compile("START(.+?)END", [dot_all=true])
  let text = "START\nhello\nEND"
  regex_match(re, text)
  regex_group(re, 1, text)   // "\nhello\n"

Lookahead assertions
====================

Lookahead assertions check what follows the current position without consuming
any input.

``(?=...)`` is a **positive lookahead** — the overall match succeeds only if
the lookahead pattern matches::

  // "foo" only if followed by "bar"
  var re <- regex_compile("foo(?=bar)")
  regex_match(re, "foobar")   // 3 (matches "foo", not "foobar")
  regex_match(re, "foobaz")   // -1

  // extract digits before " dollars"
  var re2 <- regex_compile("\\d+(?= dollars)")
  let pos = regex_search(re2, "100 dollars")
  // pos == int2(0, 3) — matches "100"

``(?!...)`` is a **negative lookahead** — the match succeeds only if the
lookahead pattern does NOT match::

  // "foo" only if NOT followed by "bar"
  var re <- regex_compile("foo(?!bar)")
  regex_match(re, "foobar")   // -1
  regex_match(re, "foobaz")   // 3

  // single char NOT followed by "!"
  var re2 <- regex_compile("\\w(?!!)")
  regex_match(re2, "a!")   // -1
  regex_match(re2, "a.")   // 1

Template-string replace
=======================

``regex_replace`` also accepts a replacement template string instead of a
block. The template supports group references:

=============  ====================================
Reference      Meaning
=============  ====================================
``$0``         Whole match
``$&``         Whole match (alternative syntax)
``$1``–``$9``  Numbered capturing groups
``${name}``    Named capturing group
``$$``         Literal ``$`` character
=============  ====================================

::

  // swap first and last name
  var re <- regex_compile("(\\w+) (\\w+)")
  regex_replace(re, "John Smith", "$2 $1")     // "Smith John"

  // wrap each word in brackets
  var re2 <- regex_compile("\\w+")
  regex_replace(re2, "hello world", "[$0]")    // "[hello] [world]"

Named group references use ``${name}`` syntax::

  var re <- regex_compile("(?P<m>\\d+)/(?P<d>\\d+)/(?P<y>\\d+)")
  regex_replace(re, "12/25/2024", "${y}-${m}-${d}")   // "2024-12-25"

Reader macro flags
==================

Flags can be appended after a second ``~`` in the ``%regex~`` reader macro:

=========================  ====================================
Syntax                     Effect
=========================  ====================================
``%regex~pattern~i%%``     Case-insensitive matching
``%regex~pattern~s%%``     Dot-all mode (``.`` matches ``\n``)
``%regex~pattern~is%%``    Both flags combined
=========================  ====================================

::

  var re_ci <- %regex~hello~i%%
  regex_match(re_ci, "HELLO")   // 5

  var re_s <- %regex~.+~s%%
  regex_match(re_s, "ab\nc")    // 4

  var re_is <- %regex~hello.+world~is%%
  regex_match(re_is, "Hello\nWorld")   // 11

.. seealso::

   Full source: :download:`tutorials/language/31_regex.das <../../../../tutorials/language/31_regex.das>`

   :ref:`JSON tutorial <tutorial_json>` (previous tutorial).

   Next tutorial: :ref:`Operator overloading <tutorial_operator_overloading>`.

   :doc:`/stdlib/generated/regex` — core regex module reference.

   :doc:`/stdlib/generated/regex_boost` — regex boost module reference.

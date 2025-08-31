# Utility to convert syntax from v1 to v2
## Key differences
First, you may want to check this [article about initialization](https://borisbat.github.io/dascf-blog/2024/07/23/data-initialization/). It describes most initialization expressions in both the old and new syntax.
However, some corner cases are missing, such as: 
```
// old
var a = new [[Foo#()]]
// will be converted to new
var a = new struct<Foo#>()
```

## How it works
Formatter tools generally operate at either token-level or the AST-level. 
AST-level looks much easier to maintain, so I decided to use it.

There's a copy of bison grammar corresponding to the v1 syntax. For required non-terminals I edited action on match.

We maintain the `last_printed` position and once a rule that
should be replaced is matched
- Everything from `last_printed` up to this rule
is written directly to the `output` 
- This rule is modified, and the position is updated.

## Usage:
```
./bin/das-fmt file1.das file2.das -i -v2
```
There are two command line arguments (excluding input files):

- The first determines whether the conversion should be done in-place (`-i`)
or printed to stdout(`-d`). 
  * If converter fails in first case it will do nothing.
  * In second case it will print a partially converted file to stdout, which will not compile, 
but can be fixed manually
- The second option is `-v1` or `-v2`, specifying whether to convert to gen1.5 syntax or to gen2 syntax.
This option is not required; by default files will be converted to gen1.5

I used this command to transform all `.das` files syntax in current folder in-place:
```
find . -name "*.das" | tr '\n' ' ' | xargs ./bin/das-fmt -i
```
Afterward, I suggest running it again on failed files individually:
```
./bin/das-fmt <filename> -d
```
This will at least partially format then, allowing you to manually edit the remaining parts.

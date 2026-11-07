# sqPacked: packed immutable data for Quirrel

The native Quirrel module `packed` stores a big tree of tables and arrays in
one immutable buffer, gives read access to it through views, and runs
queries on it.

Use it for big data that scripts read and do not change: a player profile,
a config made from a DataBlock, a list of unlocks or offers. A sample
profile of 11.6 MB JSON takes 27 MB as Quirrel tables and 1.61 MB packed.

| operation (csq dev build)          | Quirrel tables | packed            |
|------------------------------------|----------------|-------------------|
| random path of depth 3.4           | 175 ns         | 380 ns (view)     |
| the same path, `packed.get()`      |                | 120-150 ns        |
| find records by a condition        | 70-600 us loop | 0.5-6 us query    |

Do not use it for data that changes often: each change needs a new
`pack()` of all the data.

## Contents

- [Setup](#setup)
- [Quick start](#quick-start)
- [Module functions](#module-functions)
- [pack()](#pack)
- [Views](#views)
- [Storage layout](#storage-layout)
- [Queries](#queries)
- [Tutorial: from Quirrel loops to queries](#tutorial-from-quirrel-loops-to-queries)
- [Query language](#query-language)
- [How values compare](#how-values-compare)
- [Indexes and plans](#indexes-and-plans)
- [Errors](#errors)
- [Limits](#limits)
- [Tests](#tests)

## Setup

The library is `gameLibs/quirrel/sqPacked`. The public header is
`sqPacked_api.h` in this folder (the `_api.h` convention, see
`prog/gameLibs/CLAUDE.md`).

jamfile of the program:

```
UseProgLibs += gameLibs/quirrel/sqPacked ;
AddIncludes += $(Root)/prog/gameLibs ;
```

C++:

```cpp
#include <quirrel/sqPacked/sqPacked_api.h>

bindquirrel::register_packed_module(module_mgr); // adds the "packed" module

// pushes a view of a DataBlock on the stack of vm, with no Quirrel tables
// on the way; register_packed_module() must be called first for this VM
if (SQ_FAILED(bindquirrel::push_packed_datablock(vm, blk)))
  ...; // the error is thrown on vm
```

What the VM must have bound:

- `pack(blk)` in a script needs the `DataBlock` class (Sqrat).
- A read of a Point2/3/4, IPoint2/3/4, E3DCOLOR or TMatrix value needs the
  `dagor.math` classes. With no binding, a read of such a value throws
  "class Point3 of a packed value is not bound in this VM"; the other
  values, and queries on the components, still work.

csq registers the module (`register_packed`).

## Quick start

```squirrel
import "packed" as packed

let profile = packed.pack(loadedProfileTable)    // a view of the root

let name = profile.units[3].name                 // read like a table
let hp = profile?.units[3]?.hp                   // ?. and ?[] work
let same = packed.get(profile, "units", 3, "hp") // faster path read

foreach (i, unit in profile.units)               // foreach works
  println($"{i}: {unit.name}")

// compile a query once, run it many times
let unitsOfCountry = packed.query("units[*] ? country == $c order by rank")
let list = unitsOfCountry.all(profile, { c = "usa" })   // views of records
let best = unitsOfCountry.first(profile, { c = "usa" }) // or null
```

## Module functions

| function                                 | result                                     |
|------------------------------------------|--------------------------------------------|
| `pack(data, [available_classes])`        | a view of the root of a new buffer         |
| `unpack(view)`                           | plain tables and arrays made from a view   |
| `get(view, key1, key2, ...)`             | the value on a path of keys, or null       |
| `len(view)`                              | the number of keys or elements of a view   |
| `is_view(x)`                             | true when x is a packed view               |
| `stats(view)`                            | a table with the sizes of the buffer parts |
| `query(text)`                            | a compiled query                           |
| `save(view)`                             | a token of a copy of the buffer            |
| `load(token)`                            | a view of the buffer of the token, once    |

`get()` with no keys gives the view. A key that is missing, or a step into a
scalar, gives null. A key can also be a component of a DataBlock type:
`packed.get(view, "tm", 3, "y")`.

`len()` of a record counts only the fields that the record has.

`unpack()` of a child view gives only that part of the data.

`save(view)` keeps a copy of the whole buffer of the view, and the place of
the view in it, in the memory of the process, and returns an integer token.
`load(token)` gives the same view of that buffer, also in another VM, and
frees the token: a hard reload of the scripts keeps a packed value in nestdb
this way, with no tables made on the way. The bytes never pass through a
script, so `load()` has nothing to check but the token. The query indexes
are not saved; the first query on the loaded view makes them again.

- A token loads once. A buffer that is saved and never loaded stays in
  memory until the process ends.
- `save()` of a buffer with class instances is an error: their classes are
  objects of one VM.

`stats(view)` fields, all in bytes but the counts:

| field              | meaning                                         |
|--------------------|-------------------------------------------------|
| `total`            | size of the buffer                              |
| `columns`          | the column data                                 |
| `strings`          | the string data and offsets                     |
| `stringCount`      | number of different strings                     |
| `keyIndex`         | the hash index of the strings                   |
| `views`            | number of live views of the buffer              |
| `indexes`          | number of query indexes made on this buffer     |
| `indexBytes`       | memory of the query indexes                     |
| `cachedStrings`    | number of strings of the buffer kept as VM strings |
| `stringCacheBytes` | memory of these VM strings and of their slots   |

A buffer keeps the VM strings of its last string reads, up to 2048 of them,
so a string that is read again and again (a field name in a loop) is made
once.

## pack()

`pack(data, [available_classes])` takes a table, an array or a DataBlock.

Values that it can store:

- null, bool, integer, float, string;
- table, array;
- Point2, Point3, Point4, IPoint2, IPoint3, IPoint4, E3DCOLOR, TMatrix
  (native: no `available_classes` needed);
- an instance of a class in `available_classes` (see below).

Table keys can be strings, integers, floats and bools. A key matches only
a key of the same type, as in a Quirrel table: the key `1` and the key
`1.0` are two keys, and `0.0` and `-0.0` are two keys too.

Other values (functions, classes, userdata, weak references) make an
error.

### Instances of script classes

```squirrel
let view = packed.pack(data, { Vec, Tagged }) // available_classes
```

Instances are stored as `writeobject` stores them:

- `pack()` calls `__getstate()` and packs its result;
- a read calls the class with no arguments, and then `__setstate(state)`;
- `__getstate` and `__setstate` get `available_classes` as one more
  argument when they take it.

Each read of an instance makes a new instance. The class must have
`__getstate` and `__setstate`, and its constructor must work with no
arguments.

### DataBlock

`pack(blk)` in a script and `push_packed_datablock()` in C++ read a
DataBlock directly, with no Quirrel tables on the way (2.5 times faster
than `convertBlk()` and then `pack()`). The result has the layout of
`convertBlk()` from `sqstd/datablock.nut`:

- a block is a table;
- a name used once gives its value;
- a name used more than once gives an array: the blocks of that name
  first, then the params;
- a param keeps its DataBlock type: an E3DCOLOR param stays an E3DCOLOR
  (`getParamValue()` gives a Color4), an int64 param is an integer.

```squirrel
let cfg = packed.pack(blk)
let x = cfg.unit[1].pos.x            // Point3 component
let tm = cfg.tm                      // TMatrix
let q = packed.query("unit[*] ? pos.y > 0 order by hp desc")
```

## Views

A view is a read-only instance that refers to one table or array in a
buffer. A read of a table or an array in a view gives a new view; a read
of a scalar gives the value.

What works on a view:

| operation              | as on a table / array                        |
|------------------------|----------------------------------------------|
| `v.key`, `v[key]`      | a missing key throws "does not exist"        |
| `v?.key`, `v?[key]`    | a missing key gives null                     |
| `key in v`             | true when the key is there (also a null)     |
| `foreach (k, x in v)`  | arrays in index order, tables in key order   |
| `clone v`              | a new view of the same data                  |
| `v.len()`              | the number of keys or elements               |
| `v.map(fn)`, ...       | the methods that read, see below             |

Methods that read work as on a table or an array:

- a table view: `keys`, `values`, `map`, `filter`, `reduce`, `each`,
  `findindex`, `findvalue`, `topairs`, `hasindex`, `hasvalue`, `rawget`,
  `rawin`, `__merge`;
- an array view: `map`, `filter`, `reduce`, `each`, `indexof`, `contains`,
  `hasvalue`, `hasindex`, `findindex`, `findvalue`, `totable`, `slice`,
  `top`;
- both: `len`.

A key of the data comes first, as in a table: in a record with the field
`len`, `v.len` is that field. A method gets the arguments that it gets on
a table or an array, and the results are plain tables and arrays. A table
or an array in them is a view, as on a read of a field.

`len`, `hasindex`, `rawin`, `hasvalue`, `indexof` and `contains` read the
view and make no copy. The search methods compare scalars as an array does
(`1 == 1.0`). A table, an array, a class instance or a native value in the
view is a new object on each read, so they never find one. `__merge` makes
one plain copy of one level of the view and adds the arguments to it.

Each call of another method makes a plain copy of one level of the view
first. Thus `findvalue()` costs a copy of all the elements, also when the
first element matches. In a hot path, use a query.

What does not work:

- writes and the methods that write (`append`, `sort`, `__update`,
  `rawset`, ...): the data is immutable, and these methods throw
  "packed view is read-only";
- the container argument of a callback (`@(v, k, container)`) is the
  plain copy, not the view;
- `==` between views compares the instances, not the data;
- `type(v)` is "instance", not "table" or "array", and `v.tostring()`
  names an instance.

The other builtin methods answer as for an immutable instance:
`v.is_frozen()` is true, `clone v` gives a view of the same data, and
`v.weakref()` refers to the view.

The key order of a table: by the type of the key (bool, integer, float,
string), then by value; strings compare by their bytes.

A view keeps its buffer alive, as a child DataBlock does: a child view
works after the root view is gone. The buffer is freed when its last view
is gone.

A buffer and its views belong to one VM. Do not give them to another VM or
use them from another thread.

## Storage layout

The buffer is made once and does not change. It holds:

- one string table: each different string once, sorted, with a hash
  index; keys and values refer to it by number;
- columns: the records of one collection keep each field in a column, so
  a field of 1000 records is one array of 1000 values;
- a column with values of several types has a child column for each
  type;
- a column with one value in all rows is stored as that value only.

The tables in one place of the tree (for example, all the records of one
array) are packed together as rows. Which rows become columns:

- Rows with up to 64 different fields, which have half of the fields or
  more on average, keep each field in a column.
- In other rows with string keys, a field in 16 rows or more gets a column
  when its presence bits cost less than map entries. The other fields go
  to a small map beside the columns. A view merges both in key order.
- Rows with other keys, or with no field that gets a column, are maps.

Examples (csq dev): wpcost air 2088 KB, unlocks 804 KB, a mission 335 KB.
100k records with a Point3 and an E3DCOLOR pack in 38 ms to 1.3 MB; the
same values as script classes through `__getstate` take 104 ms and 2.8 MB.

## Queries

`packed.query(text)` compiles a query. A compiled query does not refer to
a buffer: keep it in a module variable and run it on any view.

Each method takes `(view, [params], [plan])`:

| method       | result                                                     |
|--------------|------------------------------------------------------------|
| `all()`      | array of all results                                       |
| `first()`    | the first result, or null                                  |
| `keys()`     | array of the keys of the matching records                  |
| `firstKey()` | the key of the first matching record, or null              |
| `count()`    | number of results                                          |
| `any()`      | true when a record matches; does not make views            |
| `value()`    | the aggregate of `select min/max/sum/avg/count`            |

- A result is a record, or the selected value when the query has `select`.
  A table or an array comes as a view.
- `keys()` and `firstKey()` do not take `select`; use `select @key` with
  `all()` for the same result.
- `any()` ignores `order by` and `select`.
- `value()` is the only method for a query with an aggregate, and works
  only for it.
- `params` is a table of the `$name` parameters of the query. A query with
  parameters needs it; a missing parameter is an error.
- `plan` is `"auto"` (the default), `"columns"` or `"rows"`, see
  [Indexes and plans](#indexes-and-plans).

The view can be the root or any child view. The source path starts from it.

```squirrel
let q = packed.query("items[*] ? kind == $k && level >= $min order by level desc limit 10")
let top = q.all(profile, { k = "tank", min = 3 })
let hasAny = q.any(profile, { k = "ship", min = 0 })
```

## Tutorial: from Quirrel loops to queries

Each step shows a Quirrel loop on plain tables and the query that gives the
same result on the packed data. The steps start simple and become more
complex. The data:

```squirrel
let data = {
  units = [
    { name = "u0", country = "usa", rank = 3, type = "air", premium = true,
      modes = [{ type = "air", num = 12 }, { type = "tank", num = 40 }] }
    { name = "u1", country = "ussr", rank = 7, type = "tank", parent = "u0",
      modes = [] }
    ...
  ]
  byName = { u0 = { rank = 3, hidden = true }, u1 = { rank = 7, hidden = false }, ... }
}
let units = data.units
let profile = packed.pack(data)
```

### 1. Find one record

```squirrel
function findUnit(name) {
  foreach (u in units)
    if (u.name == name)
      return u
  return null
}
```

```squirrel
let qUnitByName = packed.query("units[*] ? name == $name")
let u = qUnitByName.first(profile, { name = "u17" })   // a view or null
```

- `units[*]` is the collection, each element is a record. `? ...` is the
  condition.
- `$name` is a parameter. Do not put a value into the query text: compile
  the query once (a module `let`) and give the values in `params`. A
  compiled query also works on other buffers.
- The first run makes an index of `name`. The next runs find the record
  with no scan.

### 2. Filter a list

```squirrel
let list = units.filter(@(u) u.country == c && u.rank >= r)
```

```squirrel
let list = packed.query("units[*] ? country == $c && rank >= $r").all(profile, { c, r })
```

The records come as views, in the order of the collection. Use
`packed.unpack()` only when you must have plain tables (for example, to
change them).

### 3. Count, or tell if there is one

```squirrel
let ships = units.filter(@(u) u.type == "ship").len()
let has = units.findindex(@(u) u.rank == 7 && u.country == "japan") != null
```

```squirrel
let ships = packed.query("units[*] ? type == 'ship'").count(profile)
let has = packed.query("units[*] ? rank == 7 && country == 'japan'").any(profile)
```

`count()` and `any()` make no views. `any()` stops at the first match.

### 4. Get one field of each record

```squirrel
let names = units.filter(@(u) u.country == "ussr").map(@(u) u.name)
```

```squirrel
let names = packed.query("units[*] ? country == 'ussr' select name").all(profile)
```

`select` gives the value of the path instead of the record. When the
record does not have the field, the value is null.

### 5. Indexes of the records

```squirrel
let i = units.findindex(@(u) u.rank == 3)
let all = []
foreach (i, u in units)
  if (u.rank == 3)
    all.append(i)
```

```squirrel
let i = packed.query("units[*] ? rank == 3").firstKey(profile)
let all = packed.query("units[*] ? rank == 3").keys(profile)
```

For a table collection, `keys()` gives the keys (see step 15).

### 6. Fields that some records do not have

```squirrel
units.filter(@(u) u?.premium)                  // premium is true
units.filter(@(u) !(u?.premium ?? false))      // no premium, or false
units.filter(@(u) u?.parent == null)           // no parent
```

```squirrel
packed.query("units[*] ? premium")
packed.query("units[*] ? !premium")
packed.query("units[*] ? parent == null")
```

A path does not need `?.`: a missing field is null. A bare path is true
when Quirrel takes the value as true: null, false, 0 and 0.0 are false.

### 7. A value from a list

```squirrel
let wanted = ["air", "ship"]
units.filter(@(u) wanted.contains(u.type)).map(@(u) u.name)
```

```squirrel
packed.query("units[*] ? type in $types select name").all(profile, { types = wanted })
```

`in` takes an array parameter or a literal list: `type in ['air', 'ship']`.
It uses the index too: one look-up for each value of the set.

### 8. Sort and take the top N

```squirrel
let list = units.filter(@(u) u.country == "usa")
list.sort(@(a, b) b.rank <=> a.rank)
let top = list.slice(0, 5).map(@(u) u.name)
```

```squirrel
packed.query("units[*] ? country == 'usa' order by rank desc limit 5 select name").all(profile)
```

- `limit` keeps the records: a partial sort finds them, the list is not
  sorted all.
- The sort keeps the collection order of equal values. `array.sort()` in
  Quirrel does not promise it, so a loop with ties can give another order.
- Records with no value of the order path go last, also with `desc`. In
  a loop, `null <=> 5` is -1, so the records with null go first with
  `a <=> b` and last with `b <=> a`.

### 9. The smallest or the largest

```squirrel
local best = null
foreach (u in units)
  if (u.country == "japan" && (best == null || u.rank > best.rank))
    best = u
```

```squirrel
// the record
let best = packed.query("units[*] ? country == 'japan' order by rank desc").first(profile)
// only the value
let maxRank = packed.query("units[*] ? country == 'japan' select max(rank)").value(profile)
```

`first()` with `order by` does not sort all the records: it finds only the
first one. An aggregate (`min`, `max`, `sum`, `avg`, `count`) needs `value()`.

### 10. A condition on an array in the record

```squirrel
units.filter(@(u) u.modes.findindex(@(m) m.num > 45) != null)
```

```squirrel
packed.query("units[*] ? modes[*].num > 45")
```

`modes[*].num` gives the `num` of each mode. The condition is true when one
of them makes it true. A record with no modes has no values, so the
condition is false for it.

### 11. Two conditions on the same element

```squirrel
units.filter(@(u) u.modes.findindex(@(m) m.type == t && m.num > n) != null)
```

```squirrel
packed.query("units[*] ? modes[? type == $t && num > $n]")
```

Do not write this as `modes[*].type == $t && modes[*].num > $n`: each
`[*]` is a separate search, so that condition is true also when one mode
has the type and another mode has the num. On the test data, the wrong
form finds 25 records and the right form finds 11.

`[? cond]` is a filter step: it keeps the elements for which all of `cond`
is true. A path can go on after it: `modes[? type == $t].num > $n` is the
same condition.

### 12. Sort by a value in an array

The record that has the mode of type `t` with the smallest `num`:

```squirrel
local bestNum = null
local bestUnit = null
foreach (u in units)
  foreach (m in u.modes)
    if (m.type == t && (bestNum == null || m.num < bestNum)) {
      bestNum = m.num
      bestUnit = u
    }
```

```squirrel
let qNext = packed.query("units[*] ? modes[? type == $t] order by modes[? type == $t].num")
let bestUnit = qNext.first(profile, { t })
let bestNum = packed.query("units[*] select min(modes[? type == $t].num)").value(profile, { t })
```

When the order path gives many values, a record sorts by its smallest one
(`asc`) or its largest one (`desc`). The same filter step in the condition
and in `order by` sorts by the right modes only.

### 13. Values with no repeats

```squirrel
let seen = {}
let types = []
foreach (u in units)
  foreach (m in u.modes)
    if (m.type not in seen) {
      seen[m.type] <- true
      types.append(m.type)
    }
```

```squirrel
let types = packed.query("units[*] select distinct modes[*].type").all(profile)
```

The values come in the order in which they are found first, as in the loop.

### 14. Group and count

```squirrel
let byCountry = {}
foreach (u in units)
  byCountry[u.country] <- (byCountry?[u.country] ?? 0) + 1

let maxRank = {}
foreach (u in units)
  if (u.country not in maxRank || u.rank > maxRank[u.country])
    maxRank[u.country] <- u.rank
```

```squirrel
let byCountry = packed.query("units[*] group by country select count()").value(profile)
let maxRank = packed.query("units[*] group by country select max(rank)").value(profile)
```

The result is a plain table: one key for each value of the group path. A
condition goes before `group by`: `units[*] ? premium group by country
select count()`.

### 15. A table as the collection

```squirrel
let names = []
foreach (k, v in data.byName)
  if (v.hidden)
    names.append(k)
names.sort()
```

```squirrel
let names = packed.query("byName[*] ? hidden").keys(profile)
let same = packed.query("byName[*] ? hidden select @key").all(profile)
let rank = packed.query("byName[*] ? @key == $n select rank").first(profile, { n = "u5" })
```

`[*]` on a table makes each value a record. `@key` is the key of the
record. The records of a table come in key order, so the result is sorted
with no `sort()`. A Quirrel `foreach` on a table has no fixed order.

### 16. A query on a child view

A view of any part of the data can be the root of a query. The source
`[*]` is the view itself:

```squirrel
let unitsView = profile.units
let high = packed.query("[*] ? rank > 5").count(unitsView)
```

### What a query cannot do

The query language has no arithmetic, no function calls and no string
operations (`startswith`, `indexof`, ...). When a condition needs them, use
a query for the part that it can do, and a loop on its result for the
rest:

```squirrel
foreach (u in packed.query("units[*] ? country == $c").all(profile, { c }))
  if (u.name.startswith("p_"))
    ...
```

## Query language

```
query     = source [ "?" condition ]
            [ "group by" path ]
            [ "order by" path [ "asc" | "desc" ] ]
            [ "limit" N ]
            [ "select" ( path | "distinct" path | agg "(" [ path ] ")" ) ]
source    = path that ends with "[*]"
agg       = "count" | "min" | "max" | "sum" | "avg"
condition = or
or        = and { "||" and }
and       = unary { "&&" unary }
unary     = "!" unary | "(" or ")" | operand [ cmp operand | "in" set ]
cmp       = "==" | "!=" | "<" | "<=" | ">" | ">="
set       = "[" literal { [","] literal } "]" | "$" name
operand   = path | literal | "$" name
literal   = 'string' | "string" | integer | float | true | false | null
```

The parts must come in this order. Spaces and new lines between tokens
are optional. Each part is described below.

### Source

The source path selects the collection: a table or an array, and its last
step must be `[*]`. Each element of the collection is a record.

| source               | collection                                   |
|----------------------|----------------------------------------------|
| `[*]`                | the view itself                              |
| `items[*]`           | the field `items`                            |
| `deep.inner.list[*]` | a path of fields                             |
| `['with space'][*]`  | a field name that is not an identifier       |
| `list[2].sub[*]`     | an index step                                |

Only the last step of the source can be `[*]`. A source that is missing,
or is not a table or an array, gives no records.

### Paths

A path in a condition, `order by`, `group by` or `select` starts from the
record.

| step          | meaning                                                  |
|---------------|----------------------------------------------------------|
| `name`        | a field                                                  |
| `.name`       | a field of the value before                              |
| `['a.b']`     | a field with any name (`'` or `"`)                       |
| `[3]`         | an array index or an integer key                         |
| `[*]`         | each element of an array or a table                      |
| `[? cond]`    | each element for which `cond` is true                    |
| `@`           | the record itself (for collections of scalars)           |
| `@key`        | the key of the record in the collection                  |
| `.x` `.r` ... | a component of a point (`x y z w`) or E3DCOLOR (`r g b a`) |
| `tm[3]`       | a column of a TMatrix (a Point3), then `.x` etc.         |

`a.b` and `['a.b']` are different paths: two fields, and one field with a
dot in its name.

A path with no `[*]` and no `[? ]` gives one value. When a step is missing
it gives null.

A path with `[*]` or `[? ]` can give many values, or none. A condition on
it is true when one of the values makes it true:

```
items[*] ? modes[*].num == 12        // a record with a mode of num 12
items[*] ? !modes[? type == 'y']     // a record with no mode of type 'y'
```

### Filter step

`[? cond]` binds the conditions to one element. Compare:

```
items[*] ? modes[*].type == 'x' && modes[*].num > 30
           // one mode of type 'x' and one mode (maybe other) with num > 30
items[*] ? modes[? type == 'x' && num > 30]
           // one mode with both
items[*] ? modes[? type == 'x'].num > 30
           // the same, written with the value after the filter
```

Inside `[? ]`, a path starts from the element, and `@key` is the key of the
element. Filter steps can be nested: `tree[*] ? groups[? tags[? c == 2]]`.

### Conditions

- `path` alone is true when Quirrel takes the value as true: null,
  false, `0` and `0.0` are false; a string (also empty), a table, an array
  and an instance are true.
- `a cmp b` compares two operands. Either side can be the path:
  `'c' < name` is the same as `name > 'c'`.
- `path in [1, 'a', null]` and `path in $array` are true when the value is
  equal to one item of the set. The commas in a list are optional.
- `&&` binds tighter than `||`. Use `( )` to change it.
- `!` applies to the next operand with its comparison: `!id == 'b'` is
  `!(id == 'b')`.

A string literal ends at its quote. A backslash makes the next character
part of the string (`'it\'s'`); there are no escape codes like `\n`.

A parameter `$name` gets its value from the `params` table. A bool, an
integer, a float or a string is used as it is; any other value is used as
null. The parameter after `in` must be an array.

### order by

`order by path [asc|desc]` sorts the records by the value of the path.

- Records with no value (a missing field or a null) go last, also with
  `desc`.
- Records with equal values keep the collection order.
- When the path gives many values (`[*]`), a record sorts by its smallest
  value with `asc`, by its largest with `desc`.
- Values of different types sort by type: bool, numbers, strings,
  tables, arrays, instances, then the DataBlock types. Numbers sort
  together: `5` and `5.0` are equal.
- `order by @key` sorts by the keys; `order by @` sorts a collection of
  scalars by its values.

With `order by`, `first()` gives the record with the smallest value (or
the largest with `desc`).

### limit

`limit N` keeps the first N records, after `order by`. It limits records,
not the values that `select` gives. With `order by`, a partial sort finds
the N records.

### select

| select                     | result for each record                     |
|----------------------------|--------------------------------------------|
| `select path`              | the value of the path (null when missing)  |
| `select modes[*].num`      | each value of the path (none when missing) |
| `select distinct path`     | each value once over all records           |
| `select @key`              | the key of the record                      |

`distinct` takes equal scalars once (`5` and `5.0` are one value, the
first one stays). Tables, arrays and instances are never equal to each
other, so each one stays.

### Aggregates and group by

`select agg(path)` gives one value through `value()`:

| aggregate     | result                                                   |
|---------------|----------------------------------------------------------|
| `count()`     | number of records                                        |
| `count(path)` | number of values that are not null                       |
| `min(path)`   | the smallest number or string, or null; numbers come     |
|               | before strings                                           |
| `max(path)`   | the largest number or string, or null                    |
| `sum(path)`   | sum of the numbers: an integer, or a float when one of   |
|               | the values is a float; 0 when there are none             |
| `avg(path)`   | the average of the numbers as a float, or null           |

Values of other types are skipped.

`group by path select agg(...)` gives, through `value()`, a table:
one key for each value of the group path, and the aggregate of its records.

- A group key is a bool, an integer, a float or a string. A float equal
  to an integer is the same key as the integer.
- Records where the group path is null, a table or an array are in no
  group.
- When the group path gives many values (`[*]`), the record is in each of
  its groups once.
- `group by` needs an aggregate, and does not go with `order by` or
  `limit`.

```
items[*] group by modes[*].type select count()        -> { x = 4, y = 1 }
items[*] group by country select max(rank)            -> { usa = 7, ... }
```

## How values compare

The rules are the rules of Quirrel, so a script loop moved to a query
gives the same records.

- An integer and a float compare as Quirrel floats (32-bit): `5 == 5.0`.
  Above 2^24 an integer and a float of different values can be equal, as
  in Quirrel (`16777217 == 16777216.0`).
- A float literal in the query is read as a Quirrel float.
- Strings compare by their bytes. A string that the buffer does not have
  compares by its text too.
- Bools: false is smaller than true.
- `null == null` is true. A missing field with no `[*]` in its path is
  null, so `kind == null` finds records with no `kind`.
- Values of different types (apart from two numbers) are not equal and
  have no order: `==`, `<`, `<=`, `>`, `>=` are false, `!=` is true.
- Tables, arrays and instances have no order and equal nothing: only `!=`
  is true.
- A NaN equals nothing and has no order: only `!=` is true. Quirrel is
  not the reference here, because most platforms build it with
  `-ffinite-math-only`.
- In a map, a key matches only a key of the same type (as in a Quirrel
  table), and `0.0` and `-0.0` are two keys.

## Indexes and plans

The `auto` plan:

- runs the simple conditions on the columns first, and checks the rest of
  the condition only on the rows they keep;
- runs a column with values of several types child column by child
  column;
- for `path == constant` (a literal or a parameter) and `path in set`,
  makes an index of the values of the path in the collection, on the first
  use. The index stays with the buffer and serves each later query on the
  same collection and path. `stats()` shows the count and the memory of
  the indexes.

A comparison with a number that is not exact in a Quirrel float
(`|x| >= 2^24`, or a NaN) does not use the index, and `in` compares such
numbers one by one.

A condition that the columns cannot decide falls back to a check of each
row, with the same result:

- a comparison with `null`;
- a path with a filter step `[? ]`;
- a path that reads `@key`.

The `columns` plan uses no index. The `rows` plan checks each record, one
by one. All plans give the same results. They are there for tests and for
timing; production code does not need to give a plan.

## Errors

`pack()`:

- "pack() expects a table, an array or a DataBlock"
- "unsupported table key type", "unsupported value type"
- "nesting is too deep"
- "instance of a class that is not in available_classes"
- "instance must have __getstate method", "class must have __setstate
  method", "failed to call __getstate method"

`save()` and `load()`:

- "save() expects a packed view"
- "save() cannot store class instances, their classes live in one VM"
- "load() expects a token of packed.save() that is not loaded yet"

A read:

- "class Point3 of a packed value is not bound in this VM"
- an error of `__setstate` or of the class constructor

`query()` (the text is checked when the query is compiled):

- "the source path must end with [*]", "only the last step of the
  source path can be [*]"
- "path expected", "identifier expected", "integer index expected",
  "']' expected", "')' expected", "unterminated string", "number
  expected"
- "'by' expected after 'group'", "'by' expected after 'order'",
  "a non-negative integer expected after 'limit'"
- "only count() goes with no path", "the list after 'in' takes only
  literals", "'[' or a parameter expected after 'in'"
- "group by needs an aggregate in select", "order by and limit do not go
  with group by"
- "unexpected text at the end of the query"

A query method:

- "the query has an aggregate in select, use value()"
- "value() needs an aggregate in select"
- "keys() and firstKey() take no select, use select @key"
- "the query has parameters, a table of them is expected", "a query
  parameter is missing", "the parameter after 'in' must be an array"
- "plan must be 'auto', 'columns' or 'rows'"

## Limits

- Nesting depth: 256 levels (an instance state counts as a level).
- A buffer uses 32-bit offsets: it must be smaller than 4 GB.
- A table becomes columns with up to 65535 column fields; a table with
  more fields is a map.

## Tests

`prog/samples/unittest/sqPackedTest` checks pack and unpack, views and
their methods (each against the same method on the unpacked data),
instances, the native types, DataBlock, save and load (also from one VM to
another) and the errors. It also runs about 1600 random queries made of
all features of the language, with each plan and each method, against a
reference of the language written in Quirrel (`reference.nut`). When you change the query language, change
`reference.nut` too.

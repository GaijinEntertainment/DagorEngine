"""
Code Bricks Demo - CodeAlchemist-style constraint-based assembly for Quirrel.

Each "brick" is a code snippet with:
  - preconditions:  what typed variables must exist in the pool
  - postconditions: what typed variables it produces
  - template:       code with $in0, $in1, $out0 placeholders

The assembler picks bricks whose preconditions are satisfiable from the
current variable pool, binds variables, emits code, updates the pool.

Usage:
    python code_bricks.py [seed] [count] [bricks_per_program]
    python code_bricks.py 42          # print one program
    python code_bricks.py 0 10        # write out_0.nut .. out_9.nut
    python code_bricks.py 0 10 25     # 25 bricks per program
"""

import random
import re
import sys
from dataclasses import dataclass


# ---------------------------------------------------------------------------
# Types
# ---------------------------------------------------------------------------

INT      = "int"
FLOAT    = "float"
STRING   = "string"
BOOL     = "bool"
ARRAY    = "array"
TABLE    = "table"
INSTANCE = "instance"
ANY      = "any"

NUMERIC = (INT, FLOAT)


# ---------------------------------------------------------------------------
# Brick definition
# ---------------------------------------------------------------------------

@dataclass
class Slot:
    """A typed variable slot in a brick's pre/post conditions."""
    name: str                 # placeholder name: "in0", "out0", etc.
    type: str | tuple         # e.g. INT or (INT, FLOAT) for union

    def accepts(self, typ: str) -> bool:
        if self.type == ANY:
            return True
        if isinstance(self.type, tuple):
            return typ in self.type
        return typ == self.type


@dataclass
class Brick:
    """A code brick with pre/post conditions and a code template."""
    name: str
    pre:  list[Slot]          # required inputs
    post: list[Slot]          # produced outputs
    template: str             # code with $in0, $out0, etc.
    weight: float = 1.0       # selection probability weight


# ---------------------------------------------------------------------------
# Brick library - 40 bricks covering core Quirrel constructs
# ---------------------------------------------------------------------------

BRICKS: list[Brick] = [
    # ---- Seed bricks (no preconditions) ----
    Brick("int_literal", [], [Slot("out0", INT)],
          "let $out0 = $EXPR_INT"),
    Brick("float_literal", [], [Slot("out0", FLOAT)],
          "let $out0 = $EXPR_FLOAT"),
    Brick("string_literal", [], [Slot("out0", STRING)],
          "let $out0 = $EXPR_STRING"),
    Brick("bool_literal", [], [Slot("out0", BOOL)],
          "let $out0 = $EXPR_BOOL"),
    Brick("empty_array", [], [Slot("out0", ARRAY)],
          "let $out0 = []"),
    Brick("empty_table", [], [Slot("out0", TABLE)],
          "let $out0 = {}"),

    # ---- Integer arithmetic ----
    Brick("int_add", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 + $in1"),
    Brick("int_sub", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 - $in1"),
    Brick("int_mul", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 * $in1"),
    Brick("int_mod_safe", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in1 != 0 ? $in0 % $in1 : $INT"),
    Brick("int_mod_unsafe", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 % $in1"),
    Brick("int_div_safe", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in1 != 0 ? $in0 / $in1 : $INT"),
    Brick("int_div_unsafe", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 / $in1"),
    Brick("int_neg", [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = -$in0"),
    Brick("int_bitand", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 & $in1"),
    Brick("int_bitor", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 | $in1"),

    # ---- Bitwise XOR / NOT / shifts ----
    Brick("int_xor", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 ^ $in1"),
    Brick("int_bitnot", [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = ~$in0"),
    Brick("int_shl", [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 << $BOUND"),
    Brick("int_shr", [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 >> $BOUND"),
    Brick("int_ushr", [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 >>> $BOUND"),

    # ---- Float arithmetic ----
    Brick("float_add", [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0 + $in1"),
    Brick("float_mul", [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0 * $in1"),
    Brick("float_div_safe", [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in1 != 0 ? $in0 / $in1 : $FLOAT"),
    Brick("float_div_unsafe", [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0 / $in1"),
    Brick("int_to_float", [Slot("in0", INT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0.tofloat()"),
    Brick("float_to_int", [Slot("in0", FLOAT)], [Slot("out0", INT)],
          "let $out0 = $in0.tointeger()"),

    # ---- String operations ----
    Brick("string_concat", [Slot("in0", STRING), Slot("in1", STRING)], [Slot("out0", STRING)],
          'let $out0 = $"{$in0}{$in1}"'),
    Brick("string_len", [Slot("in0", STRING)], [Slot("out0", INT)],
          "let $out0 = $in0.len()"),
    Brick("string_slice", [Slot("in0", STRING)], [Slot("out0", STRING)],
          "let $out0 = $in0.len() > 1 ? $in0.slice(0, 1) : $in0"),
    Brick("string_toupper", [Slot("in0", STRING)], [Slot("out0", STRING)],
          "let $out0 = $in0.toupper()"),
    Brick("int_to_string", [Slot("in0", INT)], [Slot("out0", STRING)],
          "let $out0 = $in0.tostring()"),
    Brick("string_interp", [Slot("in0", INT)], [Slot("out0", STRING)],
          'let $out0 = $"val={$in0}"'),

    # ---- Boolean / comparison ----
    Brick("int_cmp_lt", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 < $in1"),
    Brick("str_cmp_eq", [Slot("in0", STRING), Slot("in1", STRING)], [Slot("out0", BOOL)],
          "let $out0 = $in0 == $in1"),
    Brick("bool_not", [Slot("in0", BOOL)], [Slot("out0", BOOL)],
          "let $out0 = !$in0"),
    Brick("bool_and", [Slot("in0", BOOL), Slot("in1", BOOL)], [Slot("out0", BOOL)],
          "let $out0 = $in0 && $in1"),
    Brick("int_cmp_ge", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 >= $in1"),
    Brick("int_cmp_le", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 <= $in1"),
    Brick("int_cmp_ne", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 != $in1"),
    Brick("null_coalesce", [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0 = $in0?.x ?? $INT"),

    # ---- Ternary ----
    Brick("ternary_int", [Slot("in0", BOOL), Slot("in1", INT), Slot("in2", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 ? $in1 : $in2"),
    Brick("ternary_str", [Slot("in0", BOOL), Slot("in1", STRING), Slot("in2", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0 ? $in1 : $in2"),

    # ---- Array operations ----
    Brick("array_of_ints", [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", ARRAY)],
          "let $out0 = [$in0, $in1]"),
    Brick("array_len", [Slot("in0", ARRAY)], [Slot("out0", INT)],
          "let $out0 = $in0.len()"),
    Brick("array_append", [Slot("in0", ARRAY), Slot("in1", INT)], [Slot("out0", ARRAY)],
          "let $out0 = clone $in0; $out0.append($in1)"),

    # ---- Table operations ----
    Brick("table_create", [Slot("in0", INT), Slot("in1", STRING)], [Slot("out0", TABLE)],
          "let $out0 = {x = $in0, s = $in1}"),
    Brick("table_get_x", [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0 = $in0?.x ?? $INT"),
    Brick("table_len", [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0 = $in0.len()"),

    # ---- typeof ----
    Brick("typeof_expr", [Slot("in0", ANY)], [Slot("out0", STRING)],
          "let $out0 = typeof $in0"),

    # ---- Clone ----
    Brick("clone_array", [Slot("in0", ARRAY)], [Slot("out0", ARRAY)],
          "let $out0 = clone $in0"),
    Brick("clone_table", [Slot("in0", TABLE)], [Slot("out0", TABLE)],
          "let $out0 = clone $in0"),

    # ---- if / else ----
    Brick("if_set",
          [Slot("in0", BOOL), Slot("in1", INT), Slot("in2", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in2\n"
          "  if ($in0) {\n"
          "    $out0 = $in1\n"
          "  }"),

    Brick("if_else_int",
          [Slot("in0", BOOL), Slot("in1", INT), Slot("in2", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if ($in0) {\n"
          "    $out0 = $in1 + $INT\n"
          "  } else {\n"
          "    $out0 = $in2 - $INT\n"
          "  }"),

    Brick("if_else_str",
          [Slot("in0", BOOL), Slot("in1", STRING), Slot("in2", STRING)],
          [Slot("out0", STRING)],
          'local $out0 = ""\n'
          "  if ($in0) {\n"
          "    $out0 = $in1\n"
          "  } else {\n"
          "    $out0 = $in2\n"
          "  }"),

    Brick("if_accum",
          [Slot("in0", BOOL), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if ($in0) {\n"
          "    $out0 = $out0 + $in1\n"
          "    $out0 = $out0 + $INT\n"
          "  }"),

    Brick("if_chain",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if ($in0 > $in1) {\n"
          "    $out0 = $INT\n"
          "  } else if ($in0 == $in1) {\n"
          "    $out0 = $INT\n"
          "  } else {\n"
          "    $out0 = $INT\n"
          "  }"),

    Brick("if_nested",
          [Slot("in0", BOOL), Slot("in1", BOOL), Slot("in2", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if ($in0) {\n"
          "    if ($in1) {\n"
          "      $out0 = $in2\n"
          "    } else {\n"
          "      $out0 = -$in2\n"
          "    }\n"
          "  }"),

    # ---- if with variable declaration in condition ----
    Brick("if_let_nullcheck",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if (let $out0_v = $in0?.x; $out0_v != null) {\n"
          "    $out0 = $out0_v\n"
          "  }"),

    Brick("if_let_positive",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if (let $out0_v = $in0; $out0_v > $INT) {\n"
          "    $out0 = $out0_v * $INT\n"
          "  }"),

    Brick("if_let_else",
          [Slot("in0", TABLE), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in1\n"
          "  if (let $out0_v = $in0?.x; $out0_v != null) {\n"
          "    $out0 = $out0_v\n"
          "  } else {\n"
          "    $out0 = $INT\n"
          "  }"),

    Brick("if_local_modify",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  if (local $out0_v = $in0 & 0xFF; $out0_v > $INT) {\n"
          "    $out0_v = $out0_v - $INT\n"
          "    $out0 = $out0_v\n"
          "  }"),

    Brick("if_let_string",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          'local $out0 = ""\n'
          "  if (let $out0_v = $in0; $out0_v.len() > 0) {\n"
          "    $out0 = $out0_v.toupper()\n"
          "  }"),

    # ---- while loops ----
    Brick("while_sum",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    $out0 = $out0 + $in0\n"
          "    $out0_i++\n"
          "  }"),

    Brick("while_countdown",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0 & 15\n"
          "  while ($out0 > 0) {\n"
          "    $out0--\n"
          "  }"),

    Brick("while_accum_cond",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    if ($out0_i % 2 == 0) {\n"
          "      $out0 = $out0 + $in0\n"
          "    } else {\n"
          "      $out0 = $out0 + $in1\n"
          "    }\n"
          "    $out0_i++\n"
          "  }"),

    Brick("while_mul",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 1\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    $out0 = $out0 * (($in0 % 4) + 1)\n"
          "    $out0_i++\n"
          "  }"),

    Brick("while_string_build",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          'local $out0 = ""\n'
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    $out0 = $out0 + $in0\n"
          "    $out0_i++\n"
          "  }"),

    Brick("while_array_fill",
          [],
          [Slot("out0", ARRAY)],
          "let $out0 = []\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    $out0.append($out0_i * $out0_i)\n"
          "    $out0_i++\n"
          "  }"),

    # ---- Lambdas (immediate application) ----
    Brick("lambda_inc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = (@(x) x + $INT)($in0)"),

    Brick("lambda_add",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = (@(a, b) a + b)($in0, $in1)"),

    Brick("lambda_negate",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = (@(x) -x)($in0)"),

    Brick("lambda_str_concat",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", STRING)],
          'let $out0 = (@(a, b) a + b)($in0, $in1)'),

    Brick("lambda_ternary",
          [Slot("in0", BOOL), Slot("in1", INT), Slot("in2", INT)],
          [Slot("out0", INT)],
          "let $out0 = (@(c, a, b) c ? a : b)($in0, $in1, $in2)"),

    # ---- Named functions (define + call) ----
    Brick("func_double",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(x) { return x * $INT }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("func_abs",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(x) { return x >= 0 ? x : -x }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("func_clamp",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(x) {\n"
          "    if (x < $INT) return $INT\n"
          "    if (x > $INT) return $INT\n"
          "    return x\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("func_multi_call",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = @(x) x + $INT\n"
          "  let $out0 = $out0_fn($in0) + $out0_fn($in1)"),

    Brick("func_compose",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_f = @(x) x * $INT\n"
          "  let $out0_g = @(x) x + $INT\n"
          "  let $out0 = $out0_f($out0_g($in0))"),

    Brick("func_recursive_sum",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(n) {\n"
          "    local x = n & 7\n"
          "    if (x <= 0) return 0\n"
          "    return x + callee()(x - 1)\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    # ---- Closures capturing mutable state ----
    Brick("closure_counter",
          [],
          [Slot("out0", INT)],
          "local $out0_state = 0\n"
          "  let $out0_inc = function() { $out0_state++; return $out0_state }\n"
          "  $out0_inc()\n"
          "  $out0_inc()\n"
          "  let $out0 = $out0_inc()"),

    Brick("closure_getter_setter",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_val = $in0\n"
          "  let $out0_set = function(x) { $out0_val = x }\n"
          "  let $out0_get = function() { return $out0_val }\n"
          "  $out0_set($in0 + $INT)\n"
          "  let $out0 = $out0_get()"),

    Brick("closure_accumulator",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0_sum = 0\n"
          "  let $out0_add = function(x) { $out0_sum = $out0_sum + x }\n"
          "  $out0_add($in0)\n"
          "  $out0_add($in1)\n"
          "  $out0_add($in0 + $in1)\n"
          "  let $out0 = $out0_sum"),

    Brick("closure_in_loop",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_val = 0\n"
          "  let $out0_fn = function(x) { $out0_val = $out0_val + x }\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    $out0_fn($in0)\n"
          "    $out0_i++\n"
          "  }\n"
          "  let $out0 = $out0_val"),

    Brick("closure_two_readers",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_shared = $in0\n"
          "  let $out0_r1 = function() { return $out0_shared + $INT }\n"
          "  let $out0_r2 = function() { return $out0_shared * $INT }\n"
          "  $out0_shared = $out0_shared + $INT\n"
          "  let $out0 = $out0_r1() + $out0_r2()"),

    # ---- Higher-order: array methods with lambdas ----
    Brick("array_map_lambda",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = $in0.map(@(v) v)"),

    Brick("array_filter_lambda",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = $in0.filter(@(v) v != null)"),

    Brick("array_reduce_lambda",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "let $out0 = $in0.len() > 0 ? $in0.reduce(@(a, _b) a) : $INT"),

    Brick("array_each_lambda",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  $in0.each(@(_v) $out0++)"),

    # ---- for loop (C-style) ----
    Brick("for_sum",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    $out0 = $out0 + $in0\n"
          "  }"),

    Brick("for_mul_accum",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 1\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    $out0 = $out0 * (($in0 % 3) + 1)\n"
          "  }"),

    Brick("for_build_array",
          [],
          [Slot("out0", ARRAY)],
          "let $out0 = []\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    $out0.append($out0_i)\n"
          "  }"),

    # ---- foreach ----
    Brick("foreach_array_sum",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (v in $in0) {\n"
          "    if (typeof v == \"integer\") $out0 = $out0 + v\n"
          "  }"),

    Brick("foreach_array_count",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (_v in $in0) {\n"
          "    $out0++\n"
          "  }"),

    Brick("foreach_table_keys",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (_k, _v in $in0) {\n"
          "    $out0++\n"
          "  }"),

    # ---- do-while ----
    Brick("do_while_dec",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0 & 7\n"
          "  do {\n"
          "    $out0--\n"
          "  } while ($out0 > 0)"),

    # ---- switch ----
    Brick("switch_int",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  switch ($in0 & 3) {\n"
          "    case 0: $out0 = $INT; break\n"
          "    case 1: $out0 = $INT; break\n"
          "    case 2: $out0 = $INT; break\n"
          "    default: $out0 = $INT\n"
          "  }"),

    Brick("switch_default",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  switch ($in0 % 3) {\n"
          "    case 0: $out0 = $INT; break\n"
          "    case 1: $out0 = $INT; break\n"
          "    default: $out0 = $INT; break\n"
          "  }"),

    # ---- try/catch ----
    Brick("try_catch_safe",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    $out0 = $in0 + $INT\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_throw",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    if ($in0 > $INT) throw $STRING\n"
          "    $out0 = $in0\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_expr_safe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    if ($in1 == 0) throw $STRING\n"
          "    $out0 = $in0 / $in1\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_expr_unsafe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    $out0 = $in0 / $in1\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_string",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          'local $out0 = ""\n'
          "  try {\n"
          "    $out0 = $in0.toupper()\n"
          "  } catch (e) {\n"
          '    $out0 = "error"\n'
          "  }", weight=0.1),

    Brick("try_catch_array",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    $out0 = $in0[$INT]\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_nested",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    try {\n"
          "      $out0 = $in0 / $in1\n"
          "    } catch (_inner) {\n"
          "      $out0 = $in0\n"
          "    }\n"
          "    $out0 = $out0 + $INT\n"
          "  } catch (_outer) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_rethrow",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    try {\n"
          "      if ($in0 > $INT) throw $STRING\n"
          "      $out0 = $in0\n"
          "    } catch (e) {\n"
          "      throw e\n"
          "    }\n"
          "  } catch (_e2) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_throw_int",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    throw $in0\n"
          "  } catch (e) {\n"
          "    $out0 = e\n"
          "  }", weight=0.1),

    Brick("try_catch_throw_table",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    throw {code = $in0, msg = $STRING}\n"
          "  } catch (e) {\n"
          "    $out0 = e?.code ?? $INT\n"
          "  }", weight=0.1),

    Brick("try_catch_in_loop",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    try {\n"
          "      $out0 = $out0 + $in0[$out0_i]\n"
          "    } catch (_e) {\n"
          "      $out0 = $out0 + $INT\n"
          "    }\n"
          "  }", weight=0.1),

    Brick("try_catch_finally_like",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  local $out0_done = false\n"
          "  try {\n"
          "    $out0 = $in0 / $in1\n"
          "    $out0_done = true\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "    $out0_done = true\n"
          "  }\n"
          "  if ($out0_done) $out0 = $out0 + $INT", weight=0.1),

    Brick("try_catch_lambda",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(x) {\n"
          "    if (x > $INT) throw $STRING\n"
          "    return x * $INT\n"
          "  }\n"
          "  local $out0 = $INT\n"
          "  try {\n"
          "    $out0 = $out0_fn($in0)\n"
          "  } catch (e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    # ---- break / continue in loops ----
    Brick("while_break",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < 100) {\n"
          "    if ($out0_i >= $BOUND) break\n"
          "    $out0 = $out0 + $in0\n"
          "    $out0_i++\n"
          "  }"),

    Brick("for_continue",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    if ($out0_i % 2 == 0) continue\n"
          "    $out0 = $out0 + $in0\n"
          "  }"),

    # ---- classes ----
    Brick("class_basic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _val = 0\n"
          "    constructor(xv) { this._val = xv }\n"
          "    function getVal() { return this._val }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0 = $out0_obj.getVal()"),

    Brick("class_method_call",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _a = 0\n"
          "    _b = 0\n"
          "    constructor(xa, xb) { this._a = xa; this._b = xb }\n"
          "    function sum() { return this._a + this._b }\n"
          "    function diff() { return this._a - this._b }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0, $in1)\n"
          "  let $out0 = $out0_obj.sum() + $out0_obj.diff()"),

    Brick("class_inherit",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_Base = class {\n"
          "    _val = 0\n"
          "    constructor(xv) { this._val = xv }\n"
          "    function get() { return this._val }\n"
          "  }\n"
          "  let $out0_Der = class($out0_Base) {\n"
          "    _extra = 0\n"
          "    constructor(xv, xe) { base.constructor(xv); this._extra = xe }\n"
          "    function get() { return base.get() + this._extra }\n"
          "  }\n"
          "  let $out0_obj = $out0_Der($in0, $INT)\n"
          "  let $out0 = $out0_obj.get()"),

    # ---- metamethods ----
    Brick("meta_tostring",
          [Slot("in0", INT)],
          [Slot("out0", STRING)],
          "let $out0_C = class {\n"
          "    _v = 0\n"
          "    constructor(xv) { this._v = xv }\n"
          '    function _tostring() { return $"C({this._v})" }\n'
          "  }\n"
          "  let $out0 = $out0_C($in0).tostring()"),

    Brick("meta_add",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _add(o) { return this.getclass()(this.xv + o.xv) }\n"
          "  }\n"
          "  let $out0_r = $out0_C($in0) + $out0_C($in1)\n"
          "  let $out0 = $out0_r.xv"),

    Brick("meta_get",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _bv = 0\n"
          "    constructor(xb) { this._bv = xb }\n"
          "    function _get(idx) { return this._bv + idx }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0 = $out0_obj[$INT]"),

    Brick("meta_call",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _v = 0\n"
          "    constructor(xv) { this._v = xv }\n"
          "    function _call(_env, arg) { return this._v + arg }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0 = $out0_obj($INT)"),

    Brick("meta_cmp",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", BOOL)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _cmp(o) { return this.xv <=> o.xv }\n"
          "  }\n"
          "  let $out0 = $out0_C($in0) < $out0_C($in1)"),

    # ---- table newslot + in operator ----
    Brick("table_newslot",
          [Slot("in0", INT), Slot("in1", STRING)],
          [Slot("out0", TABLE)],
          "let $out0 = {}\n"
          "  $out0.a <- $in0\n"
          "  $out0.b <- $in1"),

    Brick("table_in",
          [Slot("in0", TABLE)],
          [Slot("out0", BOOL)],
          'let $out0 = "x" in $in0'),

    Brick("table_merge",
          [Slot("in0", TABLE), Slot("in1", TABLE)],
          [Slot("out0", TABLE)],
          "let $out0 = $in0.__merge($in1)"),

    Brick("table_keys",
          [Slot("in0", TABLE)],
          [Slot("out0", ARRAY)],
          "let $out0 = $in0.keys()"),

    Brick("table_values",
          [Slot("in0", TABLE)],
          [Slot("out0", ARRAY)],
          "let $out0 = $in0.values()"),

    # ---- table/array element access in expressions ----
    Brick("table_field_arith",
          [Slot("in0", TABLE), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = ($in0?.x ?? $INT) + $in1"),

    Brick("table_field_cond",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0 = $in0?.x != null ? $in0.x : $INT"),

    Brick("array_elem_arith",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = ($in0.len() > 0 ? $in0[0] : $INT) + $in1"),

    Brick("array_two_elems",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "let $out0 = $in0.len() >= 2\n"
          "    ? $in0[0] + $in0[1]\n"
          "    : $in0.len() == 1 ? $in0[0] : $INT"),

    Brick("array_last_elem",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "let $out0 = $in0.len() > 0 ? $in0[$in0.len() - 1] : $INT"),

    Brick("table_multi_field",
          [Slot("in0", TABLE)],
          [Slot("out0", STRING)],
          'let $out0 = $"{_sv($in0?.x ?? $STRING)}_{_sv($in0?.s ?? $STRING)}"'),

    # ---- table/array content modification ----
    Brick("table_set_field",
          [Slot("in0", TABLE), Slot("in1", INT)],
          [Slot("out0", INT)],
          "$in0.x <- $in1\n"
          "  let $out0 = $in0.x"),

    Brick("table_modify_field",
          [Slot("in0", TABLE), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_prev = $in0?.x ?? $INT\n"
          "  $in0.x <- $in1\n"
          "  let $out0 = $out0_prev + $in0.x"),

    Brick("array_set_elem",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", INT)],
          "if ($in0.len() > 0) $in0[0] = $in1\n"
          "  let $out0 = $in0?[0] ?? $INT"),

    Brick("array_append_read",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", INT)],
          "$in0.append($in1)\n"
          "  let $out0 = $in0[$in0.len() - 1]"),

    Brick("array_modify_elems",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  for (local $out0_i = 0; $out0_i < $out0.len(); $out0_i++)\n"
          "    $out0[$out0_i] = $in1"),

    Brick("table_delete_read",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0_t = clone $in0\n"
          "  $out0_t.tmp <- $INT\n"
          "  let $out0 = $out0_t.tmp"),

    # ---- destructuring ----
    Brick("destruct_table",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_tbl = {a = $in0, b = $in1}\n"
          "  let $out0 = $out0_tbl.a + $out0_tbl.b"),

    Brick("destruct_array",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let [$out0_x, $out0_y] = [$in0, $in1]\n"
          "  let $out0 = $out0_x - $out0_y"),

    Brick("destruct_nested",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let [$out0_a, $out0_b] = [$in0, $in0 + 1]\n"
          "  let [$out0_c] = [$out0_a + $out0_b]\n"
          "  let $out0 = $out0_c"),

    # ---- destructuring in function parameters ----
    Brick("destruct_param_table",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function({da, db}) { return da + db }\n"
          "  let $out0 = $out0_fn({da = $in0, db = $in1})"),

    Brick("destruct_param_array",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function([da, db]) { return da - db }\n"
          "  let $out0 = $out0_fn([$in0, $in1])"),

    Brick("destruct_param_table_default",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function({da, db = $INT}) { return da + db }\n"
          "  let $out0 = $out0_fn({da = $in0})"),

    Brick("destruct_param_array_default",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function([da, db = $INT]) { return da * db }\n"
          "  let $out0 = $out0_fn([$in0])"),

    Brick("destruct_param_mixed",
          [Slot("in0", INT), Slot("in1", INT), Slot("in2", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function({dx, dy = $INT}, [da, db]) {\n"
          "    return dx + dy + da + db\n"
          "  }\n"
          "  let $out0 = $out0_fn({dx = $in0}, [$in1, $in2])"),

    Brick("destruct_param_nested_table",
          [Slot("in0", INT), Slot("in1", STRING)],
          [Slot("out0", STRING)],
          'let $out0_fn = function({da, db = $STRING}) {\n'
          '    return $"{_sv(da)}{_sv(db)}"\n'
          "  }\n"
          "  let $out0 = $out0_fn({da = $in0, db = $in1})"),

    # ---- null-safe access ----
    Brick("nullsafe_field",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0 = $in0?.x ?? $INT"),

    Brick("nullsafe_chain",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0_inner = {v = $in0}\n"
          "  let $out0 = $out0_inner?.v?.x ?? $INT"),

    Brick("nullsafe_index",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "let $out0 = $in0?[$INT] ?? $INT"),

    # ---- coroutines (yield/resume) ----
    Brick("coroutine_basic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = (function() {\n"
          "    let g = (function() { yield $in0; yield $in0 + $INT })()\n"
          "    let a = resume g\n"
          "    let b = resume g\n"
          "    return a + b\n"
          "  })()"),

    Brick("coroutine_sum",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = (function() {\n"
          "    let g = (function() {\n"
          "      for (local i = 0; i < $BOUND; i++) yield i\n"
          "    })()\n"
          "    local s = 0\n"
          "    while (g.getstatus() == \"suspended\") {\n"
          "      s += resume g\n"
          "    }\n"
          "    return s\n"
          "  })()"),

    Brick("coroutine_with_input",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = (function() {\n"
          "    let g = (function() { yield $INT; yield $INT })()\n"
          "    let a = resume g\n"
          "    let b = resume g\n"
          "    return a + b + $in0\n"
          "  })()"),

    # ---- three-way compare ----
    Brick("threeway_cmp",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <=> $in1"),

    # ---- weak references ----
    Brick("weakref_table",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0_wr = $in0.weakref()\n"
          "  let $out0 = $out0_wr.ref()?.x ?? $INT"),

    # ---- array sort/reverse/extend ----
    Brick("array_sort",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  $out0.sort(@(a, b) a <=> b)"),

    Brick("array_reverse",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  $out0.reverse()"),

    Brick("array_extend",
          [Slot("in0", ARRAY), Slot("in1", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  $out0.extend($in1)"),

    Brick("array_findindex",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "let $out0 = $in0.findindex(@(val) val != null) ?? $INT"),

    Brick("array_indexof",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0.indexof($in1) ?? $INT"),

    # ---- variable params (varargs) ----
    Brick("vargv_len",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(a, ...) { return vargv.len() }\n"
          "  let $out0 = $out0_fn($in0, $in1, $INT)"),

    Brick("vargv_sum",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(...) {\n"
          "    local s = 0\n"
          "    foreach (v in vargv) s += v\n"
          "    return s\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0, $in1, $INT)"),

    Brick("vargv_first",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(...) {\n"
          "    return vargv.len() > 0 ? vargv[0] : $INT\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("vargv_forward",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_inner = function(...) {\n"
          "    local s = 0\n"
          "    foreach (v in vargv) s += v\n"
          "    return s\n"
          "  }\n"
          "  let $out0_outer = function(a, ...) { return a + $out0_inner(vargv.len(), a) }\n"
          "  let $out0 = $out0_outer($in0, $in1)"),

    Brick("vargv_to_array",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", ARRAY)],
          "let $out0_fn = function(...) {\n"
          "    let arr = []\n"
          "    foreach (v in vargv) arr.append(v)\n"
          "    return arr\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0, $in1, $INT)"),


    # ---- enums ----
    Brick("enum_basic",
          [],
          [Slot("out0", INT)],
          "$ENUM_DECL { A, B, C }\n"
          "  let $out0 = $ENUM_ID.A + $ENUM_ID.B + $ENUM_ID.C"),

    Brick("enum_explicit",
          [],
          [Slot("out0", INT)],
          "$ENUM_DECL { X = $INT, Y = $INT, Z = $INT }\n"
          "  let $out0 = $ENUM_ID.X + $ENUM_ID.Y + $ENUM_ID.Z"),

    Brick("enum_string",
          [],
          [Slot("out0", STRING)],
          "$ENUM_DECL { A = $STRING, B = $STRING }\n"
          '  let $out0 = $"{$ENUM_ID.A}{$ENUM_ID.B}"'),

    Brick("enum_in_expr",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "$ENUM_DECL { LO = $INT, HI = $INT }\n"
          "  let $out0 = $in0 + $ENUM_ID.LO + $ENUM_ID.HI"),

    Brick("enum_switch",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "$ENUM_DECL { A = 0, B = 1, C = 2 }\n"
          "  local $out0 = $INT\n"
          "  switch ($in0 & 3) {\n"
          "    case $ENUM_ID.A: $out0 = $INT; break\n"
          "    case $ENUM_ID.B: $out0 = $INT; break\n"
          "    case $ENUM_ID.C: $out0 = $INT; break\n"
          "    default: $out0 = $INT\n"
          "  }"),

    Brick("enum_compare",
          [Slot("in0", INT)],
          [Slot("out0", BOOL)],
          "$ENUM_DECL { LOW = 0, MID = 1, HIGH = 2 }\n"
          "  let $out0 = ($in0 & 3) == $ENUM_ID.MID"),

    # ---- function attributes: pure, nodiscard ----
    Brick("func_pure",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = @[pure] (x) x * $INT\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("func_nodiscard",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = @[nodiscard] (x) x + $INT\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("func_pure_nodiscard",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = @[pure, nodiscard] (a, b) a + b\n"
          "  let $out0 = $out0_fn($in0, $in1)"),

    Brick("named_func_pure",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function [pure] $out0_fn(x) { return x * $INT }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("named_func_nodiscard",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function [nodiscard] $out0_fn(x) { return x + $INT }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("named_func_pure_nodiscard",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function [pure, nodiscard] $out0_fn(x) { return x - $INT }\n"
          "  let $out0 = $out0_fn($in0)"),

    # ---- const declarations ----
    Brick("const_int",
          [],
          [Slot("out0", INT)],
          "const $out0 = $INT"),

    Brick("const_string",
          [],
          [Slot("out0", STRING)],
          "const $out0 = $STRING"),

    Brick("const_float",
          [],
          [Slot("out0", FLOAT)],
          "const $out0 = $FLOAT"),

    Brick("const_bool",
          [],
          [Slot("out0", BOOL)],
          "const $out0 = $BOOL"),

    Brick("const_expr",
          [],
          [Slot("out0", INT)],
          "const $out0 = $INT + $INT"),

    Brick("const_pure_call",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "const function [pure] $out0_fn(x) { return x * $INT }\n"
          "  const $out0 = $out0_fn($INT)"),

    Brick("const_pure_call_multi",
          [],
          [Slot("out0", INT)],
          "const function [pure] $out0_fn(a, b) { return a + b * $INT }\n"
          "  const $out0 = $out0_fn($INT, $INT)"),

    Brick("const_chain",
          [],
          [Slot("out0", INT)],
          "const function [pure] $out0_fn(x) { return x + $INT }\n"
          "  const $out0_a = $INT\n"
          "  const $out0 = $out0_fn($out0_a)"),

    # ---- not in ----
    Brick("table_not_in",
          [Slot("in0", TABLE)],
          [Slot("out0", BOOL)],
          'let $out0 = "zzz" not in $in0'),

    # ---- instanceof ----
    Brick("instanceof_check",
          [Slot("in0", INT)],
          [Slot("out0", BOOL)],
          "let $out0_C = class {\n"
          "    _v = 0\n"
          "    constructor(xv) { this._v = xv }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0 = $out0_obj instanceof $out0_C"),

    # ---- prefix / postfix increment / decrement ----
    Brick("postfix_inc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0++"),

    Brick("postfix_dec",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0--"),

    Brick("prefix_inc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  ++$out0"),

    Brick("prefix_dec",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  --$out0"),

    Brick("inc_dec_chain",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0++\n"
          "  $out0++\n"
          "  $out0--"),

    # ---- computed table keys ----
    Brick("table_computed_key",
          [Slot("in0", INT), Slot("in1", STRING)],
          [Slot("out0", TABLE)],
          "let $out0 = {[$in1] = $in0, [\"y\"] = $INT}"),

    Brick("table_computed_key_read",
          [Slot("in0", STRING), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {[$in0] = $in1}\n"
          "  let $out0 = $out0_t?[$in0] ?? $INT"),

    # ---- null-safe call ----
    Brick("nullsafe_call",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0 = $in0 != null ? $in0.keys().len() : $INT"),

    # ---- magic constants ----
    Brick("magic_line",
          [],
          [Slot("out0", INT)],
          "let $out0 = __LINE__"),

    Brick("magic_file",
          [],
          [Slot("out0", STRING)],
          "let $out0 = __FILE__ ?? $STRING"),

    # ---- logical or / and as values ----
    Brick("logical_or_val",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 || $in1"),

    Brick("logical_and_val",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 && $in1"),

    # ---- default parameter values ----
    Brick("default_param_one",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(a, b = $INT) { return a + b }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("default_param_call_with",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(a, b = $INT) { return a + b }\n"
          "  let $out0 = $out0_fn($in0, $in1)"),

    Brick("default_param_multi",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(a, b = $INT, c = $INT) { return a + b + c }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("default_param_string",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          'let $out0_fn = function(a, b = $STRING) { return a + b }\n'
          "  let $out0 = $out0_fn($in0)"),

    Brick("default_param_bool",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(a, flag = $BOOL) { return flag ? a : -a }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("default_param_null",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(a, b = null) { return b != null ? a + b : a }\n"
          "  let $out0 = $out0_fn($in0)"),

    Brick("default_param_lambda",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = @(a, b = $INT, c = $INT) a * b + c\n"
          "  let $out0 = $out0_fn($in0)"),

    # ---- variable modifications (compound assignment) ----
    Brick("modify_add_int",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 += $in1"),

    Brick("modify_sub_int",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 -= $in1"),

    Brick("modify_mul_int",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 *= $in1"),

    Brick("modify_div_int_safe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  if ($in1 != 0) $out0 /= $in1"),

    Brick("modify_div_int_unsafe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 /= $in1"),

    Brick("modify_mod_int_safe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  if ($in1 != 0) $out0 %= $in1"),

    Brick("modify_mod_int_unsafe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 %= $in1"),

    Brick("modify_add_float",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)],
          [Slot("out0", FLOAT)],
          "local $out0 = $in0\n"
          "  $out0 += $in1"),

    Brick("modify_mul_float",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)],
          [Slot("out0", FLOAT)],
          "local $out0 = $in0\n"
          "  $out0 *= $in1"),

    Brick("modify_add_const",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 += $INT"),

    Brick("modify_mul_const",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 *= $INT"),

    Brick("modify_chain",
          [Slot("in0", INT), Slot("in1", INT), Slot("in2", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  $out0 += $in1\n"
          "  $out0 -= $in2\n"
          "  $out0 *= $INT"),

    Brick("modify_in_loop",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    $out0 += $in0\n"
          "  }"),

    Brick("modify_string_concat",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", STRING)],
          "local $out0 = $in0\n"
          "  $out0 += $in1"),

    # ---- string stdlib methods ----
    Brick("string_hash",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "let $out0 = $in0.hash()"),

    Brick("string_contains",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.contains($in1)"),

    Brick("string_startswith",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.startswith($in1)"),

    Brick("string_endswith",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.endswith($in1)"),

    Brick("string_replace",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0.replace($STRING, $STRING)"),

    Brick("string_strip",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0.strip()"),

    Brick("string_lstrip",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0.lstrip()"),

    Brick("string_rstrip",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0.rstrip()"),

    Brick("string_split",
          [Slot("in0", STRING)],
          [Slot("out0", ARRAY)],
          'let $out0 = $in0.split($STRING)'),

    Brick("string_split_by_chars",
          [Slot("in0", STRING)],
          [Slot("out0", ARRAY)],
          'let $out0 = $in0.split_by_chars($STRING)'),

    Brick("string_join",
          [Slot("in0", ARRAY)],
          [Slot("out0", STRING)],
          'let $out0 = ",".join($in0.map(@(v) $"{_sv(v)}"))'),

    Brick("string_escape",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0.escape()"),

    Brick("string_indexof",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", INT)],
          "let $out0 = $in0.indexof($in1) ?? $INT"),

    Brick("string_slice_neg",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          "let $out0 = $in0.len() > 2 ? $in0.slice(-2) : $in0"),

    Brick("string_subst",
          [Slot("in0", STRING)],
          [Slot("out0", STRING)],
          'let $out0 = "{0}+{1}".subst($in0, $STRING)'),

    # ---- table stdlib methods ----
    Brick("table_each",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  $in0.each(@(_v, _k) $out0++)"),

    Brick("table_reduce",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0 = $in0.len() > 0 ? $in0.reduce(@(a, _v) a) : $INT"),

    Brick("table_findindex",
          [Slot("in0", TABLE)],
          [Slot("out0", STRING)],
          'let $out0 = ($in0.findindex(@(_v) true) ?? $STRING).tostring()'),

    Brick("table_findvalue",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0 = $in0.findvalue(@(v) v != null) ?? $INT"),

    Brick("table_hasindex",
          [Slot("in0", TABLE)],
          [Slot("out0", BOOL)],
          'let $out0 = $in0.hasindex("x")'),

    Brick("table_hasvalue",
          [Slot("in0", TABLE), Slot("in1", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.hasvalue($in1)"),

    Brick("table_topairs",
          [Slot("in0", TABLE)],
          [Slot("out0", ARRAY)],
          "let $out0 = $in0.topairs()"),

    Brick("table_update",
          [Slot("in0", TABLE), Slot("in1", TABLE)],
          [Slot("out0", TABLE)],
          "let $out0 = clone $in0\n"
          "  $out0.__update($in1)"),

    Brick("table_replace_with",
          [Slot("in0", TABLE), Slot("in1", TABLE)],
          [Slot("out0", TABLE)],
          "let $out0 = clone $in0\n"
          "  $out0.replace_with($in1)"),

    # ---- array stdlib methods ----
    Brick("array_contains",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.contains($in1)"),

    Brick("array_hasindex",
          [Slot("in0", ARRAY)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.hasindex(0)"),

    Brick("array_hasvalue",
          [Slot("in0", ARRAY), Slot("in1", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.hasvalue($in1)"),

    Brick("array_findvalue",
          [Slot("in0", ARRAY)],
          [Slot("out0", INT)],
          "let $out0 = $in0.findvalue(@(v) v != null) ?? $INT"),

    Brick("array_totable",
          [Slot("in0", ARRAY)],
          [Slot("out0", TABLE)],
          "let $out0 = $in0.totable()"),

    Brick("array_replace_with",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  $out0.replace_with($in0.map(@(v) v))"),

    Brick("array_swap",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  if ($out0.len() >= 2) $out0.swap(0, $out0.len() - 1)"),

    # ---- freeze / is_frozen ----
    Brick("freeze_table",
          [Slot("in0", TABLE)],
          [Slot("out0", TABLE)],
          "let $out0 = freeze(clone $in0)"),

    Brick("freeze_array",
          [Slot("in0", ARRAY)],
          [Slot("out0", ARRAY)],
          "let $out0 = freeze(clone $in0)"),

    Brick("is_frozen_check",
          [Slot("in0", TABLE)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0.is_frozen()"),

    # ---- static keyword ----
    Brick("static_value",
          [],
          [Slot("out0", INT)],
          "let $out0_fn = function() { return static($INT) }\n"
          "  let $out0 = $out0_fn() + $out0_fn()"),

    Brick("static_table",
          [],
          [Slot("out0", TABLE)],
          "let $out0 = static({x = $INT, y = $INT})"),

    # ---- float operations ----
    Brick("float_sub",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0 - $in1"),
    Brick("float_neg",
          [Slot("in0", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = -$in0"),
    Brick("float_mod_safe",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in1 != 0.0 ? $in0 % $in1 : $FLOAT"),
    Brick("float_add_int",
          [Slot("in0", FLOAT), Slot("in1", INT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0 + $in1.tofloat()"),
    Brick("float_mul_int",
          [Slot("in0", FLOAT), Slot("in1", INT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0 * $in1.tofloat()"),
    Brick("float_sub_neg",
          [Slot("in0", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $FLOAT - $in0"),
    Brick("int_float_arith",
          [Slot("in0", INT), Slot("in1", FLOAT)], [Slot("out0", FLOAT)],
          "let $out0 = $in0.tofloat() + $in1"),
    Brick("float_int_compare",
          [Slot("in0", FLOAT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 > $in1.tofloat()"),
    Brick("float_to_string",
          [Slot("in0", FLOAT)], [Slot("out0", STRING)],
          "let $out0 = $in0.tostring()"),

    # ---- more bool / comparison ----
    Brick("bool_or",
          [Slot("in0", BOOL), Slot("in1", BOOL)], [Slot("out0", BOOL)],
          "let $out0 = $in0 || $in1"),
    Brick("int_cmp_gt",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 > $in1"),
    Brick("int_cmp_eq",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 == $in1"),
    Brick("float_cmp_lt",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 < $in1"),
    Brick("float_cmp_eq",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)], [Slot("out0", BOOL)],
          "let $out0 = $in0 == $in1"),
    Brick("string_cmp_lt",
          [Slot("in0", STRING), Slot("in1", STRING)], [Slot("out0", BOOL)],
          "let $out0 = $in0 < $in1"),
    Brick("string_cmp_ne",
          [Slot("in0", STRING), Slot("in1", STRING)], [Slot("out0", BOOL)],
          "let $out0 = $in0 != $in1"),
    Brick("any_cmp_eq",
          [Slot("in0", ANY), Slot("in1", ANY)], [Slot("out0", BOOL)],
          "local $out0 = false\n"
          "  try { $out0 = $in0 == $in1 } catch (_e) {}"),

    # ---- typeof checks ----
    Brick("typeof_int_check",
          [Slot("in0", INT)], [Slot("out0", BOOL)],
          'let $out0 = typeof $in0 == "integer"'),
    Brick("typeof_string_check",
          [Slot("in0", STRING)], [Slot("out0", BOOL)],
          'let $out0 = typeof $in0 == "string"'),
    Brick("typeof_float_check",
          [Slot("in0", FLOAT)], [Slot("out0", BOOL)],
          'let $out0 = typeof $in0 == "float"'),
    Brick("typeof_array_check",
          [Slot("in0", ARRAY)], [Slot("out0", BOOL)],
          'let $out0 = typeof $in0 == "array"'),
    Brick("typeof_table_check",
          [Slot("in0", TABLE)], [Slot("out0", BOOL)],
          'let $out0 = typeof $in0 == "table"'),
    Brick("typeof_null_check",
          [], [Slot("out0", BOOL)],
          "let $out0 = typeof null == \"null\""),
    Brick("typeof_switch",
          [Slot("in0", ANY)], [Slot("out0", STRING)],
          "local $out0 = typeof $in0\n"
          "  switch ($out0) {\n"
          "    case \"integer\": $out0 = \"int\"; break\n"
          "    case \"float\": $out0 = \"flt\"; break\n"
          "    case \"string\": $out0 = \"str\"; break\n"
          "    default: $out0 = \"other\"\n"
          "  }"),
    Brick("type_function",
          [Slot("in0", ANY)], [Slot("out0", STRING)],
          "let $out0 = type($in0)"),

    # ---- null patterns ----
    Brick("null_var",
          [], [Slot("out0", ANY)],
          "let $out0 = null"),
    Brick("null_check_ternary",
          [Slot("in0", TABLE)], [Slot("out0", BOOL)],
          "let $out0 = $in0 != null"),
    Brick("null_coalesce_chain",
          [Slot("in0", TABLE), Slot("in1", TABLE)], [Slot("out0", INT)],
          "let $out0 = $in0?.x ?? $in1?.x ?? $INT"),
    Brick("null_in_array",
          [], [Slot("out0", ARRAY)],
          "let $out0 = [null, null, null]"),
    Brick("null_table_value",
          [], [Slot("out0", TABLE)],
          "let $out0 = {a = null, b = null}"),
    Brick("null_assign_fallback",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_maybe = $in0 > $INT ? $in0 : null\n"
          "  let $out0 = $out0_maybe ?? $INT"),

    # ---- type annotations ----
    Brick("typed_int_var",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0: int = $in0"),
    Brick("typed_float_var",
          [Slot("in0", FLOAT)], [Slot("out0", FLOAT)],
          "local $out0: float = $in0"),
    Brick("typed_string_var",
          [Slot("in0", STRING)], [Slot("out0", STRING)],
          "local $out0: string = $in0"),
    Brick("typed_function_params",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(a: int, b: int): int { return a + b }\n"
          "  let $out0 = $out0_fn($in0, $in1)"),
    Brick("typed_function_return",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x: int): int { return x * $INT }\n"
          "  let $out0 = $out0_fn($in0)"),
    Brick("typed_union_param",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x: int|float): int { return x.tointeger() }\n"
          "  let $out0 = $out0_fn($in0)"),
    Brick("typed_nullable_param",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x: int|null): int { return x != null ? x : $INT }\n"
          "  let $out0 = $out0_fn($in0)"),
    Brick("typed_lambda",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_fn = @(a: int, b: int) a * b\n"
          "  let $out0 = $out0_fn($in0, $in1)"),
    Brick("typed_return_null",
          [Slot("in0", INT)], [Slot("out0", BOOL)],
          "let $out0_fn = function(x: int): string|null { return x > $INT ? $STRING : null }\n"
          "  let $out0 = $out0_fn($in0) != null"),

    # ---- more metamethods ----
    Brick("meta_sub",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _sub(o) { return this.getclass()(this.xv - o.xv) }\n"
          "  }\n"
          "  let $out0_r = $out0_C($in0) - $out0_C($in1)\n"
          "  let $out0 = $out0_r.xv"),
    Brick("meta_mul",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _mul(o) { return this.getclass()(this.xv * o.xv) }\n"
          "  }\n"
          "  let $out0_r = $out0_C($in0) * $out0_C($in1)\n"
          "  let $out0 = $out0_r.xv"),
    Brick("meta_unm",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _unm() { return this.getclass()(-this.xv) }\n"
          "  }\n"
          "  let $out0_r = -$out0_C($in0)\n"
          "  let $out0 = $out0_r.xv"),
    Brick("meta_cloned",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    extra = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _cloned(_other) { this.extra = $INT }\n"
          "  }\n"
          "  let $out0_orig = $out0_C($in0)\n"
          "  let $out0_cpy = clone $out0_orig\n"
          "  let $out0 = $out0_cpy.xv + $out0_cpy.extra"),
    Brick("meta_nexti",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _n = 0\n"
          "    constructor(n) { this._n = n & 7 }\n"
          "    function _nexti(idx) {\n"
          "      local next = idx == null ? 0 : idx + 1\n"
          "      return next < this._n ? next : null\n"
          "    }\n"
          "    function _get(idx) { return idx * $INT }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  local $out0 = 0\n"
          "  foreach (_i, v in $out0_obj) $out0 += v"),
    Brick("meta_typeof",
          [Slot("in0", INT)], [Slot("out0", STRING)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _typeof() { return \"mytype\" }\n"
          "  }\n"
          "  let $out0 = typeof $out0_C($in0)"),

    # ---- more class features ----
    Brick("class_getclass_use",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function copy() { return this.getclass()(this.xv) }\n"
          "  }\n"
          "  let $out0_o = $out0_C($in0)\n"
          "  let $out0 = $out0_o.copy().xv"),
    Brick("class_three_level",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_A = class { v = 0; constructor(x) { this.v = x }; function get() { return this.v } }\n"
          "  let $out0_B = class($out0_A) { bonus = 0; constructor(x) { base.constructor(x); this.bonus = $INT } }\n"
          "  let $out0_C = class($out0_B) { extra = 0; constructor(x) { base.constructor(x); this.extra = $INT } }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0 = $out0_obj.get() + $out0_obj.bonus + $out0_obj.extra"),
    Brick("class_instanceof_base",
          [Slot("in0", INT)], [Slot("out0", BOOL)],
          "let $out0_Base = class { xv = 0; constructor(x) { this.xv = x } }\n"
          "  let $out0_Der = class($out0_Base) { constructor(x) { base.constructor(x) } }\n"
          "  let $out0_obj = $out0_Der($in0)\n"
          "  let $out0 = $out0_obj instanceof $out0_Base"),
    Brick("class_method_override",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_Base = class {\n"
          "    function compute(x) { return x + $INT }\n"
          "  }\n"
          "  let $out0_Der = class($out0_Base) {\n"
          "    function compute(x) { return base.compute(x) * $INT }\n"
          "  }\n"
          "  let $out0 = $out0_Der().compute($in0)"),
    Brick("class_array_of",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "  }\n"
          "  let $out0_arr = [$out0_C($in0), $out0_C($in0 + 1), $out0_C($in0 + 2)]\n"
          "  let $out0 = $out0_arr[0].xv + $out0_arr[1].xv + $out0_arr[2].xv"),
    Brick("class_constructor_default",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = $INT\n"
          "    function get() { return this.xv }\n"
          "  }\n"
          "  let $out0 = $out0_C().get()"),
    Brick("class_field_from_table",
          [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _t = null\n"
          "    constructor(t) { this._t = t }\n"
          "    function get() { return this._t?.x ?? $INT }\n"
          "  }\n"
          "  let $out0 = $out0_C($in0).get()"),
    Brick("class_with_array",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _items = []\n"
          "    function add(x) { this._items.append(x) }\n"
          "    function sum() {\n"
          "      local s = 0\n"
          "      foreach (v in this._items) s += v\n"
          "      return s\n"
          "    }\n"
          "  }\n"
          "  let $out0_obj = $out0_C()\n"
          "  $out0_obj.add($in0)\n"
          "  $out0_obj.add($in0 + 1)\n"
          "  let $out0 = $out0_obj.sum()"),
    Brick("class_getbase",
          [Slot("in0", INT)], [Slot("out0", BOOL)],
          "let $out0_Base = class { xv = 0 }\n"
          "  let $out0_Der = class($out0_Base) { yv = 0 }\n"
          "  let $out0 = $out0_Der.getbase() == $out0_Base"),

    # ---- rawget / rawset / rawin / rawdelete ----
    Brick("rawget_table",
          [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0 = $in0.rawget(\"x\") ?? $INT"),
    Brick("rawset_table",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {}\n"
          "  $out0_t.rawset(\"x\", $in0)\n"
          "  let $out0 = $out0_t.rawget(\"x\") ?? $INT"),
    Brick("rawin_table",
          [Slot("in0", TABLE)], [Slot("out0", BOOL)],
          "let $out0 = $in0.rawin(\"x\")"),
    Brick("table_rawdelete",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {x = $in0, y = $INT}\n"
          "  let $out0_v = $out0_t.rawget(\"x\") ?? $INT\n"
          "  $out0_t.rawdelete(\"x\")\n"
          "  let $out0 = $out0_v"),

    # ---- table delegate ----
    Brick("table_setdelegate",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_base = {extra = $INT, x = $INT}\n"
          "  let $out0_t = {x = $in0}\n"
          "  let $out0 = $out0_t.rawget(\"x\") ?? $out0_base.extra"),
    Brick("table_getdelegate",
          [Slot("in0", TABLE)], [Slot("out0", BOOL)],
          "let $out0 = $in0.len() > 0"),

    # ---- more foreach ----
    Brick("foreach_array_break",
          [Slot("in0", ARRAY)], [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  foreach (v in $in0) {\n"
          "    if (typeof v == \"integer\") { $out0 = v; break }\n"
          "  }"),
    Brick("foreach_table_values_sum",
          [Slot("in0", TABLE)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (_k, v in $in0) {\n"
          "    if (typeof v == \"integer\") $out0 += v\n"
          "  }"),
    Brick("foreach_array_continue",
          [Slot("in0", ARRAY)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (idx, v in $in0) {\n"
          "    if (idx % 2 != 0) continue\n"
          "    if (typeof v == \"integer\") $out0 += v\n"
          "  }"),
    Brick("foreach_collect_keys",
          [Slot("in0", TABLE)], [Slot("out0", ARRAY)],
          "let $out0 = []\n"
          "  foreach (k, _v in $in0) $out0.append(k)"),
    Brick("foreach_string_chars",
          [Slot("in0", STRING)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (_c in $in0) $out0++"),
    Brick("foreach_nested",
          [Slot("in0", ARRAY), Slot("in1", ARRAY)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (_a in $in0) {\n"
          "    foreach (_b in $in1) {\n"
          "      $out0++\n"
          "    }\n"
          "  }"),

    # ---- compound assign on table fields / array elements ----
    Brick("table_field_pluseq",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  $out0_t.x += $INT\n"
          "  let $out0 = $out0_t.x"),
    Brick("table_field_muleq",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  $out0_t.x *= $INT\n"
          "  let $out0 = $out0_t.x"),
    Brick("table_field_minuseq",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  $out0_t.x -= $INT\n"
          "  let $out0 = $out0_t.x"),
    Brick("array_elem_pluseq",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_a = [$in0, $in0 + 1]\n"
          "  $out0_a[0] += $INT\n"
          "  let $out0 = $out0_a[0]"),
    Brick("array_elem_muleq",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_a = [$in0]\n"
          "  $out0_a[0] *= $INT\n"
          "  let $out0 = $out0_a[0]"),

    # ---- switch variants ----
    Brick("switch_string",
          [Slot("in0", STRING)], [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  switch ($in0) {\n"
          "    case \"a\": $out0 = $INT; break\n"
          "    case \"b\": $out0 = $INT; break\n"
          "    case \"c\": $out0 = $INT; break\n"
          "    default: $out0 = $INT\n"
          "  }"),
    Brick("switch_fallthrough",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  switch ($in0 & 3) {\n"
          "    case 0:\n"
          "    case 1: $out0 = $INT; break\n"
          "    case 2: $out0 = $INT; break\n"
          "    default: $out0 = $INT\n"
          "  }"),
    Brick("switch_three_way",
          [Slot("in0", INT)], [Slot("out0", STRING)],
          "local $out0 = \"\"\n"
          "  switch ($in0 <=> $INT) {\n"
          "    case -1: $out0 = \"less\"; break\n"
          "    case 0: $out0 = \"equal\"; break\n"
          "    case 1: $out0 = \"greater\"; break\n"
          "    default: $out0 = \"other\"\n"
          "  }"),
    Brick("switch_enum_full",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "$ENUM_DECL { RED = 0, GREEN = 1, BLUE = 2, ALPHA = 3 }\n"
          "  local $out0 = $INT\n"
          "  switch ($in0 & 3) {\n"
          "    case $ENUM_ID.RED: $out0 = $INT; break\n"
          "    case $ENUM_ID.GREEN: $out0 = $INT; break\n"
          "    case $ENUM_ID.BLUE: $out0 = $INT; break\n"
          "    default: $out0 = $INT\n"
          "  }"),

    # ---- more loop variants ----
    Brick("for_countdown",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = $BOUND; $out0_i > 0; $out0_i--) {\n"
          "    $out0 += $in0\n"
          "  }"),
    Brick("for_break_early",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    $out0 += $in0\n"
          "    if ($out0 >= ($in1 & 0x7FFFFFFF)) break\n"
          "  }"),
    Brick("while_continue_skip",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  local $out0_i = 0\n"
          "  while ($out0_i < $BOUND) {\n"
          "    $out0_i++\n"
          "    if ($out0_i % 2 == 0) continue\n"
          "    $out0 += $in0\n"
          "  }"),
    Brick("nested_loops",
          [], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++) {\n"
          "    for (local $out0_j = 0; $out0_j < $BOUND; $out0_j++) {\n"
          "      $out0 += $out0_i * $out0_j\n"
          "    }\n"
          "  }"),
    Brick("do_while_count",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  local $out0_i = 0\n"
          "  do {\n"
          "    $out0 += $in0\n"
          "    $out0_i++\n"
          "  } while ($out0_i < $BOUND)"),
    Brick("while_early_return",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(n) {\n"
          "    local i = 0\n"
          "    while (i < $BOUND) {\n"
          "      if (i * n > $INT) return i\n"
          "      i++\n"
          "    }\n"
          "    return i\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    # ---- more array operations ----
    Brick("array_of_strings",
          [Slot("in0", STRING), Slot("in1", STRING)], [Slot("out0", ARRAY)],
          "let $out0 = [$in0, $in1, $STRING]"),
    Brick("array_of_bools",
          [Slot("in0", BOOL), Slot("in1", BOOL)], [Slot("out0", ARRAY)],
          "let $out0 = [$in0, $in1, true, false]"),
    Brick("array_slice_mid",
          [Slot("in0", ARRAY)], [Slot("out0", ARRAY)],
          "let $out0 = $in0.len() >= 2 ? $in0.slice(1) : clone $in0"),
    Brick("array_push_pop",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_a = [$in0, $in0 + 1]\n"
          "  $out0_a.append($in0 * 2)\n"
          "  let $out0 = $out0_a.pop()"),
    Brick("array_insert_remove",
          [Slot("in0", INT)], [Slot("out0", ARRAY)],
          "let $out0 = [$INT, $INT, $INT]\n"
          "  $out0.insert(1, $in0)\n"
          "  $out0.remove(0)"),
    Brick("array_resize",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_a = []\n"
          "  $out0_a.resize(4, $in0)\n"
          "  let $out0 = $out0_a.len()"),
    Brick("array_top",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_a = [$in0, $in1, $INT]\n"
          "  let $out0 = $out0_a.top()"),
    Brick("array_apply_inplace",
          [Slot("in0", ARRAY)], [Slot("out0", ARRAY)],
          "let $out0 = clone $in0\n"
          "  $out0.apply(@(v) v)"),
    Brick("array_function_init",
          [Slot("in0", INT)], [Slot("out0", ARRAY)],
          "let $out0 = array($in0 & 7, $INT)"),
    Brick("array_reduce_sum",
          [Slot("in0", ARRAY)], [Slot("out0", INT)],
          "let $out0 = $in0.len() > 0\n"
          "    ? $in0.reduce(@(acc, v) (typeof v == \"integer\" ? acc + v : acc), 0)\n"
          "    : $INT"),
    Brick("array_filter_map",
          [Slot("in0", ARRAY)], [Slot("out0", ARRAY)],
          "let $out0 = $in0.filter(@(v) v != null).map(@(v) v)"),
    Brick("array_mixed_types",
          [Slot("in0", INT), Slot("in1", STRING)], [Slot("out0", ARRAY)],
          "let $out0 = [$in0, $in1, true, null]"),

    # ---- more string operations ----
    Brick("string_tofloat",
          [Slot("in0", STRING)], [Slot("out0", FLOAT)],
          "let $out0 = $in0.tofloat()"),
    Brick("string_find_method",
          [Slot("in0", STRING), Slot("in1", STRING)], [Slot("out0", INT)],
          "let $out0_idx = $in0.indexof($in1)\n"
          "  let $out0 = $out0_idx != null ? $out0_idx : $INT"),
    Brick("string_tolower",
          [Slot("in0", STRING)], [Slot("out0", STRING)],
          "let $out0 = $in0.tolower()"),
    Brick("string_format_pct",
          [Slot("in0", INT), Slot("in1", FLOAT)], [Slot("out0", STRING)],
          "let $out0 = $\"todo format: {$in0} {$in1}\""),
    Brick("string_slice_both",
          [Slot("in0", STRING)], [Slot("out0", STRING)],
          "let $out0 = $in0.len() > 4 ? $in0.slice(1, 3) : $in0"),
    Brick("string_tointeger",
          [Slot("in0", STRING)], [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try { $out0 = $in0.tointeger() } catch (_e) {}"),
    Brick("string_repeat_concat",
          [Slot("in0", STRING)], [Slot("out0", STRING)],
          "local $out0 = \"\"\n"
          "  for (local $out0_i = 0; $out0_i < $BOUND; $out0_i++)\n"
          "    $out0 += $in0"),
    Brick("string_interp_float",
          [Slot("in0", FLOAT)], [Slot("out0", STRING)],
          "let $out0 = $\"val={$in0}\""),
    Brick("string_interp_bool",
          [Slot("in0", BOOL)], [Slot("out0", STRING)],
          "let $out0 = $\"flag={$in0}\""),

    # ---- more table operations ----
    Brick("table_clear",
          [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0_t = clone $in0\n"
          "  $out0_t.clear()\n"
          "  let $out0 = $out0_t.len()"),
    Brick("table_filter",
          [Slot("in0", TABLE)], [Slot("out0", TABLE)],
          "let $out0 = $in0.filter(@(v) v != null)"),
    Brick("table_map",
          [Slot("in0", TABLE)], [Slot("out0", TABLE)],
          "let $out0 = $in0.map(@(v) v)"),
    Brick("table_each_collect",
          [Slot("in0", TABLE)], [Slot("out0", ARRAY)],
          "let $out0 = []\n"
          "  $in0.each(@(v, _k) $out0.append(v))"),
    Brick("table_deep",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {a = {b = {c = $in0}}}\n"
          "  let $out0 = $out0_t?.a?.b?.c ?? $INT"),

    # ---- more coroutine patterns ----
    Brick("coroutine_send_val",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = (function() {\n"
          "    let $out0_iv = $in0\n"
          "    let g = (function() {\n"
          "      for (local i = 0; i < 3; i++) yield $out0_iv + i\n"
          "    })()\n"
          "    local acc = 0\n"
          "    while (g.getstatus() == \"suspended\") {\n"
          "      acc += resume g\n"
          "    }\n"
          "    return acc\n"
          "  })()", weight=0.5),
    Brick("coroutine_collect_all",
          [], [Slot("out0", ARRAY)],
          "let $out0 = (function() {\n"
          "    let gen = (function() {\n"
          "      yield $INT\n"
          "      yield $INT\n"
          "      yield $INT\n"
          "    })()\n"
          "    let arr = []\n"
          "    while (gen.getstatus() == \"suspended\") {\n"
          "      arr.append(resume gen)\n"
          "    }\n"
          "    return arr\n"
          "  })()", weight=0.5),
    Brick("coroutine_newthread",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_iv = $in0\n"
          "  let $out0_t = (function() { yield $out0_iv; yield $out0_iv + 1 })()\n"
          "  let $out0_a = resume $out0_t\n"
          "  let $out0_b = resume $out0_t\n"
          "  let $out0 = $out0_a + $out0_b", weight=0.5),
    Brick("coroutine_try_catch",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  try {\n"
          "    let g = (function() { yield $in0; yield $in0 + 1 })()\n"
          "    $out0 = resume g\n"
          "  } catch (_e) {\n"
          "    $out0 = $INT\n"
          "  }", weight=0.1),

    # ---- more function patterns ----
    Brick("func_bindenv",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_env = {base_val = $in0}\n"
          "  let $out0_fn = (function() { return this.base_val + $INT }).bindenv($out0_env)\n"
          "  let $out0 = $out0_fn()"),
    Brick("immediate_invoke",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = (function(x) { return x * x })($in0)"),
    Brick("immediate_lambda",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = (@(a, b) a + b)($in0, $in1)"),
    Brick("nested_function",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_outer = function(x) {\n"
          "    let inner = @(n) n * $INT\n"
          "    return inner(x) + $INT\n"
          "  }\n"
          "  let $out0 = $out0_outer($in0)"),
    Brick("mutual_recursion",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_iseven = null\n"
          "  local $out0_isodd = null\n"
          "  $out0_iseven = @(n) (n & 7) == 0 ? true : $out0_isodd(n - 1)\n"
          "  $out0_isodd = @(n) (n & 7) == 0 ? false : $out0_iseven(n - 1)\n"
          "  let $out0 = $out0_iseven($in0 & 7) ? $INT : $INT"),
    Brick("higher_order_compose",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_compose = @(f, g) @(x) f(g(x))\n"
          "  let $out0_f = @(x) x + $INT\n"
          "  let $out0_g = @(x) x * $INT\n"
          "  let $out0 = $out0_compose($out0_f, $out0_g)($in0)"),
    Brick("higher_order_apply",
          [Slot("in0", ARRAY)], [Slot("out0", ARRAY)],
          "let $out0_apply = @(fn, arr) arr.map(fn)\n"
          "  let $out0 = $out0_apply(@(v) v, $in0)"),
    Brick("func_returns_func",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_make = @(bv) @(x) x + bv\n"
          "  let $out0_add = $out0_make($in0)\n"
          "  let $out0 = $out0_add($INT)"),

    # ---- more null-safe patterns ----
    Brick("nullsafe_nested_table",
          [], [Slot("out0", INT)],
          "let $out0_outer = {inner = {value = $INT}}\n"
          "  let $out0 = $out0_outer?.inner?.value ?? $INT"),
    Brick("nullsafe_array_index",
          [Slot("in0", ARRAY)], [Slot("out0", INT)],
          "let $out0 = $in0?[$INT] ?? $INT"),
    Brick("nullsafe_method_on_result",
          [Slot("in0", TABLE)], [Slot("out0", INT)],
          "let $out0_arr = $in0?.keys\n"
          "  let $out0 = $out0_arr != null ? $in0.keys().len() : $INT"),

    # ---- assert / error ----
    Brick("assert_tautology",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0\n"
          "  assert($out0 == $out0)"),
    Brick("assert_msg",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0\n"
          "  assert($out0 >= 0 || $out0 <= 0, \"value check\")"),

    # ---- freeze / immutability ----
    Brick("freeze_nested",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = freeze({x = $in0, y = $INT})\n"
          "  let $out0 = $out0_t.x + $out0_t.y"),
    Brick("is_frozen_false",
          [Slot("in0", TABLE)], [Slot("out0", BOOL)],
          "let $out0_t = clone $in0\n"
          "  let $out0 = $out0_t.is_frozen()"),

    # ---- getroottable / globals ----
    Brick("getroottable_use",
          [], [Slot("out0", BOOL)],
          "let $out0 = getroottable() != null"),
    Brick("getconsttable_use",
          [], [Slot("out0", BOOL)],
          "let $out0 = getconsttable() != null"),

    # ---- mixed type arithmetic ----
    Brick("int_tostring_concat",
          [Slot("in0", INT), Slot("in1", STRING)], [Slot("out0", STRING)],
          "let $out0 = $\"{$in0}\" + $in1"),
    Brick("bool_to_string",
          [Slot("in0", BOOL)], [Slot("out0", STRING)],
          "let $out0 = $in0.tostring()"),

    # ---- more int arithmetic ----
    Brick("int_abs_ternary",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 >= 0 ? $in0 : -$in0"),
    Brick("int_clamp",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_lo = $INT\n"
          "  let $out0_hi = $out0_lo + $INT\n"
          "  let $out0 = $in0 < $out0_lo ? $out0_lo : ($in0 > $out0_hi ? $out0_hi : $in0)"),
    Brick("int_sign",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 > 0 ? 1 : ($in0 < 0 ? -1 : 0)"),
    Brick("int_min_two",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 < $in1 ? $in0 : $in1"),
    Brick("int_max_two",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0 = $in0 > $in1 ? $in0 : $in1"),
    Brick("int_pow2_check",
          [Slot("in0", INT)], [Slot("out0", BOOL)],
          "let $out0_v = $in0 & 0xFFFF\n"
          "  let $out0 = $out0_v > 0 && ($out0_v & ($out0_v - 1)) == 0"),
    Brick("int_bit_count",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  local $out0_v = $in0 & 0xFF\n"
          "  while ($out0_v != 0) {\n"
          "    $out0 += $out0_v & 1\n"
          "    $out0_v = $out0_v >>> 1\n"
          "  }"),
    Brick("int_swap_via_xor",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "local $out0_a = $in0\n"
          "  local $out0_b = $in1\n"
          "  $out0_a = $out0_a ^ $out0_b\n"
          "  $out0_b = $out0_a ^ $out0_b\n"
          "  $out0_a = $out0_a ^ $out0_b\n"
          "  let $out0 = $out0_a"),
    Brick("int_gcd",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "local $out0_a = ($in0 & 0xFFFF) + 1\n"
          "  local $out0_b = ($in1 & 0xFFFF) + 1\n"
          "  while ($out0_b != 0) {\n"
          "    local $out0_tmp = $out0_b\n"
          "    $out0_b = $out0_a % $out0_b\n"
          "    $out0_a = $out0_tmp\n"
          "  }\n"
          "  let $out0 = $out0_a"),

    # ---- string scan / manual pattern matching ----
    Brick("string_scan_digits",
          [Slot("in0", STRING)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (c in $in0) {\n"
          "    if (c >= '0' && c <= '9') $out0++\n"
          "  }"),
    Brick("string_scan_upper",
          [Slot("in0", STRING)], [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (c in $in0) {\n"
          "    if (c >= 'A' && c <= 'Z') $out0++\n"
          "  }"),
    Brick("string_build_filtered",
          [Slot("in0", STRING)], [Slot("out0", STRING)],
          "local $out0 = \"\"\n"
          "  foreach (c in $in0) {\n"
          "    if (c != ' ') $out0 += c.tochar()\n"
          "  }"),

    # ---- more enum patterns ----
    Brick("enum_mixed_types",
          [], [Slot("out0", STRING)],
          "$ENUM_DECL { CODE = $INT, NAME = $STRING }\n"
          "  let $out0 = $\"{$ENUM_ID.CODE}{$ENUM_ID.NAME}\""),
    Brick("enum_in_table",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "$ENUM_DECL { V1 = $INT, V2 = $INT }\n"
          "  let $out0_t = {a = $ENUM_ID.V1, b = $ENUM_ID.V2}\n"
          "  let $out0 = $out0_t.a + $out0_t.b"),
    Brick("enum_compare_chain",
          [Slot("in0", INT)], [Slot("out0", STRING)],
          "$ENUM_DECL { LOW = 0, MID = 50, HIGH = 100 }\n"
          "  local $out0 = \"\"\n"
          "  if ($in0 < $ENUM_ID.MID) $out0 = \"low\"\n"
          "  else if ($in0 < $ENUM_ID.HIGH) $out0 = \"mid\"\n"
          "  else $out0 = \"high\""),

    # ---- data structure patterns ----
    Brick("table_as_stack",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_stack = []\n"
          "  $out0_stack.append($in0)\n"
          "  $out0_stack.append($in1)\n"
          "  let $out0 = $out0_stack.pop()"),
    Brick("table_registry",
          [Slot("in0", INT), Slot("in1", STRING)], [Slot("out0", INT)],
          "let $out0_reg = {}\n"
          "  $out0_reg[$in1] <- $in0\n"
          "  let $out0 = $out0_reg[$in1]"),
    Brick("array_as_queue",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_q = [$in0, $in1]\n"
          "  $out0_q.append($INT)\n"
          "  let $out0_val = $out0_q[0]\n"
          "  $out0_q.remove(0)\n"
          "  let $out0 = $out0_val"),

    # ---- expression statements / side-effect patterns ----
    Brick("expr_stmt_assign_chain",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0 = $INT\n"
          "  $out0 = $in0\n"
          "  $out0 = $out0 + $INT\n"
          "  $out0 = $out0 * $INT"),
    Brick("multi_assign",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "local $out0_a = $in0\n"
          "  local $out0_b = $in1\n"
          "  $out0_a = $out0_a + $out0_b\n"
          "  $out0_b = $out0_a - $out0_b\n"
          "  $out0_a = $out0_a - $out0_b\n"
          "  let $out0 = $out0_a"),

    # ---- more destructuring ----
    Brick("destruct_swap",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let [$out0_a, $out0_b] = [$in0, $in1]\n"
          "  let [$out0_x, $out0_y] = [$out0_b, $out0_a]\n"
          "  let $out0 = $out0_x + $out0_y"),
    Brick("destruct_defaults",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let {$out0_da = $INT, $out0_db = $INT} = {$out0_da = $in0}\n"
          "  let $out0 = $out0_da + $out0_db"),
    Brick("destruct_from_func",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = @(x) {a = x, b = x + 1}\n"
          "  let $out0_res = $out0_fn($in0)\n"
          "  let $out0 = $out0_res.a + $out0_res.b"),

    # ---- more const / static ----
    Brick("const_in_expr",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "const $out0_BASE = $INT\n"
          "  const $out0_SCALE = $INT\n"
          "  let $out0 = $in0 * $out0_SCALE + $out0_BASE"),
    Brick("static_complex",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x) {\n"
          "    local cached = static({v = $INT * $INT})\n"
          "    return x + cached.v\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    # ---- more varargs ----
    Brick("vargv_last",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(...) {\n"
          "    return vargv.len() > 0 ? vargv[vargv.len() - 1] : $INT\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0, $in1, $INT)"),
    Brick("vargv_max",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(...) {\n"
          "    local m = $INT\n"
          "    foreach (v in vargv)\n"
          "      if (typeof v == \"integer\" && v > m) m = v\n"
          "    return m\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0, $in1)"),

    # ---- more try/catch patterns ----
    Brick("try_catch_typeof",
          [Slot("in0", ANY)], [Slot("out0", STRING)],
          "local $out0 = \"\"\n"
          "  try {\n"
          "    $out0 = typeof $in0\n"
          "  } catch (_e) {\n"
          "    $out0 = \"error\"\n"
          "  }", weight=0.1),
    Brick("try_catch_clone",
          [Slot("in0", ANY)], [Slot("out0", BOOL)],
          "local $out0 = false\n"
          "  try {\n"
          "    let _c = clone $in0\n"
          "    $out0 = true\n"
          "  } catch (_e) {\n"
          "    $out0 = false\n"
          "  }", weight=0.1),

    # ---- default params containing functions / classes ----

    # default param is a table holding a lambda; call with and without override
    Brick("default_param_fn_in_table",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x, opts = {f = @(v) v + $INT, scale = $INT}) {\n"
          "    return opts.f(x) * opts.scale\n"
          "  }\n"
          "  let $out0_a = $out0_fn($in0)\n"
          "  let $out0_b = $out0_fn($in0, {f = @(v) v * $INT, scale = $INT})\n"
          "  let $out0 = $out0_a + $out0_b"),

    # default param is a table with multiple lambdas (pipeline)
    Brick("default_param_pipeline",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x, pipe = {pre = @(v) v & 0xFF,\n"
          "                                   run = @(v) v + $INT,\n"
          "                                   post = @(v) v * $INT}) {\n"
          "    return pipe.post(pipe.run(pipe.pre(x)))\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    # default param is an array of lambdas; call each in sequence
    Brick("default_param_array_of_fns",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x, ops = [@(v) v + $INT, @(v) v * $INT, @(v) v - $INT]) {\n"
          "    local r = x\n"
          "    foreach (op in ops) r = op(r)\n"
          "    return r\n"
          "  }\n"
          "  let $out0_a = $out0_fn($in0)\n"
          "  let $out0_b = $out0_fn($in0, [@(v) v, @(v) v + $INT])\n"
          "  let $out0 = $out0_a + $out0_b"),

    # default param is a class; instantiate it inside the function
    Brick("default_param_class",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x,\n"
          "      cls = class { function calc(v) { return v + $INT } }) {\n"
          "    return cls().calc(x)\n"
          "  }\n"
          "  let $out0_a = $out0_fn($in0)\n"
          "  let $out0_other = class { function calc(v) { return v * $INT } }\n"
          "  let $out0_b = $out0_fn($in0, $out0_other)\n"
          "  let $out0 = $out0_a + $out0_b"),

    # default param is a table containing a class + factory lambda
    Brick("default_param_table_class",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x, ctx = {\n"
          "      cls = class { v = 0; constructor(n) { this.v = n } },\n"
          "      wrap = @(inst) inst.v + $INT\n"
          "    }) {\n"
          "    return ctx.wrap(ctx.cls(x))\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0)"),

    # mutable default param (shared across calls - classic Squirrel gotcha)
    Brick("default_param_mutable_shared",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(x, acc = []) {\n"
          "    acc.append(x)\n"
          "    return acc.len()\n"
          "  }\n"
          "  let $out0_a = $out0_fn($in0)\n"
          "  let $out0_b = $out0_fn($in0 + 1)\n"
          "  let $out0 = $out0_a + $out0_b"),

    # default param is a nested config table with function and sub-table
    Brick("default_param_config_nested",
          [Slot("in0", INT), Slot("in1", STRING)], [Slot("out0", STRING)],
          "let $out0_fn = function(x, s, cfg = {\n"
          "      fmt = @(n, t) $\"{n}:{t}\",\n"
          "      limits = {lo = $INT, hi = $INT}\n"
          "    }) {\n"
          "    local clamped = x < cfg.limits.lo ? cfg.limits.lo\n"
          "                  : x > cfg.limits.hi ? cfg.limits.hi : x\n"
          "    return cfg.fmt(clamped, s)\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0, $in1)"),

    # default param: function stored as field, called inside body
    Brick("default_param_fn_field_call",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "let $out0_fn = function(a, b,\n"
          "      combine = {op = @(x, y) x + y, identity = $INT}) {\n"
          "    return combine.op(a, b) + combine.identity\n"
          "  }\n"
          "  let $out0_add = $out0_fn($in0, $in1)\n"
          "  let $out0_mul = $out0_fn($in0, $in1,\n"
          "    {op = @(x, y) x * y, identity = $INT})\n"
          "  let $out0 = $out0_add + $out0_mul"),

    # ---- complex nested table/array structures + nullsafe access / mutation ----

    # build {arr = [int, null, {key = x}]}, read via ?.arr?[2]?.key
    Brick("nested_table_array_nullsafe_read",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_t = {arr = [$INT, null, {key = $in0}]}\n"
          "  let $out0 = $out0_t?.arr?[2]?.key ?? $INT"),

    # same structure, modify via local ref to inner table
    Brick("nested_table_array_modify",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_t = {arr = [$INT, null, {key = $in0}]}\n"
          "  local $out0_inner = $out0_t?.arr?[2]\n"
          "  if ($out0_inner != null) $out0_inner.key += $INT\n"
          "  let $out0 = $out0_t?.arr?[2]?.key ?? $INT"),

    # array of tables, some null, sum a field via optional chain in foreach
    Brick("array_of_tables_nullable_sum",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_rows = [{v = $in0}, null, {v = $in0 + 1}, null, {v = $INT}]\n"
          "  local $out0 = 0\n"
          "  foreach (row in $out0_rows) $out0 += row?.v ?? 0"),

    # array of tables, modify element then re-read with ?.
    Brick("array_of_tables_modify_read",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "local $out0_arr = [{x = $in0, y = $INT}, {x = $in1, y = $INT}]\n"
          "  $out0_arr[0].x += $INT\n"
          "  $out0_arr[1].y *= $INT\n"
          "  let $out0 = ($out0_arr?[0]?.x ?? $INT) + ($out0_arr?[1]?.y ?? $INT)"),

    # table whose slot is an array; modify array element then read back
    Brick("table_slot_array_modify",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_state = {scores = [$in0, $INT, $INT], tag = $STRING}\n"
          "  $out0_state.scores[0] += $INT\n"
          "  $out0_state.scores[2] = $out0_state.scores[0] + $out0_state.scores[1]\n"
          "  let $out0 = $out0_state.scores.reduce(@(a, v) a + v, 0)"),

    # function field in nested table; call via nullsafe, fallback if absent
    Brick("nested_table_fn_field_call",
          [Slot("in0", INT), Slot("in1", INT)], [Slot("out0", INT)],
          "local $out0_obj = {\n"
          "    methods = {\n"
          "      add = @(a, b) a + b,\n"
          "      mul = @(a, b) a * b\n"
          "    }\n"
          "  }\n"
          "  let $out0_add_fn = $out0_obj?.methods?.add\n"
          "  let $out0_mul_fn = $out0_obj?.methods?.mul\n"
          "  let $out0 = ($out0_add_fn != null ? $out0_add_fn($in0, $in1) : $INT)\n"
          "              + ($out0_mul_fn != null ? $out0_mul_fn($in0, $in1) : $INT)"),

    # deeply nested db-style: db.users.alice.score, modify then nullsafe read
    Brick("deep_nested_db_modify",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_db = {users = {alice = {score = $INT, level = $INT},\n"
          "                           bob   = {score = $INT, level = $INT}}}\n"
          "  $out0_db.users.alice.score += $in0\n"
          "  $out0_db.users.bob.score   -= $in0\n"
          "  let $out0 = ($out0_db?.users?.alice?.score ?? $INT)\n"
          "            + ($out0_db?.users?.bob?.score ?? $INT)"),

    # array inside table inside table; read/write via multi-level index
    Brick("nested_db_array_field",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_store = {pages = [{lines = [$INT, $INT]}, {lines = [$INT, $INT]}]}\n"
          "  $out0_store.pages[0].lines[1] += $in0\n"
          "  let $out0 = $out0_store?.pages?[0]?.lines?[1] ?? $INT"),

    # array of mixed int/null/table; access table.fn if present
    Brick("mixed_array_fn_call",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_items = [$INT, null, {fn = @(x) x * $INT, tag = $STRING}, null]\n"
          "  let $out0_entry = $out0_items?[2]\n"
          "  let $out0 = $out0_entry != null ? $out0_entry.fn($in0) : $INT"),

    # conditional deep path: ternary choosing which nested field to read
    Brick("conditional_deep_path",
          [Slot("in0", INT), Slot("in1", BOOL)], [Slot("out0", INT)],
          "local $out0_cfg = {a = {val = $in0 + $INT}, b = {val = $in0 - $INT}}\n"
          "  let $out0 = $in1 ? ($out0_cfg?.a?.val ?? $INT) : ($out0_cfg?.b?.val ?? $INT)"),

    # modify nested path through a local reference, verify original changed
    Brick("modify_via_local_ref",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_root = {child = {data = {count = $INT, value = $in0}}}\n"
          "  local $out0_data = $out0_root.child.data\n"
          "  $out0_data.count += $INT\n"
          "  $out0_data.value *= $INT\n"
          "  let $out0 = $out0_root.child.data.count + $out0_root.child.data.value"),

    # array of closures stored in table; dispatch by string key
    Brick("table_of_closures_dispatch",
          [Slot("in0", INT), Slot("in1", STRING)], [Slot("out0", INT)],
          "local $out0_ops = {\n"
          "    inc  = @(x) x + $INT,\n"
          "    dec  = @(x) x - $INT,\n"
          "    dbl  = @(x) x * 2\n"
          "  }\n"
          "  let $out0_op = $out0_ops?[$in1]\n"
          "  let $out0 = $out0_op != null ? $out0_op($in0) : $in0"),

    # array of closures; call each, accumulate result
    Brick("array_of_closures_accumulate",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_fns = [@(x) x + $INT, @(x) x * $INT, @(x) x - $INT]\n"
          "  local $out0 = $in0\n"
          "  foreach (f in $out0_fns) $out0 = f($out0)"),

    # nested table with class instance as field; access instance method via ?.
    Brick("table_with_instance_field",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    v = 0\n"
          "    constructor(x) { this.v = x }\n"
          "    function get() { return this.v }\n"
          "  }\n"
          "  local $out0_holder = {obj = $out0_C($in0), tag = $STRING}\n"
          "  let $out0 = $out0_holder?.obj?.get() ?? $INT"),

    # multi-level array[i][j] access and mutation
    Brick("matrix_modify_read",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_mat = [[$INT, $INT, $INT],\n"
          "                   [$INT, $INT, $INT]]\n"
          "  $out0_mat[0][1] += $in0\n"
          "  $out0_mat[1][0] -= $in0\n"
          "  let $out0 = ($out0_mat?[0]?[1] ?? $INT) + ($out0_mat?[1]?[0] ?? $INT)"),

    # forEach over array of tables, modify matching entries, re-sum
    Brick("foreach_array_tables_modify",
          [Slot("in0", INT)], [Slot("out0", INT)],
          "local $out0_items = [{k = $STRING, v = $INT},\n"
          "                     {k = $STRING, v = $INT},\n"
          "                     {k = $STRING, v = $in0}]\n"
          "  foreach (item in $out0_items)\n"
          "    if (item.v > $INT) item.v *= $INT\n"
          "  local $out0 = 0\n"
          "  foreach (item in $out0_items) $out0 += item.v"),

    # Output sinks (no postconditions, low weight) ----
    Brick("print_int", [Slot("in0", INT)], [],
          'print($"int={$in0}\\n")', weight=0.3),
    Brick("print_str", [Slot("in0", STRING)], [],
          'print($"str={$in0}\\n")', weight=0.3),
    Brick("print_bool", [Slot("in0", BOOL)], [],
          'print($"bool={$in0}\\n")', weight=0.3),
    Brick("print_float", [Slot("in0", FLOAT)], [],
          'print($"float={$in0}\\n")', weight=0.3),
    Brick("print_array", [Slot("in0", ARRAY)], [],
          'print($"array.len={$in0.len()}\\n")', weight=0.3),
    Brick("print_table", [Slot("in0", TABLE)], [],
          'print($"table.len={$in0.len()}\\n")', weight=0.3),

    # ---- VM coverage: tail call (_OP_TAILCALL) ----
    Brick("tailcall_recursive",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function(n, acc) {\n"
          "    local x = n & 7\n"
          "    if (x <= 0) return acc\n"
          "    return callee()(x - 1, acc + x)\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0, 0)"),
    Brick("tailcall_mutual",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_f = null\n"
          "  local $out0_g = null\n"
          "  $out0_f = function(n, acc) {\n"
          "    if ((n & 7) <= 0) return acc\n"
          "    return $out0_g(n - 1, acc + 1)\n"
          "  }\n"
          "  $out0_g = function(n, acc) {\n"
          "    if ((n & 7) <= 0) return acc\n"
          "    return $out0_f(n - 1, acc + 2)\n"
          "  }\n"
          "  let $out0 = $out0_f($in0 & 7, 0)"),

    # ---- VM coverage: _set metamethod ----
    Brick("meta_set",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _store = null\n"
          "    constructor() { this._store = {} }\n"
          "    function _set(idx, val) { this._store[idx] <- val }\n"
          "    function _get(idx) { return idx in this._store ? this._store[idx] : 0 }\n"
          "  }\n"
          "  let $out0_obj = $out0_C()\n"
          "  $out0_obj.mykey = $in0 + $INT\n"
          "  let $out0 = $out0_obj.mykey"),

    # ---- VM coverage: null-safe call _OP_NULLCALL ----
    Brick("nullsafe_call_func",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = @(x) x + $INT\n"
          "  let $out0_null = null\n"
          "  let $out0_a = $out0_fn?($in0) ?? $INT\n"
          "  let $out0_b = $out0_null?($in0) ?? $INT\n"
          "  let $out0 = $out0_a + $out0_b"),
    Brick("nullsafe_call_method",
          [Slot("in0", TABLE)],
          [Slot("out0", INT)],
          "let $out0_fn = $in0?.len\n"
          "  let $out0 = $out0_fn?() ?? $INT"),

    # ---- VM coverage: slot increment/decrement (_OP_INC, _OP_PINC on slots) ----
    Brick("slot_inc_prefix",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  let $out0_a = ++$out0_t.x\n"
          "  let $out0 = $out0_a + $out0_t.x"),
    Brick("slot_inc_postfix",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  let $out0_a = $out0_t.x++\n"
          "  let $out0 = $out0_a + $out0_t.x"),
    Brick("slot_dec_prefix",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  let $out0_a = --$out0_t.x\n"
          "  let $out0 = $out0_a + $out0_t.x"),
    Brick("slot_dec_postfix",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {x = $in0}\n"
          "  let $out0_a = $out0_t.x--\n"
          "  let $out0 = $out0_a + $out0_t.x"),
    Brick("slot_arr_inc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_a = [$in0, $in0 + $INT]\n"
          "  let $out0_before = $out0_a[0]++\n"
          "  ++$out0_a[1]\n"
          "  let $out0 = $out0_before + $out0_a[0] + $out0_a[1]"),

    # ---- VM coverage: static class members (_OP_NEWSLOTA with static flag) ----
    Brick("class_static_member",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    static sval = $INT\n"
          "    _v = 0\n"
          "    constructor(x) { this._v = x }\n"
          "    function getSum() { return this._v + this.getclass().sval }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0 = $out0_C.sval + $out0_obj.getSum()"),
    Brick("class_static_func",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    static sval = $INT\n"
          "    static function compute(x) { return x * 2 }\n"
          "  }\n"
          "  let $out0 = $out0_C.compute($out0_C.sval) + $in0"),

    # ---- VM coverage: _div and _modulo metamethods ----
    Brick("meta_div",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _div(o) { return this.getclass()(o.xv != 0 ? this.xv / o.xv : 0) }\n"
          "  }\n"
          "  let $out0_r = $out0_C($in0) / $out0_C($in1 | 1)\n"
          "  let $out0 = $out0_r.xv"),
    Brick("meta_modulo",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _modulo(o) { return this.getclass()(o.xv != 0 ? this.xv % o.xv : 0) }\n"
          "  }\n"
          "  let $out0_r = $out0_C($in0) % $out0_C($in1 | 1)\n"
          "  let $out0 = $out0_r.xv"),

    # ---- VM coverage: _OP_LOADROOT (:: syntax) ----
    Brick("root_access",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "getroottable().$out0_rv <- $in0 + $INT\n"
          "  let $out0 = ::$out0_rv"),

    # ---- VM coverage: foreach on class object ----
    Brick("foreach_class",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    a = $INT\n"
          "    b = $INT\n"
          "    function getv() { return $INT }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  foreach (_k, _v in $out0_C) { $out0++ }"),

    # ---- VM coverage: _newslot metamethod ----
    Brick("meta_newslot",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _store = null\n"
          "    constructor() { this._store = {} }\n"
          "    function _newslot(key, val) { this._store[key] <- val }\n"
          "    function _get(idx) { return idx in this._store ? this._store[idx] : 0 }\n"
          "  }\n"
          "  let $out0_obj = $out0_C()\n"
          "  $out0_obj.zz <- $in0 + $INT\n"
          "  let $out0 = $out0_obj.zz"),

    # ---- VM coverage: delete operator (_OP_DELETE) ----
    Brick("delete_slot",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {x = $in0 + $INT, y = $in0}\n"
          "  let $out0_d = delete $out0_t.x\n"
          "  let $out0 = $out0_d + $out0_t.y"),

    # ---- VM coverage: _delslot metamethod ----
#    Brick("meta_delslot",
#          [Slot("in0", INT)],
#          [Slot("out0", INT)],
#          "let $out0_delegate = {\n"
#          "    function _delslot(key) {\n"
#          "      let val = this.rawget(key, 0)\n"
#          "      this.rawdelete(key)\n"
#          "      return val + $INT\n"
#          "    }\n"
#          "  }\n"
#          "  let $out0_t = {x = $in0, y = $INT}\n"
#          "  $out0_t.setdelegate($out0_delegate)\n"
#          "  let $out0_d = delete $out0_t.x\n"
#          "  let $out0 = $out0_d + $out0_t.y"),

    # ===========================================================================
    # Round 1: Mixed-type arithmetic (Cat A) + Comparison operators (Cat B)
    # Target: sqvm.cpp lines 67, 97-128, 231-401
    # ===========================================================================

    # ---- Cat A: Mixed float+int arithmetic via _ARITH_ macro (line 67) ----
    # Direct float+int without .tofloat() conversion triggers OT_FLOAT|OT_INTEGER
    Brick("mixed_float_int_add_direct",
          [Slot("in0", FLOAT), Slot("in1", INT)],
          [Slot("out0", FLOAT)],
          "let $out0 = $in0 + $in1"),
    Brick("mixed_int_float_add_direct",
          [Slot("in0", INT), Slot("in1", FLOAT)],
          [Slot("out0", FLOAT)],
          "let $out0 = $in0 + $in1"),
    Brick("mixed_float_int_sub_direct",
          [Slot("in0", FLOAT), Slot("in1", INT)],
          [Slot("out0", FLOAT)],
          "let $out0 = $in0 - $in1"),
    Brick("mixed_float_int_mul_direct",
          [Slot("in0", FLOAT), Slot("in1", INT)],
          [Slot("out0", FLOAT)],
          "let $out0 = $in0 * $in1"),
    Brick("mixed_float_int_div_direct",
          [Slot("in0", FLOAT), Slot("in1", INT)],
          [Slot("out0", FLOAT)],
          "let $out0 = $in1 != 0 ? $in0 / $in1 : $FLOAT"),

    # ---- Cat A: Integer division via ARITH_OP (lines 97-100) ----
    # Compound /= on table field goes through _OP_COMPARITH -> DerefInc -> ARITH_OP
    Brick("table_field_diveq_int",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0 * $INT}\n"
          "  $out0_t.x /= ($in0 | 1)\n"
          "  let $out0 = $out0_t.x"),
    Brick("table_field_diveq_int_unsafe",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  $out0_t.x /= $in1\n"
          "  let $out0 = $out0_t.x"),
    Brick("array_elem_diveq_int",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0_a = [$in0 * $INT]\n"
          "  $out0_a[0] /= ($in1 | 1)\n"
          "  let $out0 = $out0_a[0]"),

    # ---- Cat A: Float arithmetic via ARITH_OP (lines 110-128) ----
    # Compound assignment on float table field goes through DerefInc -> ARITH_OP
    Brick("table_field_addeq_float",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  $out0_t.x += $FLOAT\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("table_field_subeq_float",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  $out0_t.x -= $FLOAT\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("table_field_muleq_float",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  $out0_t.x *= $FLOAT\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("table_field_diveq_float",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  let $out0_d = $in0 == 0.0 ? 1.0 : $in0\n"
          "  $out0_t.x /= $out0_d\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("table_field_diveq_float_unsafe",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  $out0_t.x /= $in1\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("table_field_modeq_float",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  let $out0_d = $in0 == 0.0 ? 1.0 : $in0\n"
          "  $out0_t.x %= $out0_d\n"
          "  let $out0 = $out0_t.x.tointeger()"),

    # ---- Cat A: Float slot increment (covers ARITH_OP float + via DerefInc) ----
    Brick("float_slot_inc",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  ++$out0_t.x\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("float_slot_postinc",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  $out0_t.x++\n"
          "  let $out0 = $out0_t.x.tointeger()"),
    Brick("float_slot_dec",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = $in0}\n"
          "  --$out0_t.x\n"
          "  let $out0 = $out0_t.x.tointeger()"),

    # ---- Cat A: Mixed type compound assignment on table fields ----
    Brick("table_field_addeq_mixed",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_t = {x = 1.5}\n"
          "  $out0_t.x += $in0\n"
          "  let $out0 = $out0_t.x.tointeger()"),

    # ---- Cat B: Cross-type int vs float comparison in ObjCmp (lines 289-298) ----
    # Both operands must be register values (not literals) to go through _OP_JCMP
    Brick("cmp_int_lt_float",
          [Slot("in0", INT), Slot("in1", FLOAT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 < $in1"),
    Brick("cmp_int_gt_float",
          [Slot("in0", INT), Slot("in1", FLOAT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 > $in1"),
    Brick("cmp_int_le_float",
          [Slot("in0", INT), Slot("in1", FLOAT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 <= $in1"),
    Brick("cmp_int_ge_float",
          [Slot("in0", INT), Slot("in1", FLOAT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 >= $in1"),
    Brick("cmp_float_lt_int",
          [Slot("in0", FLOAT), Slot("in1", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 < $in1"),
    Brick("cmp_float_gt_int",
          [Slot("in0", FLOAT), Slot("in1", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 > $in1"),

    # ---- Cat B: Float var vs int literal in ObjCmpI (lines 314-317) ----
    # Compiler emits JCMPI for small integer literals
    Brick("cmp_float_gt_intlit",
          [Slot("in0", FLOAT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 > 0"),
    Brick("cmp_float_lt_intlit",
          [Slot("in0", FLOAT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 < 1"),
    Brick("cmp_float_le_intlit",
          [Slot("in0", FLOAT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <= 0 ? 1 : 0"),

    # ---- Cat B: Int var vs float literal in ObjCmpF (lines 331-334) ----
    # Compiler emits JCMPF for float literals
    Brick("cmp_int_gt_floatlit",
          [Slot("in0", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 > 0.5"),
    Brick("cmp_int_lt_floatlit",
          [Slot("in0", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 < 1.5"),
    Brick("cmp_int_ge_floatlit",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 >= 2.5 ? 1 : 0"),

    # ---- Cat B: Null vs float literal in ObjCmpF (line 335) ----
    Brick("cmp_null_lt_floatlit",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_n = $in0 > 99999999 ? null : $in0\n"
          "  local $out0 = 0\n"
          "  try { $out0 = $out0_n < 3.14 ? 1 : 0 } catch (_e) {}", weight=0.3),

    # ---- Cat B: Table comparison via _cmp delegate (lines 264-265) ----
    # Covers OT_TABLE case label in ObjCmp (line 264)
#    Brick("cmp_table_via_cmp_delegate",
#          [Slot("in0", INT), Slot("in1", INT)],
#          [Slot("out0", BOOL)],
#          "let $out0_delegate = {\n"
#          "    function _cmp(other) { return this.val <=> other.val }\n"
#          "  }\n"
#          "  let $out0_a = {val = $in0}\n"
#          "  $out0_a.setdelegate($out0_delegate)\n"
#          "  let $out0_b = {val = $in1}\n"
#          "  $out0_b.setdelegate($out0_delegate)\n"
#          "  let $out0 = $out0_a < $out0_b"),

    # ---- Cat B: Negate table/instance via _unm metamethod (lines 231-232) ----
    Brick("neg_instance_unm",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(x) { this.xv = x }\n"
          "    function _unm() { return this.getclass()(-this.xv) }\n"
          "  }\n"
          "  let $out0_r = -$out0_C($in0)\n"
          "  let $out0 = $out0_r.xv"),
#    Brick("neg_table_unm_delegate",
#          [Slot("in0", INT)],
#          [Slot("out0", INT)],
#          "let $out0_delegate = {\n"
#          "    function _unm() { return {val = -(this?.val ?? 0)} }\n"
#          "  }\n"
#          "  let $out0_t = {val = $in0}\n"
#          "  $out0_t.setdelegate($out0_delegate)\n"
#          "  let $out0 = (-$out0_t)?.val ?? $INT"),
#
    # ---- Cat B: Three-way comparison <=> on mixed types (line 398 in CMP_OP) ----
    Brick("threeway_int_int",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <=> $in1"),
    Brick("threeway_float_float",
          [Slot("in0", FLOAT), Slot("in1", FLOAT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <=> $in1"),
    Brick("threeway_string_string",
          [Slot("in0", STRING), Slot("in1", STRING)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <=> $in1"),
    Brick("threeway_int_float",
          [Slot("in0", INT), Slot("in1", FLOAT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <=> $in1"),
    Brick("threeway_bool_bool",
          [Slot("in0", BOOL), Slot("in1", BOOL)],
          [Slot("out0", INT)],
          "let $out0 = $in0 <=> $in1"),

    # ===========================================================================
    # Round 2: Generators/foreach (Cat C+D)
    # Target: sqvm.cpp lines 773-788, 1623-1634
    # ===========================================================================

    # ---- Generator foreach: basic (covers FOREACH_OP gen path + POSTFOREACH) ----
    Brick("foreach_generator_basic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_iv = $in0\n"
          "  let $out0_gen = (function() {\n"
          "    yield $out0_iv\n"
          "    yield $out0_iv + 1\n"
          "    yield $out0_iv + 2\n"
          "  })()\n"
          "  local $out0 = 0\n"
          "  foreach (_idx, val in $out0_gen) { $out0 += val }"),
    Brick("foreach_generator_keys",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_iv = $in0\n"
          "  let $out0_gen = (function() {\n"
          "    yield $out0_iv * $INT\n"
          "    yield $out0_iv + $INT\n"
          "  })()\n"
          "  local $out0 = 0\n"
          "  foreach (idx, _val in $out0_gen) { $out0 += idx }"),
    Brick("foreach_generator_string",
          [],
          [Slot("out0", STRING)],
          "let $out0_gen = (function() {\n"
          "    yield $STRING\n"
          "    yield $STRING\n"
          "    yield $STRING\n"
          "  })()\n"
          "  local $out0 = \"\"\n"
          "  foreach (_idx, val in $out0_gen) { $out0 += val }"),
    Brick("foreach_generator_single",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_gen = (function() { yield $in0 })()\n"
          "  local $out0 = 0\n"
          "  foreach (_idx, val in $out0_gen) { $out0 = val }"),
    Brick("foreach_generator_empty",
          [],
          [Slot("out0", INT)],
          "let $out0_gen = (function() {\n"
          "    let _x = $INT\n"
          "  })()\n"
          "  local $out0 = $INT\n"
          "  foreach (_idx, _val in $out0_gen) { $out0 = 0 }"),
    Brick("foreach_generator_loop",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_iv = $in0\n"
          "  let $out0_gen = (function() {\n"
          "    for (local i = 0; i < 5; i++) yield $out0_iv + i\n"
          "  })()\n"
          "  local $out0 = 0\n"
          "  foreach (_idx, val in $out0_gen) { $out0 += val }"),
    Brick("foreach_generator_break",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_iv = $in0\n"
          "  let $out0_gen = (function() {\n"
          "    for (local i = 0; i < 10; i++) yield $out0_iv + i\n"
          "  })()\n"
          "  local $out0 = 0\n"
          "  foreach (_idx, val in $out0_gen) {\n"
          "    $out0 += val\n"
          "    if ($out0 > ($out0_iv + $INT)) break\n"
          "  }", weight=0.5),

    # ---- Dead generator foreach (covers eDead path in FOREACH_OP line 774) ----
    Brick("foreach_dead_generator",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_gen = (function() { yield $in0 })()\n"
          "  let $out0_v = resume $out0_gen\n"
          "  local $out0 = $out0_v\n"
          "  foreach (_idx, val in $out0_gen) { $out0 += val }"),

    # ===========================================================================
    # Round 3: String indexing, Clone, NewSlot, DeleteSlot, Set/Get metamethods,
    #          callee(), immutable errors
    # Target: sqvm.cpp lines 2108-2120, 2287-2319, 2322-2382, 2387-2427,
    #         2214-2248, 2174-2211, 2250-2285, 1395-1401
    # ===========================================================================

    # ---- String numeric indexing (GetImpl lines 2108-2120) ----
    # Covers OT_STRING case in GetImpl: sq_isnumeric, positive & negative indexing
    Brick("string_index_positive",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "let $out0 = $in0.len() > 0 ? $in0[0] : 0"),
    Brick("string_index_negative",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "let $out0 = $in0.len() > 0 ? $in0[-1] : 0"),
    Brick("string_index_mid",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "let $out0_s = $in0\n"
          "  let $out0 = $out0_s.len() > 1 ? $out0_s[$out0_s.len() / 2] : ($out0_s.len() > 0 ? $out0_s[0] : 0)"),
    Brick("string_index_out_of_range",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { $out0 = $in0[999] } catch (_e) { $out0 = -1 }"),

    # ---- Clone on table (lines 2292-2307) ----
    # Covers OT_TABLE clone + _cloned metamethod path
    Brick("clone_table_basic",
          [Slot("in0", INT)],
          [Slot("out0", TABLE)],
          "let $out0_src = {a = $in0, b = $in0 + 1}\n"
          "  let $out0 = clone $out0_src"),
#    Brick("clone_table_with_cloned_mm",
#          [Slot("in0", INT)],
#          [Slot("out0", TABLE)],
#          "let $out0_d = { function _cloned(_orig) { this.cloned <- true } }\n"
#          "  let $out0_src = {val = $in0}\n"
#          "  $out0_src.setdelegate($out0_d)\n"
#          "  let $out0 = clone $out0_src"),
#
    # ---- Clone on instance (lines 2295-2307) ----
    Brick("clone_instance",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(v) { this.xv = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  let $out0_copy = clone $out0_inst\n"
          "  let $out0 = $out0_copy.xv"),
    Brick("clone_instance_with_cloned",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    xv = 0\n"
          "    constructor(v) { this.xv = v }\n"
          "    function _cloned(orig) { this.xv = orig.xv * 2 }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  let $out0_copy = clone $out0_inst\n"
          "  let $out0 = $out0_copy.xv"),

    # ---- Clone on class (error path, line 2312-2314) ----
    Brick("clone_class_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class { x = $in0 }\n"
          "  local $out0 = 0\n"
          "  try { clone $out0_C; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- Clone on array (line 2308-2310) ----
    Brick("clone_array",
          [Slot("in0", INT)],
          [Slot("out0", ARRAY)],
          "let $out0 = clone [$in0, $in0 + 1, $in0 + 2]"),

    # ---- Clone on primitive (default path, lines 2315-2317) ----
    Brick("clone_primitive",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = clone $in0"),

    # ---- NewSlot on table with _newslot metamethod (lines 2332-2349) ----
#    Brick("newslot_table_with_delegate",
#          [Slot("in0", INT)],
#          [Slot("out0", TABLE)],
#          "let $out0_d = { function _newslot(key, val) { this[key] = val * 2 } }\n"
#          "  let $out0 = {}\n"
#          "  $out0.setdelegate($out0_d)\n"
#          "  $out0.newkey <- $in0"),
    Brick("newslot_table_basic",
          [Slot("in0", INT)],
          [Slot("out0", TABLE)],
          "let $out0 = {}\n"
          "  $out0.x <- $in0\n"
          "  $out0.y <- $in0 + 1"),

    # ---- NewSlot on instance without _newslot (line 2361) ----
    Brick("newslot_instance_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class { x = 0 }\n"
          "  let $out0_inst = $out0_C()\n"
          "  local $out0 = 0\n"
          "  try { $out0_inst.newfield <- $in0; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- NewSlot on instance with _newslot metamethod (lines 2351-2359) ----
    Brick("newslot_instance_with_mm",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _data = null\n"
          "    constructor() { this._data = {} }\n"
          "    function _newslot(key, val) { this._data[key] <- val }\n"
          "  }\n"
          "  let $out0_inst = $out0_C()\n"
          "  $out0_inst.dynamicKey <- $in0\n"
          "  let $out0 = $out0_inst._data?.dynamicKey ?? 0"),

    # ---- NewSlot on locked class (line 2366-2368) ----
    Brick("newslot_locked_class_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class { x = 0 }\n"
          "  let $out0_inst = $out0_C()\n"
          "  local $out0 = 0\n"
          "  try { $out0_C.newprop <- $in0; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- NewSlot duplicate property on class (lines 2370-2373) ----
    Brick("newslot_class_duplicate_prop",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class { x = 0 }\n"
          "  local $out0 = 0\n"
          "  try { $out0_C.x <- $in0; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- DeleteSlot on table (lines 2405-2412) ----
    Brick("delete_table_slot",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {a = $in0, b = $in0 + 1}\n"
          "  let $out0_deleted = delete $out0_t.a\n"
          "  let $out0 = $out0_deleted"),
    Brick("delete_table_missing_key",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {a = $in0}\n"
          "  local $out0 = 0\n"
          "  try { delete $out0_t.missing; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- DeleteSlot on instance (error path, lines 2414-2416) ----
    Brick("delete_instance_slot_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class { x = 0 }\n"
          "  let $out0_inst = $out0_C()\n"
          "  local $out0 = 0\n"
          "  try { delete $out0_inst.x; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- DeleteSlot on non-delegable type (default path, lines 2422-2424) ----
    Brick("delete_nondelegable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { delete $in0[\"x\"]; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- DeleteSlot on immutable table (lines 2389-2391) ----
    Brick("delete_immutable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = freeze({a = $in0})\n"
          "  local $out0 = 0\n"
          "  try { delete $out0_t.a; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- DeleteSlot with _delslot metamethod (lines 2400-2402) ----
#    Brick("delete_with_delslot_mm",
#          [Slot("in0", INT)],
#          [Slot("out0", INT)],
#          "let $out0_d = {\n"
#          "    function _delslot(key) { return key.len() }\n"
#          "  }\n"
#          "  let $out0_t = {a = $in0}\n"
#          "  $out0_t.setdelegate($out0_d)\n"
#          "  let $out0 = delete $out0_t.a"),
#
    # ---- Set on immutable (lines 2216-2218) ----
    Brick("set_immutable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = freeze({a = $in0})\n"
          "  local $out0 = 0\n"
          "  try { $out0_t.a = 999; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- Set on array with non-numeric key (line 2229) ----
    Brick("set_array_nonnumeric_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_a = [$in0]\n"
          "  local $out0 = 0\n"
          "  try { $out0_a[\"x\"] = 1; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- Set on non-settable type (line 2237) ----
    Brick("set_nonsettable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { $in0[\"x\"] = 1; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- FallBackGet with _get metamethod (lines 2187-2205) ----
    Brick("instance_get_metamethod",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _data = null\n"
          "    constructor() { this._data = {secret = $INT} }\n"
          "    function _get(key) { return this._data?[key] }\n"
          "  }\n"
          "  let $out0_inst = $out0_C()\n"
          "  let $out0 = $out0_inst.secret"),
    Brick("instance_get_mm_not_found",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    function _get(_key) { throw null }\n"
          "  }\n"
          "  let $out0_inst = $out0_C()\n"
          "  local $out0 = 0\n"
          "  try { $out0 = $out0_inst.noexist } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- FallBackGet via table delegation (lines 2177-2184) ----
#    Brick("table_delegate_get",
#          [Slot("in0", INT)],
#          [Slot("out0", INT)],
#          "let $out0_base = {inherited_val = $in0 + $INT}\n"
#          "  let $out0_t = {}\n"
#          "  $out0_t.setdelegate($out0_base)\n"
#          "  let $out0 = $out0_t.inherited_val"),
#
    # ---- FallBackSet with _set metamethod (lines 2258-2278) ----
    Brick("instance_set_metamethod",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _data = null\n"
          "    constructor() { this._data = {} }\n"
          "    function _set(key, val) { this._data[key] <- val }\n"
          "  }\n"
          "  let $out0_inst = $out0_C()\n"
          "  $out0_inst.dynamic = $in0\n"
          "  let $out0 = $out0_inst._data?.dynamic ?? 0"),
    Brick("instance_set_mm_fail",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    function _set(_key, _val) { throw \"no set\" }\n"
          "  }\n"
          "  let $out0_inst = $out0_C()\n"
          "  local $out0 = 0\n"
          "  try { $out0_inst.anything = $in0; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- FallBackSet via table delegation (lines 2253-2256) ----
#    Brick("table_delegate_set",
#          [Slot("in0", INT)],
#          [Slot("out0", INT)],
#          "let $out0_base = {shared_val = 0}\n"
#          "  let $out0_t = {}\n"
#          "  $out0_t.setdelegate($out0_base)\n"
#          "  $out0_t.shared_val = $in0\n"
#          "  let $out0 = $out0_base.shared_val"),

    # ---- callee() - _OP_LOADCALLEE (lines 1395-1401) ----
    Brick("callee_basic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function($out0_x) {\n"
          "    let $out0_self = callee()\n"
          "    return $out0_x > 0 ? $out0_self($out0_x - 1) + 1 : 0\n"
          "  }\n"
          "  let $out0 = $out0_fn($in0 % 5 + 1)", weight=0.5),

    # ---- Set on array out-of-range (line 2230-2232) ----
    Brick("set_array_out_of_range",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_a = [$in0]\n"
          "  local $out0 = 0\n"
          "  try { $out0_a[999] = 1; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- NewSlot on non-indexable type (default path, lines 2377-2380) ----
    Brick("newslot_nonindexable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    local $out0_v = $in0\n"
          "    $out0_v.newkey <- 1\n"
          "    $out0 = 1\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- NewSlot on immutable table (lines 2324-2327) ----
    Brick("newslot_immutable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = freeze({})\n"
          "  local $out0 = 0\n"
          "  try { $out0_t.x <- $in0; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- Clone on table then modify (verify deep clone) ----
    Brick("clone_table_modify",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_orig = {x = $in0}\n"
          "  let $out0_copy = clone $out0_orig\n"
          "  $out0_copy.x = $in0 + 100\n"
          "  let $out0 = $out0_orig.x"),

    # ---- Get on class (lines 2102-2107) ----
    Brick("get_class_member",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    static sval = $INT\n"
          "  }\n"
          "  let $out0 = $out0_C.sval"),

    # ---- Stack overflow ----
    Brick("loadcallee_stack_overflow",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    function recurse(n) {\n"
          "      local _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18 = (n > 0 ? recurse(n - 1) + 1 : 0), _19\n"
          "      _18++\n"
          "    }\n"
          "  }\n"
          "  local $out0 = 123\n"
          "  try { $out0 = $out0_C().recurse(100000) } catch (e) { println($\"stack: {e}\")}\n"),


    # ===========================================================================
    # Round 4: LOADCALLEE, FREEZE opcode, dynamic-key Set/Get, RelocateOuters,
    #          AAT_FLOAT, table Set via _OP_SET, NewSlot delegate path
    # Target: sqvm.cpp lines 1395-1401, 1729-1732, 1495-1498, 2222-2224,
    #         2225-2227, 2241-2244, 2332-2347, 2575-2581
    # ===========================================================================

    # ---- _OP_LOADCALLEE: class method self-recursion (lines 1395-1401) ----
    # Emitted when a named function references itself and the name is not a local var
    # This happens in class methods: function bar(x) { ... bar(x-1) ... }
    Brick("loadcallee_class_method",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    function recurse(n) {\n"
          "      return n > 0 ? recurse(n - 1) + 1 : 0\n"
          "    }\n"
          "  }\n"
          "  let $out0 = $out0_C().recurse($in0 % 5 + 1)"),
    Brick("loadcallee_class_factorial",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    function fact(n) {\n"
          "      return n <= 1 ? 1 : n * fact(n - 1)\n"
          "    }\n"
          "  }\n"
          "  let $out0 = $out0_C().fact($in0 % 7 + 1)", weight=0.5),

    # ---- _OP_FREEZE opcode: auto-freeze with pragma (lines 1729-1732) ----
    # Requires #allow-auto-freeze pragma; 'let' with table/array/class literal
    # Since we can't add pragmas mid-script, we use the freeze() function instead
    # which is already covered. _OP_FREEZE is only via auto-freeze pragma.
    # Skip - not coverable from our test harness without pragma support.

    # ---- AAT_FLOAT in _OP_APPENDARRAY (lines 1495-1498) ----
    # Compiler may emit AAT_FLOAT for float literals in array construction
    # Need array literal with float constants
    Brick("array_float_literal",
          [],
          [Slot("out0", ARRAY)],
          "let $out0 = [1.0, 2.0, 3.0, -1.5, 0.0]"),
    Brick("array_mixed_float_int",
          [Slot("in0", INT)],
          [Slot("out0", ARRAY)],
          "let $out0 = [1.0, $in0, 2.5, $in0 + 1, -3.14]"),

    # ---- Set() with OT_TABLE via dynamic key (lines 2222-2224) ----
    # _OP_SET is used for dynamic (non-literal) keys; _OP_SET_LITERAL is for string literals
    Brick("table_set_dynamic_key",
          [Slot("in0", INT), Slot("in1", STRING)],
          [Slot("out0", TABLE)],
          "let $out0 = {}\n"
          "  $out0[$in1] <- $in0\n"
          "  $out0[$in1] = $in0 + 1"),
    Brick("table_set_dynamic_int_key",
          [Slot("in0", INT)],
          [Slot("out0", TABLE)],
          "let $out0_key = \"k\" + $in0\n"
          "  let $out0 = {}\n"
          "  $out0[$out0_key] <- $in0\n"
          "  $out0[$out0_key] = $in0 * 2"),
    Brick("table_get_dynamic_key",
          [Slot("in0", TABLE), Slot("in1", STRING)],
          [Slot("out0", INT)],
          "let $out0 = ($in0?[$in1] ?? 0).tointeger()"),

    # ---- Set() with OT_INSTANCE via dynamic key (lines 2225-2227) ----
    Brick("instance_set_dynamic_key",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = 0\n"
          "    constructor(v) { this.x = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  let $out0_key = \"x\"\n"
          "  $out0_inst[$out0_key] = $in0 + 1\n"
          "  let $out0 = $out0_inst.x"),

    # ---- FallBackSet via Set() (lines 2241-2244) ----
    # This is the path: Set() -> OT_TABLE Set() fails -> FallBackSet()
    # Table.Set() fails when key doesn't exist (returns false for new keys!)
    # Wait - Table::Set returns false only if key not found. NewSlot is for new keys.
    # Actually: OT_TABLE case in Set() calls _table(self)->Set(key,val) which returns false
    # if the key doesn't exist. Then it falls through to FallBackSet.
    # So: Set on a table with a key that doesn't exist (via dynamic key).
    Brick("table_set_missing_dynamic_key",
          [Slot("in0", INT), Slot("in1", STRING)],
          [Slot("out0", INT)],
          "let $out0_t = {a = $in0}\n"
          "  local $out0 = 0\n"
          "  try { $out0_t[$in1] = 999; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.4),

    # ---- NewSlot OT_TABLE with delegate and _newslot (lines 2332-2347) ----
    # Key insight: the key must NOT already exist in the table for the delegate path
    # AND the table must have a delegate with _newslot metamethod
#    Brick("newslot_table_delegate_mm",
#          [Slot("in0", INT)],
#          [Slot("out0", TABLE)],
#          "let $out0_log = []\n"
#          "  let $out0_d = {\n"
#          "    function _newslot(key, val) {\n"
#          "      $out0_log.append($\"ns:{key}={val}\")\n"
#          "    }\n"
#          "  }\n"
#          "  let $out0 = {}\n"
#          "  $out0.setdelegate($out0_d)\n"
#          "  $out0.newfield <- $in0", weight=0.5),
#
    # ---- RelocateOuters: deep recursion with closures (lines 2575-2581) ----
    # Stack growth triggers RelocateOuters when open outers exist
    Brick("relocate_outers_deep",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_captured = $in0\n"
          "  let $out0_fn = @() $out0_captured\n"
          "  let $out0_deep = function($out0_n) {\n"
          "    local $out0_a = 0, $out0_b = 0, $out0_c = 0, $out0_d = 0\n"
          "    $out0_a = $out0_n; $out0_b = $out0_n + 1; $out0_c = $out0_n + 2; $out0_d = $out0_n + 3\n"
          "    return $out0_n > 0 ? callee()($out0_n - 1) + $out0_a : $out0_d\n"
          "  }\n"
          "  let $out0 = $out0_deep(50) + $out0_fn()", weight=0.3),

    # ---- _OP_BWNOT on non-integer (line 1583) ----
    # Error path: bitwise NOT on non-integer type
    Brick("bwnot_nonint_error",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { $out0 = ~$in0 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- _OP_INSTANCEOF on non-class (lines 1546-1548) ----
    Brick("instanceof_nonclass_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { $out0 = ($in0 instanceof $in0) ? 1 : 0 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- _OP_NULLCOALESCE (lines 1567-1572) ----
    Brick("nullcoalesce_nonnull",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = $in0 ?? $INT"),
    Brick("nullcoalesce_null",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_v = $in0 > 99999999 ? null : $in0\n"
          "  let $out0 = $out0_v ?? $INT"),

    # ---- _OP_EXISTS (line 1544) ----
    Brick("exists_in_table",
          [Slot("in0", TABLE)],
          [Slot("out0", BOOL)],
          "let $out0 = \"a\" in $in0"),
    Brick("exists_in_array",
          [Slot("in0", INT)],
          [Slot("out0", BOOL)],
          "let $out0 = $in0 in [$INT, $INT, $in0]"),

    # ---- _OP_GETBASE in derived class (line 1662-1663) ----
    Brick("getbase_derived",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_Base = class {\n"
          "    function getval() { return $INT }\n"
          "  }\n"
          "  let $out0_Derived = class($out0_Base) {\n"
          "    function getval() { return base.getval() + $in0 }\n"
          "  }\n"
          "  let $out0 = $out0_Derived().getval()"),

    # ---- _OP_THROW (line 1656) ----
    Brick("throw_and_catch",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { throw $in0 } catch (e) { $out0 = e }"),

    # ---- _OP_LOADROOT (lines 1391-1393) ----
    Brick("loadroot_access",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0 = ::print != null ? $in0 : 0"),

    # ---- _OP_SETI / _OP_SETK (lines 1255-1257) ----
    # _OP_SETI is for integer index set, _OP_SETK for literal key set
    Brick("array_seti",
          [Slot("in0", INT)],
          [Slot("out0", ARRAY)],
          "let $out0 = [$in0, $in0 + 1, $in0 + 2]\n"
          "  $out0[0] = $in0 * 10\n"
          "  $out0[1] = $in0 * 20"),

    # ===========================================================================
    # Round 5: DerefInc on instance, wrong-arg-count errors, _OP_GETBASE null,
    #          static memo auto for mutable container, misc error paths
    # Target: sqvm.cpp lines 665-671, 522-528, 558-565, 1665-1667, 1701-1707
    # ===========================================================================

    # ---- DerefInc on instance field (lines 665-671) ----
    # Compound assignment on an instance field: inst.x += val
    Brick("instance_field_addeq",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = 0\n"
          "    constructor(v) { this.x = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  $out0_inst.x += $INT\n"
          "  let $out0 = $out0_inst.x"),
    Brick("instance_field_subeq",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = 0\n"
          "    constructor(v) { this.x = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  $out0_inst.x -= $INT\n"
          "  let $out0 = $out0_inst.x"),
    Brick("instance_field_muleq",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = 1\n"
          "    constructor(v) { this.x = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  $out0_inst.x *= $INT\n"
          "  let $out0 = $out0_inst.x"),
    Brick("instance_field_inc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = 0\n"
          "    constructor(v) { this.x = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  $out0_inst.x++\n"
          "  let $out0 = $out0_inst.x"),
    Brick("instance_field_postinc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = 0\n"
          "    constructor(v) { this.x = v }\n"
          "  }\n"
          "  let $out0_inst = $out0_C($in0)\n"
          "  let $out0 = $out0_inst.x++"),

    # ---- Wrong arg count: non-varargs function (lines 558-565) ----
    # Call a function with wrong number of args via dynamic dispatch
    Brick("wrong_argcount_too_few",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fns = {\n"
          "    add = function(a, b) { return a + b }\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  try { $out0 = $out0_fns[\"add\"]($in0) } catch (_e) { $out0 = -1 }", weight=0.3),
    Brick("wrong_argcount_too_many",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fns = {\n"
          "    single = function(a) { return a }\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  try { $out0 = $out0_fns[\"single\"]($in0, $in0, $in0) } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- Wrong arg count: varargs with too few required (lines 522-528) ----
    Brick("wrong_argcount_varargs",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fns = {\n"
          "    vaarg = function(a, b, ...) { return a + b }\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  try { $out0 = $out0_fns[\"vaarg\"]($in0) } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- DerefInc on immutable (line 660-661) ----
    Brick("derefinc_immutable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = freeze({x = $in0})\n"
          "  local $out0 = 0\n"
          "  try { $out0_t.x += 1; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- foreach on class members (lines 738-741) ----
    Brick("foreach_class_members",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    x = $in0\n"
          "    y = $in0 + 1\n"
          "    static z = $INT\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  foreach (_k, v in $out0_C) { $out0 += (type(v) == \"integer\") ? v : 0 }"),

    # ---- foreach on instance with _nexti (lines 743-758) ----
    Brick("foreach_instance_nexti",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _keys = null\n"
          "    _vals = null\n"
          "    constructor(k, v) { this._keys = k; this._vals = v }\n"
          "    function _nexti(prev) {\n"
          "      if (prev == null) return this._keys.len() > 0 ? 0 : null\n"
          "      return prev + 1 < this._keys.len() ? prev + 1 : null\n"
          "    }\n"
          "    function _get(idx) { return this._vals[idx] }\n"
          "  }\n"
          "  let $out0_inst = $out0_C([\"a\", \"b\"], [$in0, $in0 + 1])\n"
          "  local $out0 = 0\n"
          "  foreach (k, v in $out0_inst) { $out0 += v }", weight=0.5),

    # ---- Static memo with auto-memoize on mutable container (lines 1701-1707) ----
    # static(someArray) should hit the auto-memo rejection path
    Brick("static_memo_mutable_container",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function() {\n"
          "    let $out0_arr = static [1, 2, 3]\n"
          "    return $out0_arr.len()\n"
          "  }\n"
          "  let $out0 = $out0_fn() + $out0_fn()", weight=0.5),

    # ---- _OP_YIELD outside generator (line 1599) ----
    Brick("yield_outside_generator",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_fn = function() { yield $INT }\n"
          "    $out0_fn()\n"
          "    $out0 = 1\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- _OP_RESUME on non-generator (line 1608) ----
    Brick("resume_nongenerator_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { resume $in0; $out0 = 1 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- Call on non-callable (line 2470) ----
    Brick("call_noncallable_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_fns = {notfn = $in0}\n"
          "    $out0 = $out0_fns[\"notfn\"]()\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),

    # ===========================================================================
    # Round 6: _cmp returns non-integer, compare uncomparable types,
    #          ObjCmpI error, DerefInc fallback, misc
    # Target: sqvm.cpp lines 273-275, 282-284, 319-320, 687-691
    # ===========================================================================

    # ---- _cmp metamethod returns non-integer (lines 272-275) ----
    Brick("cmp_mm_returns_nonint",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _cmp(other) { return \"not_an_int\" }\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  try {\n"
          "    $out0 = $out0_C($in0) < $out0_C($in0 + 1) ? 1 : 0\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- _cmp metamethod fails (line 278) ----
    Brick("cmp_mm_fails",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _cmp(other) { throw \"cmp_fail\" }\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  try {\n"
          "    $out0 = $out0_C($in0) < $out0_C(0) ? 1 : 0\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.3),

    # ---- Compare uncomparable types (lines 282-284) ----
    # Two closures/arrays/etc that have no _cmp metamethod
    Brick("compare_closures_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_a = @() $in0\n"
          "    let $out0_b = @() $in0 + 1\n"
          "    $out0 = $out0_a < $out0_b ? 1 : 0\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),
    Brick("compare_arrays_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    $out0 = [$in0] < [$in0 + 1] ? 1 : 0\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- ObjCmpI compare error (line 319-320) ----
    # Comparing a non-numeric, non-delegable value with an integer literal
    Brick("cmpi_closure_vs_int_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_fn = @() $in0\n"
          "    $out0 = $out0_fn < 5 ? 1 : 0\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- Foreach on frozen table (covers _CHECK_FREEZE in FOREACH_OP) ----
    Brick("foreach_frozen_table",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = freeze({a = $in0, b = $in0 + 1})\n"
          "  local $out0 = 0\n"
          "  foreach (k, v in $out0_t) { $out0 += (type(v) == \"integer\") ? v : 0 }"),

    # ---- Foreach on frozen array ----
    Brick("foreach_frozen_array",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_a = freeze([$in0, $in0 + 1, $in0 + 2])\n"
          "  local $out0 = 0\n"
          "  foreach (_k, v in $out0_a) { $out0 += v }"),

    # ---- _OP_NEWOBJ: class with base class (covers class creation paths) ----
    Brick("class_with_base",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_Base = class {\n"
          "    bval = 0\n"
          "    constructor(v) { this.bval = v }\n"
          "    function getval() { return this.bval }\n"
          "  }\n"
          "  let $out0_Child = class($out0_Base) {\n"
          "    cval = 0\n"
          "    constructor(v) {\n"
          "      base.constructor(v)\n"
          "      this.cval = v * 2\n"
          "    }\n"
          "    function getval() { return base.getval() + this.cval }\n"
          "  }\n"
          "  let $out0 = $out0_Child($in0).getval()"),

    # ---- Typeof on various types (covers _OP_TYPEOF) ----
    Brick("typeof_mixed",
          [Slot("in0", INT)],
          [Slot("out0", STRING)],
          "let $out0_vals = [$in0, $in0.tofloat(), \"str\", true, null, [], {}]\n"
          "  local $out0 = \"\"\n"
          "  foreach (_k, v in $out0_vals) { $out0 += type(v) + \",\" }"),

    # ---- Switch statement (covers _OP_JMP chains) ----
    Brick("switch_int",
          [Slot("in0", INT)],
          [Slot("out0", STRING)],
          "local $out0 = \"other\"\n"
          "  switch ($in0 % 4) {\n"
          "    case 0: $out0 = \"zero\"; break\n"
          "    case 1: $out0 = \"one\"; break\n"
          "    case 2: $out0 = \"two\"; break\n"
          "    case 3: $out0 = \"three\"; break\n"
          "  }"),

    # ---- Nested try-catch with re-throw ----
    Brick("nested_try_rethrow",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    try { throw $in0 }\n"
          "    catch (inner) { $out0 = inner; throw inner + 1 }\n"
          "  } catch (outer) { $out0 = outer }"),

    # ---- Instance with constructor that throws ----
    Brick("constructor_throws",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    constructor(v) { if (v > 99999) throw \"too big\" }\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  try { $out0_C($in0); $out0 = 1 } catch (_e) { $out0 = -1 }"),

    # ---- Weakref (covers OT_WEAKREF type path) ----
    Brick("weakref_basic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {val = $in0}\n"
          "  let $out0_wr = $out0_t.weakref()\n"
          "  let $out0 = $out0_wr.val"),

    # ---- Static memo hit path (second call) (lines 1689-1692) ----
    Brick("static_memo_hit",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function() {\n"
          "    return static ($INT + $INT)\n"
          "  }\n"
          "  let $out0_a = $out0_fn()\n"
          "  let $out0 = $out0_fn()"),

    # ===========================================================================
    # Round 7: ObjCmpF error, string vs float literal, bool vs float literal,
    #          more edge cases
    # ===========================================================================

    # ---- ObjCmpF error: non-numeric vs float literal (lines 336-337) ----
    Brick("cmpf_string_vs_floatlit",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { $out0 = $in0 < 1.5 ? 1 : 0 } catch (_e) { $out0 = -1 }", weight=0.2),
    Brick("cmpf_bool_vs_floatlit",
          [Slot("in0", BOOL)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try { $out0 = $in0 < 2.5 ? 1 : 0 } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- ObjCmpI error: non-numeric vs int literal (also line 319 already covered) ----
    Brick("cmpi_array_vs_intlit",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_a = [$in0]\n"
          "    $out0 = $out0_a < 5 ? 1 : 0\n"
          "  } catch (_e) { $out0 = -1 }", weight=0.2),

    # ---- Recursive class method with _cmp (combined test) ----
    Brick("class_recursive_with_cmp",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _cmp(other) { return this.val <=> other.val }\n"
          "    function sum(n) { return n > 0 ? sum(n - 1) + this.val : 0 }\n"
          "  }\n"
          "  let $out0_a = $out0_C($in0)\n"
          "  let $out0_b = $out0_C($in0 + 1)\n"
          "  let $out0 = ($out0_a < $out0_b ? 1 : 0) + $out0_a.sum(3)"),

    # ---- Multiple return values from try-catch ----
    Brick("try_catch_multiple_paths",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  for (local $out0_i = 0; $out0_i < 3; $out0_i++) {\n"
          "    try {\n"
          "      if ($out0_i == 0) throw \"first\"\n"
          "      if ($out0_i == 1) $out0 += $in0\n"
          "      if ($out0_i == 2) throw $in0 + 1\n"
          "    } catch (e) {\n"
          "      $out0 += (type(e) == \"integer\") ? e : 0\n"
          "    }\n"
          "  }"),

    # ---- Deeply nested closures (more open outers) ----
    Brick("deeply_nested_closures",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_a = $in0\n"
          "  let $out0_f1 = @() $out0_a\n"
          "  local $out0_b = $in0 + 1\n"
          "  let $out0_f2 = @() $out0_b\n"
          "  local $out0_c = $in0 + 2\n"
          "  let $out0_f3 = @() $out0_c\n"
          "  $out0_a = $in0 * 2\n"
          "  $out0_b = $in0 * 3\n"
          "  $out0_c = $in0 * 4\n"
          "  let $out0 = $out0_f1() + $out0_f2() + $out0_f3()"),

    # --- Round 8: Native closure wrong args, stack resize, misc ---

    # Native closure wrong args - exact param count (CallNative lines 1899-1902)
    # type() requires exactly 2 params (this + obj), call with 1 (just this)
    Brick("native_wrong_args_exact",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fns = { t = type }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_fns[\"t\"]() } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Native closure wrong args - vararg minimum (CallNative lines 1903-1906)
    # assert() requires at least 2 params (this + condition), call with just this
    Brick("native_wrong_args_vararg",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fns = { a = assert }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_fns[\"a\"]() } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Foreach instance _nexti returns invalid key (FOREACH_OP line 754)
    Brick("foreach_instance_nexti_invalid",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  class $out0_C {\n"
          "    x = $in0\n"
          "    function _nexti(prev) {\n"
          "      if (prev == null) return \"x\"\n"
          "      if (prev == \"x\") return \"nonexistent_key\"\n"
          "      return null\n"
          "    }\n"
          "  }\n"
          "  try { foreach (k, v in $out0_C()) { $out0 += 1 } } catch($out0_e) { $out0 += 100 }"),

    # Foreach instance _nexti throws (FOREACH_OP lines 759-760)
    Brick("foreach_instance_nexti_throw",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  class $out0_C {\n"
          "    x = $in0\n"
          "    function _nexti(prev) {\n"
          "      throw \"nexti_error\"\n"
          "    }\n"
          "  }\n"
          "  try { foreach (k, v in $out0_C()) { $out0 += 1 } } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Class inherit from non-class at runtime (CLASS_OP line 846-847)
    Brick("class_inherit_nonclass",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_make_cls($out0_base) {\n"
          "    class $out0_C($out0_base) { v = 1 }\n"
          "    return $out0_C\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_make_cls($in0) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Default constructor with args (line 1096-1097)
    # Class with no constructor called with extra args via dynamic dispatch
    Brick("default_ctor_with_args",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_NoCtorCls { val = $in0 }\n"
          "  let $out0_cls = { c = $out0_NoCtorCls }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_cls[\"c\"](1, 2) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Stack resize via deep recursion with many locals (EnterFrame lines 2531-2538)
    Brick("stack_resize_deep_recursion",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_deep($out0_n) {\n"
          "    local $out0_a = $out0_n; local $out0_b = $out0_n+1; local $out0_c = $out0_n+2\n"
          "    local $out0_d = $out0_n+3; local $out0_e = $out0_n+4; local $out0_f = $out0_n+5\n"
          "    local $out0_g = $out0_n+6; local $out0_h = $out0_n+7; local $out0_i = $out0_n+8\n"
          "    local $out0_j = $out0_n+9; local $out0_k = $out0_n+10; local $out0_l = $out0_n+11\n"
          "    local $out0_m = $out0_n+12; local $out0_o = $out0_n+13; local $out0_p = $out0_n+14\n"
          "    if ($out0_n > 0) return $out0_deep($out0_n - 1)\n"
          "    return $out0_a + $out0_b + $out0_c + $out0_d + $out0_e + $out0_p\n"
          "  }\n"
          "  let $out0 = $out0_deep(150)"),

    # Stack resize with open outers to trigger RelocateOuters (lines 2575-2581)
    Brick("stack_resize_with_outers",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_captured = $in0\n"
          "  let $out0_get_cap = @() $out0_captured\n"
          "  function $out0_deep2($out0_n) {\n"
          "    local $out0_a = $out0_n; local $out0_b = $out0_n+1; local $out0_c = $out0_n+2\n"
          "    local $out0_d = $out0_n+3; local $out0_e = $out0_n+4; local $out0_f = $out0_n+5\n"
          "    local $out0_g = $out0_n+6; local $out0_h = $out0_n+7; local $out0_i = $out0_n+8\n"
          "    local $out0_j = $out0_n+9; local $out0_k = $out0_n+10; local $out0_l = $out0_n+11\n"
          "    if ($out0_n > 0) return $out0_deep2($out0_n - 1)\n"
          "    return $out0_a + $out0_b + $out0_get_cap()\n"
          "  }\n"
          "  $out0_captured = $in0 * 2\n"
          "  let $out0 = $out0_deep2(150)"),

    # GETK with TYPE_METHODS_ONLY flag (line 1153)
    # This happens with ?. optional chaining on type methods
    Brick("getk_type_methods_only",
          [Slot("in0", STRING)],
          [Slot("out0", INT)],
          "let $out0 = $in0?.len() ?? 0"),

    # GetImpl SLOT_RESOLVE_STATUS_ERROR via _get metamethod throw (line 1159)
    Brick("get_metamethod_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_ErrGet {\n"
          "    function _get(key) { throw \"get_error\" }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { let $out0_v = $out0_ErrGet()?[\"missing\"] } catch($out0_e) { $out0 = $in0 + 1 }"),

    # --- Round 9: Scattered coverable lines ---

    # SET_LITERAL on instance: set unknown field via function param (line 1214)
    Brick("set_literal_instance_no_member",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_set_field($out0_obj, $out0_val) { $out0_obj.nonexistent_field = $out0_val }\n"
          "  class $out0_C { x = 1 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_set_field($out0_C(), 42) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # SET_LITERAL table type hint match in loop (line 1227)
    Brick("set_literal_table_hint_loop",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = { x = $in0, y = 0 }\n"
          "  for (local $out0_i = 0; $out0_i < 10; $out0_i++) {\n"
          "    $out0_t.x = $out0_i\n"
          "    $out0_t.y = $out0_i * 2\n"
          "  }\n"
          "  let $out0 = $out0_t.x + $out0_t.y"),

    # SET_LITERAL on table: nonexistent key via function param (line 1241)
    Brick("set_literal_table_no_key",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_set_key($out0_t, $out0_v) { $out0_t.missing_key = $out0_v }\n"
          "  let $out0_tbl = { x = $in0 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_set_key($out0_tbl, 42) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # NewSlot duplicate property on unlocked class (lines 2370-2374)
    Brick("newslot_class_dup_unlocked",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_C { x = $in0 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_C[\"x\"] <- 42 } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Call() with non-callable via _call metamethod (lines 2469-2471)
    Brick("call_noncallable_metamethod",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_BadCall {\n"
          "    _call = 42\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_BadCall()() } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Thread type method to trigger FindTypeClass OT_THREAD (line 2155)
    Brick("thread_type_method",
          [Slot("in0", INT)],
          [Slot("out0", STRING)],
          "let $out0_th = newthread(@() $in0)\n"
          "  let $out0 = $out0_th.getstatus()"),

    # Call class with no constructor via _call metamethod (line 2466)
    Brick("call_class_no_ctor_metamethod",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_NoCtor { val = $in0 }\n"
          "  class $out0_Wrap {\n"
          "    _call = $out0_NoCtor\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try {\n"
          "    let $out0_inst = $out0_Wrap()()\n"
          "    $out0 = $out0_inst.val\n"
          "  } catch($out0_e) { $out0 = $in0 + 1 }"),

    # NewSlot default case: indexing non-indexable via function param (line 2378)
    Brick("newslot_nonindexable_dynamic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_add_slot($out0_obj) { $out0_obj[\"k\"] <- 1 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_add_slot($in0) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # --- Round 10: DerefInc on table, typed params, getbase ---

    # DerefInc on table field (lines 687-696)
    Brick("derefinc_table_field",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = { x = $in0, y = 0 }\n"
          "  $out0_t.x += 10\n"
          "  $out0_t.y -= 5\n"
          "  let $out0 = $out0_t.x + $out0_t.y"),

    # DerefInc postfix on table field
    Brick("derefinc_table_postinc",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = { x = $in0 }\n"
          "  let $out0 = $out0_t.x++"),

    # Typed varargs with wrong type via dynamic dispatch (StartCall line 538)
    Brick("varargs_type_check_fail",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_typed_va($out0_a: int, ...: int) { return $out0_a }\n"
          "  let $out0_fns = { f = $out0_typed_va }\n"
          "  local $out0 = $in0\n"
          "  try { $out0 = $out0_fns[\"f\"]($in0, \"wrong_type\") } catch($out0_e) { $out0 = $in0 + 1 }"),

    # _OP_CHECK_TYPE failure via typed param with wrong type (line 1739)
    Brick("check_type_fail",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_typed($out0_a: int) { return $out0_a }\n"
          "  let $out0_fns = { f = $out0_typed }\n"
          "  local $out0 = $in0\n"
          "  try { $out0 = $out0_fns[\"f\"](\"not_an_int\") } catch($out0_e) { $out0 = $in0 + 1 }"),

    # getbase in class without parent (lines 1665-1667)
    Brick("getbase_no_parent",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_NoBase {\n"
          "    val = $in0\n"
          "    function get_base() { return base }\n"
          "  }\n"
          "  let $out0_b = $out0_NoBase().get_base()\n"
          "  let $out0 = $out0_b == null ? $in0 + 1 : 0"),

    # DerefInc on table with missing key (lines 687-690)
    Brick("derefinc_table_missing_key",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_inc_missing($out0_obj) { $out0_obj.no_such_field += 1 }\n"
          "  let $out0_t = { x = $in0 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_inc_missing($out0_t) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # DerefInc on instance with missing key (lines 687-690)
    Brick("derefinc_instance_missing_key",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_inc_inst($out0_obj) { $out0_obj.no_such_member += 1 }\n"
          "  class $out0_C { x = $in0 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_inc_inst($out0_C()) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # DerefInc via _get/_set metamethods (lines 695-696)
    Brick("derefinc_via_get_set_mm",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_Proxy {\n"
          "    _data = null\n"
          "    constructor() { this._data = { x = $in0 } }\n"
          "    function _get(key) { return this._data[key] }\n"
          "    function _set(key, val) { this._data[key] = val }\n"
          "  }\n"
          "  function $out0_inc_proxy($out0_obj) { $out0_obj.x += 10 }\n"
          "  let $out0_p = $out0_Proxy()\n"
          "  $out0_inc_proxy($out0_p)\n"
          "  let $out0 = $out0_p.x"),

    # =========================================================================
    # Round 11: stdlib-based coverage -- debug hooks, error handler, coroutines
    # =========================================================================

    # Debug hook via metamethod -- covers CallDebugHook (1845-1884),
    # StartCall<true> call hook (591-595), Return<true> return hook (617-623),
    # Execute<true> line hook (986-995)
    Brick("debug_hook_via_metamethod",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  local $out0_hk = 0\n"
          "  $out0_dbg.setdebughook(function(_evt, _src, _line, _fn) { $out0_hk++ })\n"
          "  class $out0_Add {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _add(other) { return this.getclass()(this.val + other) }\n"
          "  }\n"
          "  let $out0_r = $out0_Add($in0) + 1\n"
          "  $out0_dbg.setdebughook(null)\n"
          "  let $out0 = $out0_hk"),

    # Debug hook exception unwind -- covers exception unwind hooks (1766-1772)
    # When exception propagates through Execute<true>, 'r' hooks fire for each frame
    Brick("debug_hook_exception_unwind",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  local $out0_hk = 0\n"
          "  $out0_dbg.setdebughook(function(_evt, _src, _line, _fn) { $out0_hk++ })\n"
          "  class $out0_Bad {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _cmp(other) { throw \"cmp_err\" }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { let $out0_r = $out0_Bad($in0) < $out0_Bad(0) } catch($out0_e) { $out0 = $in0 + $out0_hk }\n"
          "  $out0_dbg.setdebughook(null)"),

    # Debug hook left active for VM finalize (line 172) -- covers CallDebugHook('x')
    # and the ci==NULL path in CallDebugHook (1847-1862)
    Brick("debug_hook_finalize",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  $out0_dbg.setdebughook(function(_evt, _src, _line, _fn) {})\n"
          "  $out0_dbg.setdebughook(null)\n"
          "  let $out0 = $in0"),

    # Error handler via thread -- covers CallErrorHandler (1800-1841)
    # Thread inherits error handler from parent VM at creation time
    Brick("error_handler_thread",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  local $out0_err = null\n"
          "  $out0_dbg.seterrorhandler(function(err) { $out0_err = err })\n"
          "  let $out0_fn = function() { throw \"err\" }\n"
          "  let $out0_th = newthread($out0_fn)\n"
          "  try { $out0_th.call() } catch($out0_e) {}\n"
          "  let $out0 = $out0_err != null ? $in0 + 1 : $in0"),

    # Coroutine suspend/resume -- covers Suspend() (711-716),
    # SQ_SUSPEND_FLAG in CallNative (1945-1947),
    # suspend handling in Execute (1043-1049),
    # ET_RESUME_VM (968-972)
    Brick("coroutine_suspend_resume",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function() { suspend($in0); return $in0 + 1 }\n"
          "  let $out0_th = newthread($out0_fn)\n"
          "  $out0_th.call()\n"
          "  let $out0 = $out0_th.wakeup()"),

    # Coroutine wakeup with throw -- covers ET_RESUME_THROW_VM (973-974)
    Brick("coroutine_wakeup_throw",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_fn = function() {\n"
          "    try { suspend($in0) } catch($out0_e) { return $out0_e }\n"
          "    return $in0\n"
          "  }\n"
          "  let $out0_th = newthread($out0_fn)\n"
          "  $out0_th.call()\n"
          "  let $out0 = $out0_th.wakeupthrow($in0 + 1)"),

    # Generator yield/resume -- covers ET_RESUME_GENERATOR (961-967),
    # _OP_YIELD success path (1596-1604)
    Brick("generator_yield_resume",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_gen() { yield $in0; yield $in0 + 1; return $in0 + 2 }\n"
          "  let $out0_g = $out0_gen()\n"
          "  resume $out0_g\n"
          "  resume $out0_g\n"
          "  let $out0 = resume $out0_g"),

    # Error handler with debug hook active -- covers both hooks interacting
    # CallErrorHandler with _debughook set but _debughook_native unset
    Brick("error_handler_with_debug_hook",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  local $out0_hk = 0\n"
          "  $out0_dbg.setdebughook(function(_evt, _src, _line, _fn) { $out0_hk++ })\n"
          "  local $out0_err = null\n"
          "  $out0_dbg.seterrorhandler(function(err) { $out0_err = err })\n"
          "  let $out0_fn = function() { throw \"err\" }\n"
          "  let $out0_th = newthread($out0_fn)\n"
          "  try { $out0_th.call() } catch($out0_e) {}\n"
          "  $out0_dbg.setdebughook(null)\n"
          "  let $out0 = $out0_err != null ? $in0 + $out0_hk : $in0"),

    # =========================================================================
    # Round 12: VM edge-case coverage -- foreach instance, float inc,
    # default param types, constructors, metamethod errors, generators
    # =========================================================================

    # foreach on plain instance -- covers delegate iteration path (764-772)
    # Instance without _nexti metamethod iterates class members via delegate
    Brick("foreach_instance_delegate",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_FI {\n"
          "    a = 10\n"
          "    b = 20\n"
          "    c = 30\n"
          "  }\n"
          "  local $out0 = 0\n"
          "  foreach ($out0_k, $out0_v in $out0_FI()) { $out0 += $out0_v }\n"
          "  $out0 = $out0 + $in0"),

    # Class with no constructor called with args -- covers lines 1095-1098
    Brick("no_ctor_with_args",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  try {\n"
          "    class $out0_Empty {}\n"
          "    $out0_Empty(1, 2)\n"
          "  } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Float local pre-increment -- covers _OP_INCL non-integer path (1523-1526)
    # Must use ++x (pre-increment = _OP_INCL), not x++ (post-increment = _OP_PINCL)
    Brick("float_local_increment",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0_x = ($in0).tofloat() + 0.5\n"
          "  ++$out0_x\n"
          "  ++$out0_x\n"
          "  let $out0 = $out0_x.tointeger()"),

    # _tostring metamethod that throws -- covers lines 440-442
    Brick("tostring_mm_throws",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_Bad {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _tostring() { throw \"tostring_fail\" }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { let $out0_s = \"\" + $out0_Bad($in0) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # suspend() inside metamethod -- covers line 715
    # "cannot suspend through native calls/metamethods"
    Brick("suspend_in_metamethod",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_SuspGet {\n"
          "    function _get(key) { suspend(42); return 0 }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  let $out0_fn = function() { return $out0_SuspGet().field }\n"
          "  let $out0_th = newthread($out0_fn)\n"
          "  try { $out0_th.call() } catch($out0_e) { $out0 = $in0 + 1 }"),

    # foreach on generator -- covers generator resume path (784)
    Brick("foreach_generator",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "function $out0_gen() { yield $in0; yield $in0 + 1; yield $in0 + 2 }\n"
          "  local $out0 = 0\n"
          "  foreach ($out0_v in $out0_gen()) { $out0 += $out0_v }"),

    # _nexti metamethod that throws -- covers line 759
    Brick("nexti_mm_throws",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_BadIter {\n"
          "    function _nexti(prev) { throw \"nexti_fail\" }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { foreach ($out0_k, $out0_v in $out0_BadIter()) {} } catch($out0_e) { $out0 = $in0 + 1 }"),

    # bindenv on native closure -- covers lines 1932-1933
    Brick("bindenv_native_closure",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_f = type.bindenv({})\n"
          "  let $out0 = $out0_f($in0) == \"integer\" ? $in0 + 1 : $in0"),

    # Instance newslot error -- covers line 2363
    # "class instances do not support the new slot operator"
    Brick("instance_newslot_error",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_C { x = 1 }\n"
          "  local $out0 = $in0\n"
          "  try { $out0_C()[\"y\"] <- 2 } catch($out0_e) { $out0 = $in0 + 1 }"),

    # clone with _cloned that throws -- covers line 2303
    Brick("clone_cloned_throws",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_CC {\n"
          "    val = 0\n"
          "    constructor(v) { this.val = v }\n"
          "    function _cloned(orig) { throw \"clone_fail\" }\n"
          "  }\n"
          "  local $out0 = $in0\n"
          "  try { clone $out0_CC($in0) } catch($out0_e) { $out0 = $in0 + 1 }"),

    # delete table slot -- covers line 2421
    Brick("delete_table_slot",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = { a = $in0, b = $in0 + 1 }\n"
          "  delete $out0_t[\"a\"]\n"
          "  let $out0 = $out0_t.len()"),

    # Call non-callable value -- covers line 1135
    # "attempt to call 'bool'"
    Brick("call_non_callable",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = $in0\n"
          "  try {\n"
          "    local $out0_x = $in0 > 0\n"
          "    $out0_x()\n"
          "  } catch($out0_e) { $out0 = $in0 + 1 }"),

    # Instance with _call metamethod -- covers instance/table _call path (1110)
    Brick("callable_instance",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_Fn {\n"
          "    _v = 0\n"
          "    constructor(v) { this._v = v }\n"
          "    function _call(env, x) { return this._v + x }\n"
          "  }\n"
          "  let $out0 = $out0_Fn($in0)(100)"),

    # =========================================================================
    # Round 13: StartCall<true> coverage -- debug hook + varargs, default
    # params, generators, bindenv, wrong-args via newthread() to enter
    # Execute<true> which dispatches through StartCall<true>
    # =========================================================================

    # Debug hook + varargs function via thread -- covers StartCall<true>
    # varargs path (lines 519-549)
    Brick("debughook_varargs",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  function $out0_va(a, ...) { return a + vargv.len() }\n"
          "  $out0_dbg.setdebughook(@(ev, src, ln, fn) null)\n"
          "  let $out0 = newthread(@() $out0_va($in0, 10, 20)).call()\n"
          "  $out0_dbg.setdebughook(null)"),

    # Debug hook + default params via thread -- covers StartCall<true>
    # default params path (lines 551-557)
    Brick("debughook_default_params",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  function $out0_df(a, b = 100) { return a + b }\n"
          "  $out0_dbg.setdebughook(@(ev, src, ln, fn) null)\n"
          "  let $out0 = newthread(@() $out0_df($in0)).call()\n"
          "  $out0_dbg.setdebughook(null)"),

    # Debug hook + generator creation via thread -- covers StartCall<true>
    # generator path (lines 598-605)
    Brick("debughook_generator",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  function $out0_gen() { yield 1; yield 2; return 3 }\n"
          "  $out0_dbg.setdebughook(@(ev, src, ln, fn) null)\n"
          "  let $out0_th = newthread(function() {\n"
          "    local $out0_sum = 0\n"
          "    foreach ($out0_v in $out0_gen()) { $out0_sum += $out0_v }\n"
          "    return $out0_sum\n"
          "  })\n"
          "  let $out0 = $out0_th.call() + $in0\n"
          "  $out0_dbg.setdebughook(null)"),

    # Debug hook + bindenv via thread -- covers StartCall<true>
    # env override path (lines 581-582)
    Brick("debughook_bindenv",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  let $out0_env = { val = $in0 + 1 }\n"
          "  let $out0_bf = (function() { return this.val }).bindenv($out0_env)\n"
          "  $out0_dbg.setdebughook(@(ev, src, ln, fn) null)\n"
          "  let $out0 = newthread(@() $out0_bf()).call()\n"
          "  $out0_dbg.setdebughook(null)"),

    # Debug hook + wrong arg count via thread -- covers StartCall<true>
    # wrong args error path (lines 558-565)
    Brick("debughook_wrong_args",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  function $out0_sf(a, b, c) { return a + b + c }\n"
          "  let $out0_fns = { f = $out0_sf }\n"
          "  $out0_dbg.setdebughook(@(ev, src, ln, fn) null)\n"
          "  let $out0_th = newthread(function() {\n"
          "    try { $out0_fns[\"f\"](1, 2) } catch($out0_e) {}\n"
          "    return $in0 + 1\n"
          "  })\n"
          "  let $out0 = $out0_th.call()\n"
          "  $out0_dbg.setdebughook(null)"),

    # Debug hook + varargs wrong arg count via thread -- covers StartCall<true>
    # varargs too-few-args error path (lines 521-528)
    Brick("debughook_varargs_wrong_args",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_dbg = require(\"debug\")\n"
          "  function $out0_va(a, b, ...) { return a + b }\n"
          "  let $out0_fns = { f = $out0_va }\n"
          "  $out0_dbg.setdebughook(@(ev, src, ln, fn) null)\n"
          "  let $out0_th = newthread(function() {\n"
          "    try { $out0_fns[\"f\"]() } catch($out0_e) {}\n"
          "    return $in0 + 1\n"
          "  })\n"
          "  let $out0 = $out0_th.call()\n"
          "  $out0_dbg.setdebughook(null)"),

    # =========================================================================
    # Round 14: _OP_PATCH_DOCOBJ and instanceof built-in types
    # =========================================================================

    # Doc comment inside table body -- covers _OP_PATCH_DOCOBJ (lines 1673-1687)
    # The @@"..." must be INSIDE the { } to set the table's docObject
    Brick("docobj_table",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_t = {\n"
          "    @@\"fuzz doc\"\n"
          "    val = $in0 + 1\n"
          "  }\n"
          "  let $out0 = $out0_t.val"),

    # Doc comment inside class body -- covers _OP_PATCH_DOCOBJ for classes
    Brick("docobj_class",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_C {\n"
          "    @@\"fuzz class doc\"\n"
          "    val = 0\n"
          "    constructor(_v) { this.val = _v }\n"
          "  }\n"
          "  let $out0 = $out0_C($in0 + 1).val"),

    # instanceof with built-in Integer type -- covers IsInstanceOf
    # built-in type path (lines 887-888, 894-895)
    Brick("instanceof_builtin_int",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_types = require(\"types\")\n"
          "  let $out0 = ($in0 instanceof $out0_types.Integer) ? $in0 + 1 : 0"),

    # instanceof with built-in Function type for closures and native closures
    # -- covers special closure handling (lines 891-892)
    Brick("instanceof_builtin_closure",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_types = require(\"types\")\n"
          "  let $out0_fn = @() $in0\n"
          "  let $out0_ok1 = print instanceof $out0_types.Function\n"
          "  let $out0_ok2 = $out0_fn instanceof $out0_types.Function\n"
          "  let $out0 = ($out0_ok1 && $out0_ok2) ? $in0 + 1 : 0"),

    # instanceof non-instance against regular class -- covers else return false
    # path (lines 896-897)
    Brick("instanceof_noninstance_class",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_C {}\n"
          "  let $out0 = ($in0 instanceof $out0_C) ? 0 : $in0 + 1"),

    # _set metamethod that throws via dynamic key -- covers Set()
    # SLOT_RESOLVE_STATUS_ERROR path (line 2244) via Set()->FallBackSet()
    # Dynamic key forces _OP_SET (not _OP_SET_LITERAL)
    Brick("set_metamethod_throw_dynamic",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "class $out0_C {\n"
          "    function _set(k, v) { throw \"set err\" }\n"
          "  }\n"
          "  let $out0_inst = $out0_C()\n"
          "  let $out0_key = \"x\"\n"
          "  local $out0 = $in0\n"
          "  try { $out0_inst[$out0_key] = 42 } catch($out0_e) { $out0 = $in0 + 1 }"),

    # ---- class + yield / generator interactions ----

    # Class method that yields, called from outside constructor
    Brick("class_method_yield",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _v = 0\n"
          "    constructor(xv) { this._v = xv }\n"
          "    function gen_vals() { yield this._v; yield this._v + $INT }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0_g = $out0_obj.gen_vals()\n"
          "  local $out0 = 0\n"
          "  while ($out0_g.getstatus() == \"suspended\") { $out0 += resume $out0_g }"),

    # Class method yield via .call() -- triggers generator+call interaction
    Brick("class_method_yield_call",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_C = class {\n"
          "      _v = 0\n"
          "      constructor(xv) { this._v = xv }\n"
          "      function do_yield() { yield $INT; yield $INT }\n"
          "      function call_yield() { this.do_yield.call(0) }\n"
          "    }\n"
          "    let $out0_obj = $out0_C($in0)\n"
          "    $out0_obj.call_yield()\n"
          "    $out0 = $in0 + $INT\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # Constructor calls method that yields via .call() -- reproduces class_yield.nut bug
    Brick("class_ctor_yield_call",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_C = class {\n"
          "      constructor() {\n"
          "        this.call_yield()\n"
          "      }\n"
          "      function call_yield() { this.do_yield.call(0) }\n"
          "      function do_yield() { yield }\n"
          "    }\n"
          "    $out0_C()\n"
          "    $out0 = $in0 + $INT\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # Constructor directly calls .call() on a yielding method
    Brick("class_ctor_direct_yield_call",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_C = class {\n"
          "      constructor() {\n"
          "        this.do_yield.call(0)\n"
          "      }\n"
          "      function do_yield() { yield }\n"
          "    }\n"
          "    $out0_C()\n"
          "    $out0 = $in0 + $INT\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # Yield inside class method called from constructor with value passing
    Brick("class_ctor_yield_value",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_C = class {\n"
          "      _r = 0\n"
          "      constructor(xv) {\n"
          "        this._r = xv\n"
          "        this.gen_val.call(this)\n"
          "      }\n"
          "      function gen_val() { yield this._r + $INT }\n"
          "    }\n"
          "    let $out0_obj = $out0_C($in0)\n"
          "    $out0 = $out0_obj._r\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # Derived class constructor calls inherited yielding method via .call()
    Brick("class_inherit_ctor_yield_call",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_Base = class {\n"
          "      function do_yield() { yield $INT }\n"
          "    }\n"
          "    let $out0_Der = class($out0_Base) {\n"
          "      constructor() { this.do_yield.call(0) }\n"
          "    }\n"
          "    $out0_Der()\n"
          "    $out0 = $in0 + $INT\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # Generator created from class method, resumed outside
    Brick("class_method_generator_resume",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "let $out0_C = class {\n"
          "    _v = 0\n"
          "    constructor(xv) { this._v = xv }\n"
          "    function make_gen() { yield this._v; yield this._v * $INT; yield this._v + $INT }\n"
          "  }\n"
          "  let $out0_obj = $out0_C($in0)\n"
          "  let $out0_g = $out0_obj.make_gen()\n"
          "  local $out0 = resume $out0_g\n"
          "  $out0 += resume $out0_g"),

    # Yield from closure inside class method
    Brick("class_closure_yield",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_C = class {\n"
          "      _v = 0\n"
          "      constructor(xv) { this._v = xv }\n"
          "      function get_gen() {\n"
          "        let xval = this._v\n"
          "        return function() { yield xval; yield xval + $INT }\n"
          "      }\n"
          "    }\n"
          "    let $out0_obj = $out0_C($in0)\n"
          "    let $out0_gf = $out0_obj.get_gen()\n"
          "    let $out0_g = $out0_gf()\n"
          "    $out0 = resume $out0_g\n"
          "    $out0 += resume $out0_g\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # newthread with class method that yields
    Brick("class_method_newthread_yield",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  try {\n"
          "    let $out0_C = class {\n"
          "      _v = 0\n"
          "      constructor(xv) { this._v = xv }\n"
          "      function do_yield() { yield this._v + $INT }\n"
          "    }\n"
          "    let $out0_obj = $out0_C($in0)\n"
          "    let $out0_th = newthread($out0_obj.do_yield)\n"
          "    $out0 = $out0_th.call($out0_obj)\n"
          "  } catch($out0_e) { $out0 = $in0 }"),

    # ---- foreach destructuring (parser desugars to surrogate val + body destr) ----
    Brick("foreach_destruct_table",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ({a, b} in [{a = $in0, b = $in1}, {a = $in1, b = $in0}])\n"
          "    $out0 += a + b"),

    Brick("foreach_destruct_array",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ([a, b] in [[$in0, $in1], [$in1, $in0]])\n"
          "    $out0 += a - b"),

    Brick("foreach_destruct_idx_table",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach (i, {x} in [{x = $in0}, {x = $in1}])\n"
          "    $out0 += i * 10 + x"),

    Brick("foreach_destruct_default",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ({m = $INT} in [{m = $in0}, {}, {m = $in0 + 1}])\n"
          "    $out0 += m"),

    Brick("foreach_destruct_typed_default",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ({k: int = $INT} in [{}, {k = $in0}, {}])\n"
          "    $out0 += k"),

    Brick("foreach_destruct_array_default",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ([a, b = $INT] in [[$in0], [$in0, $in0 + 1]])\n"
          "    $out0 += a + b"),

    Brick("foreach_destruct_empty_table",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ({} in [{a = $in0}, {b = $in0}, {}])\n"
          "    $out0++"),

    Brick("foreach_destruct_empty_array",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  foreach ([] in [[$in0, $in0], [$in0]])\n"
          "    $out0++"),

    Brick("foreach_destruct_nested",
          [Slot("in0", INT), Slot("in1", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_rows = [{xs = [{v = $in0}, {v = $in1}]}, {xs = [{v = $in1}]}]\n"
          "  foreach ({xs} in $out0_rows)\n"
          "    foreach ({v} in xs) $out0 += v"),

    # ---- per-iter capture: closures store v / i and call after loop ----
    Brick("foreach_capture_val",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_fns = []\n"
          "  foreach (v in [$in0, $in0 + 1, $in0 + 2])\n"
          "    $out0_fns.append(@() v)\n"
          "  foreach (f in $out0_fns) $out0 += f()"),

    Brick("foreach_capture_idx_val",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_fns = []\n"
          "  foreach (i, v in [$in0, $in0 * 2, $in0 * 3])\n"
          "    $out0_fns.append(@() i * 100 + v)\n"
          "  foreach (f in $out0_fns) $out0 += f()"),

    Brick("foreach_capture_destruct_field",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_fns = []\n"
          "  foreach ({v} in [{v = $in0}, {v = $in0 + 1}, {v = $in0 + 2}])\n"
          "    $out0_fns.append(@() v)\n"
          "  foreach (f in $out0_fns) $out0 += f()"),

    Brick("foreach_capture_idx_destruct",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_fns = []\n"
          "  foreach (i, {x} in [{x = $in0}, {x = $in0 + 1}])\n"
          "    $out0_fns.append(@() i + x)\n"
          "  foreach (f in $out0_fns) $out0 += f()"),

    Brick("foreach_capture_no_inner_capture",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_fns = []\n"
          "  foreach (v in [$in0, $in0 + 1])\n"
          "    $out0_fns.append(function() { let local_v = $INT; return local_v })\n"
          "  foreach (f in $out0_fns) $out0 += f()"),

    Brick("foreach_destruct_default_with_closure",
          [Slot("in0", INT)],
          [Slot("out0", INT)],
          "local $out0 = 0\n"
          "  let $out0_factor = $in0\n"
          "  foreach ({n = (function(){ return $out0_factor + $INT })()}\n"
          "           in [{n = $in0}, {}, {n = $in0 + 1}])\n"
          "    $out0 += n"),
]


def _compute_pre_sig(pre: list[Slot]) -> tuple[dict, int]:
    """Precompute the type-count signature for a brick's preconditions.

    Returns (needed_dict, any_needed) where needed_dict maps type -> count
    and any_needed is the number of ANY-typed slots.
    """
    needed: dict = {}
    any_needed = 0
    for slot in pre:
        if slot.type == ANY:
            any_needed += 1
        else:
            needed[slot.type] = needed.get(slot.type, 0) + 1
    return needed, any_needed


# Precomputed signatures: brick index -> (needed_dict, any_needed, total_non_any)
_BRICK_PRE_SIGS: list[tuple[dict, int, int]] = []
for _b in BRICKS:
    _needed, _any_needed = _compute_pre_sig(_b.pre)
    _BRICK_PRE_SIGS.append((_needed, _any_needed, sum(_needed.values())))
del _b, _needed, _any_needed


# ---------------------------------------------------------------------------
# Variable pool
# ---------------------------------------------------------------------------

@dataclass
class PoolVar:
    name: str
    type: str


class VarPool:
    def __init__(self, prefix: str = "v"):
        self.vars: list[PoolVar] = []
        self._counter = 0
        self._prefix = prefix
        self._type_counts: dict[str, int] = {}

    def fresh(self) -> str:
        name = f"{self._prefix}{self._counter}"
        self._counter += 1
        return name

    def add(self, typ: str) -> str:
        name = self.fresh()
        self.vars.append(PoolVar(name, typ))
        self._type_counts[typ] = self._type_counts.get(typ, 0) + 1
        return name

    def add_existing(self, name: str, typ: str) -> None:
        """Add a variable with a known name (e.g. captured from outer scope)."""
        self.vars.append(PoolVar(name, typ))
        self._type_counts[typ] = self._type_counts.get(typ, 0) + 1

    def find(self, slot: Slot, exclude: set[str]) -> list[PoolVar]:
        return [pv for pv in self.vars
                if pv.name not in exclude and slot.accepts(pv.type)]

    def can_satisfy(self, pre: list[Slot], _pre_sig=None) -> bool:
        if not pre:
            return True
        if _pre_sig is not None:
            needed, any_needed, total_non_any = _pre_sig
        else:
            needed = {}
            any_needed = 0
            for slot in pre:
                if slot.type == ANY:
                    any_needed += 1
                else:
                    needed[slot.type] = needed.get(slot.type, 0) + 1
            total_non_any = sum(needed.values())
        for typ, count in needed.items():
            if isinstance(typ, tuple):
                avail = sum(self._type_counts.get(t, 0) for t in typ)
            else:
                avail = self._type_counts.get(typ, 0)
            if avail < count:
                return False
        if any_needed > 0 and len(self.vars) < any_needed + total_non_any:
            return False
        return True


# ---------------------------------------------------------------------------
# Literal generators
# ---------------------------------------------------------------------------

_BOUNDARY_N = [1, 7, 8, 15, 16, 31, 32, 63, 64]
_INT64_MIN = -(2**63)
_INT64_MAX = 2**63 - 1
EDGE_INTS = sorted(set(
    v for v in
    [0, 42, 1000]
    + [2**n - 1 for n in _BOUNDARY_N]
    + [2**n     for n in _BOUNDARY_N]
    + [2**n + 1 for n in _BOUNDARY_N]
    + [-(2**n) - 2 for n in _BOUNDARY_N]
    + [-(2**n) - 1 for n in _BOUNDARY_N]
    + [-(2**n)     for n in _BOUNDARY_N]
    + [-(2**n) + 1 for n in _BOUNDARY_N]
    if _INT64_MIN < v <= _INT64_MAX
))
EDGE_FLOATS = [
    0.0, -0.0, 1.5, -2.5, 3.14, 0.001, 100.0, -0.5,
    # denormalized
    1e-40, -1e-40,
    # small normal
    1e-37, -1e-37,
    # big
    1e15, -1e15,
    1e37, -1e37,
]
EDGE_STRINGS = [
    # plain
    '""',
    '"hello"',
    '"world"',
    '"foo"',
    '"bar"',
    '"test"',
    '"quirrel"',
    '"x"',
    '"abc"',
    # whitespace & escapes
    '" "',
    '"\\t"',
    '"\\n"',
    '"\\r\\n"',
    '"\\t\\n\\r"',
    # special characters
    '"!@#%^&*()"',
    '"[]{}()"',
    '","',
    '";"',
    '":"',
    '"."',
    # backslash
    '"\\\\"',
    '"\\\\\\\\"',
    # quotes inside
    '"\\""',
    '"he said \\"hi\\""',
    # unicode escapes
    '"\\u0000"',
    '"\\u0001"',
    '"\\u00ff"',
    '"\\u0100"',
    '"\\uffff"',
    '"\\u041f\\u0440\\u0438\\u0432\\u0435\\u0442"',
    # long & repetitive
    '"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"',
    '"abcdefghijklmnopqrstuvwxyz0123456789"',
    # digits as strings
    '"0"',
    '"123"',
    '"-1"',
    '"3.14"',
    '"1e10"',
    # boundary-like
    '"null"',
    '"true"',
    '"false"',
    '"0.0"',
    # verbatim (raw) strings
    '@"raw_string"',
    '@"line1\nstill_line1"',
    '@"back\\slash"',
]


EDGE_HEX_INTS = [
    "0x0", "0x1", "0xFF", "0x100", "0x7FFF", "0x8000", "0xFFFF",
    "0x7FFFFFFF", "0x80000000", "0xFFFFFFFF", "0xDEADBEEF", "0xCAFE",
]
EDGE_CHAR_LITERALS = [
    "'A'", "'Z'", "'0'", "'9'", "' '", "'\\n'", "'\\t'", "'\\\\'", "'\\''",
]

SMALL_SHIFT_AMOUNTS = [1, 2, 3, 4, 5, 8, 16, 31]

def rand_int(rng: random.Random) -> str:
    # 10% chance hex, 5% chance char literal
    r = rng.random()
    if r < 0.10:
        return rng.choice(EDGE_HEX_INTS)
    if r < 0.15:
        return rng.choice(EDGE_CHAR_LITERALS)
    return str(rng.choice(EDGE_INTS))

def rand_float(rng: random.Random) -> str:
    return str(rng.choice(EDGE_FLOATS))

def rand_string(rng: random.Random) -> str:
    return rng.choice(EDGE_STRINGS)

def rand_bool(rng: random.Random) -> str:
    return rng.choice(["true", "false"])


# ---------------------------------------------------------------------------
# Complex expression generators
# ---------------------------------------------------------------------------

_no_exclude: set[str] = frozenset()


def _int_atom(rng: random.Random, pool, exclude_vars=_no_exclude) -> str:
    """Atomic int expression: pool variable or literal."""
    vs = [pv for pv in pool.vars if pv.type == INT and pv.name not in exclude_vars]
    if vs and rng.random() < 0.6:
        return rng.choice(vs).name
    v = rand_int(rng)
    return f"({v})" if v.startswith('-') else v


def _float_atom(rng: random.Random, pool, exclude_vars=_no_exclude) -> str:
    vs = [pv for pv in pool.vars if pv.type == FLOAT and pv.name not in exclude_vars]
    if vs and rng.random() < 0.6:
        return rng.choice(vs).name
    v = rand_float(rng)
    return f"({v})" if v.startswith('-') else v


def _bool_atom(rng: random.Random, pool, exclude_vars=_no_exclude) -> str:
    vs = [pv for pv in pool.vars if pv.type == BOOL and pv.name not in exclude_vars]
    if vs and rng.random() < 0.6:
        return rng.choice(vs).name
    return rand_bool(rng)


def _string_atom(rng: random.Random, pool, exclude_vars=_no_exclude) -> str:
    vs = [pv for pv in pool.vars if pv.type == STRING and pv.name not in exclude_vars]
    if vs and rng.random() < 0.6:
        return rng.choice(vs).name
    return rand_string(rng)


def gen_int_expr(rng: random.Random, pool, depth: int = 0, max_depth: int = 3,
                 exclude_vars: set[str] = _no_exclude) -> str:
    """Generate a complex int rvalue expression."""
    if depth >= max_depth or (depth > 0 and rng.random() < 0.3):
        return _int_atom(rng, pool, exclude_vars)
    d = depth + 1
    r = rng.random()
    if r < 0.35:  # binary arithmetic
        op = rng.choice(['+', '-', '*', '&', '|', '^'])
        return f"({gen_int_expr(rng, pool, d, max_depth, exclude_vars)} {op} {gen_int_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.45:  # unary
        op = rng.choice(['-', '~'])
        return f"{op}({gen_int_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.60:  # ternary
        return f"({gen_bool_expr(rng, pool, d, max_depth, exclude_vars)} ? {gen_int_expr(rng, pool, d, max_depth, exclude_vars)} : {gen_int_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.80:  # shift
        op = rng.choice(['<<', '>>', '>>>'])
        return f"({gen_int_expr(rng, pool, d, max_depth, exclude_vars)} {op} {rng.choice(SMALL_SHIFT_AMOUNTS)})"
    # float-to-int cast
    return f"({gen_float_expr(rng, pool, d, max_depth, exclude_vars)}).tointeger()"


def gen_float_expr(rng: random.Random, pool, depth: int = 0, max_depth: int = 3,
                   exclude_vars: set[str] = _no_exclude) -> str:
    """Generate a complex float rvalue expression."""
    if depth >= max_depth or (depth > 0 and rng.random() < 0.3):
        return _float_atom(rng, pool, exclude_vars)
    d = depth + 1
    r = rng.random()
    if r < 0.35:  # binary arithmetic
        op = rng.choice(['+', '-', '*'])
        return f"({gen_float_expr(rng, pool, d, max_depth, exclude_vars)} {op} {gen_float_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.45:  # unary neg
        return f"-({gen_float_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.60:  # ternary
        return f"({gen_bool_expr(rng, pool, d, max_depth, exclude_vars)} ? {gen_float_expr(rng, pool, d, max_depth, exclude_vars)} : {gen_float_expr(rng, pool, d, max_depth, exclude_vars)})"
    # int-to-float cast
    return f"({gen_int_expr(rng, pool, d, max_depth, exclude_vars)}).tofloat()"


def gen_bool_expr(rng: random.Random, pool, depth: int = 0, max_depth: int = 3,
                  exclude_vars: set[str] = _no_exclude) -> str:
    """Generate a complex bool rvalue expression."""
    if depth >= max_depth or (depth > 0 and rng.random() < 0.3):
        return _bool_atom(rng, pool, exclude_vars)
    d = depth + 1
    r = rng.random()
    if r < 0.20:  # chained NOT
        nots = rng.randint(1, 3)
        return f"{'!' * nots}({gen_bool_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.40:  # binary logic
        op = rng.choice(['&&', '||'])
        return f"({gen_bool_expr(rng, pool, d, max_depth, exclude_vars)} {op} {gen_bool_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.75:  # comparison
        cmp_op = rng.choice(['<', '<=', '>', '>=', '==', '!='])
        if rng.random() < 0.5:
            return f"({gen_int_expr(rng, pool, d, max_depth, exclude_vars)} {cmp_op} {gen_int_expr(rng, pool, d, max_depth, exclude_vars)})"
        return f"({gen_float_expr(rng, pool, d, max_depth, exclude_vars)} {cmp_op} {gen_float_expr(rng, pool, d, max_depth, exclude_vars)})"
    # ternary bool
    return f"({gen_bool_expr(rng, pool, d, max_depth, exclude_vars)} ? {gen_bool_expr(rng, pool, d, max_depth, exclude_vars)} : {gen_bool_expr(rng, pool, d, max_depth, exclude_vars)})"


def gen_string_expr(rng: random.Random, pool, depth: int = 0, max_depth: int = 2,
                    exclude_vars: set[str] = _no_exclude) -> str:
    """Generate a complex string rvalue expression."""
    if depth >= max_depth or (depth > 0 and rng.random() < 0.4):
        return _string_atom(rng, pool, exclude_vars)
    d = depth + 1
    r = rng.random()
    if r < 0.40:  # concat
        return f"({gen_string_expr(rng, pool, d, max_depth, exclude_vars)} + {gen_string_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.60:  # ternary
        return f"({gen_bool_expr(rng, pool, d, max_depth, exclude_vars)} ? {gen_string_expr(rng, pool, d, max_depth, exclude_vars)} : {gen_string_expr(rng, pool, d, max_depth, exclude_vars)})"
    if r < 0.80:  # string method
        fn = rng.choice(['toupper', 'tolower', 'strip'])
        return f"({gen_string_expr(rng, pool, d, max_depth, exclude_vars)}).{fn}()"
    # int-to-string cast
    return f"({gen_int_expr(rng, pool, d, max_depth, exclude_vars)}).tostring()"


def gen_expr(rng: random.Random, pool, typ: str, depth: int = 0, max_depth: int = 3,
             exclude_vars: set[str] = _no_exclude) -> str:
    """Generate a complex rvalue expression of the given type."""
    if typ == INT:
        return gen_int_expr(rng, pool, depth, max_depth, exclude_vars)
    if typ == FLOAT:
        return gen_float_expr(rng, pool, depth, max_depth, exclude_vars)
    if typ == BOOL:
        return gen_bool_expr(rng, pool, depth, max_depth, exclude_vars)
    if typ == STRING:
        return gen_string_expr(rng, pool, depth, max_depth, exclude_vars)
    raise ValueError(f"No expression generator for type {typ}")


# ---------------------------------------------------------------------------
# Template expansion
# ---------------------------------------------------------------------------

# Match $name but not $$, $" (string interp) - only alphanumeric placeholders
PLACEHOLDER_RE = re.compile(r'\$([a-zA-Z_][a-zA-Z0-9_]*)')


LOOP_BOUNDS = [-4, -1, 0, 1, 2, 3, 5, 7, 8]
SHIFT_AMOUNTS = [-100, -65, -64, -63, -33, -32, -31, -17, -15, -9, -7, -2, -1, 0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 100]
_enum_counter = 0


def expand(template: str, bindings: dict[str, str], rng: random.Random,
           pool=None, exclude_vars: set[str] | None = None) -> str:
    """Replace $name placeholders with bound values."""
    cached: dict[str, str] = {}
    def replacer(m):
        key = m.group(1)
        if key in bindings:
            return bindings[key]
        # Complex expression generators (need pool)
        if pool is not None:
            _ev = exclude_vars or set()
            if key == "EXPR_INT":
                return gen_int_expr(rng, pool, exclude_vars=_ev)
            if key == "EXPR_FLOAT":
                return gen_float_expr(rng, pool, exclude_vars=_ev)
            if key == "EXPR_BOOL":
                return gen_bool_expr(rng, pool, exclude_vars=_ev)
            if key == "EXPR_STRING":
                return gen_string_expr(rng, pool, exclude_vars=_ev)
        # Built-in literal generators (each occurrence gets a fresh value)
        if key == "INT":
            return rand_int(rng)
        if key == "FLOAT":
            return rand_float(rng)
        if key == "STRING":
            return rand_string(rng)
        if key == "BOOL":
            return rand_bool(rng)
        if key == "BOUND":
            return str(rng.choice(LOOP_BOUNDS))
        if key == "SHIFT_AMOUNT":
            return str(rng.choice(SHIFT_AMOUNTS))
        # Stable generators (same value for all occurrences in one expansion)
        if key == "ENUM_DECL":
            if key not in cached:
                global _enum_counter
                _enum_counter += 1
                eid = f"{_enum_prefix}E{_enum_counter}"
                cached["ENUM_ID"] = eid
                cached[key] = rng.choice([f"enum {eid}", f"global enum {eid}"])
            return cached[key]
        if key == "ENUM_ID":
            if key not in cached:
                _enum_counter += 1
                cached[key] = f"{_enum_prefix}E{_enum_counter}"
            return cached[key]
        # Not a placeholder - keep the original (e.g. $"..." string interp)
        return m.group(0)
    return PLACEHOLDER_RE.sub(replacer, template)


# ---------------------------------------------------------------------------
# Assembler
# ---------------------------------------------------------------------------

# Variable prefixes per nesting depth (avoid shadowing across scopes)
_VAR_PREFIXES   = ["v", "u", "w", "z"]
_PARAM_PREFIXES = ["p", "q", "r", "s"]
_MAX_DEPTH = 3
_NESTED_PROB = 0.12  # probability of nested function per brick step


def _reindent(code: str, indent: str) -> str:
    """Re-indent a multi-line brick expansion.

    Brick templates use 2-space ("  ") base indent on continuation lines.
    Replace that base with *indent* so the code aligns at any nesting depth.
    """
    parts = code.split("\n")
    if len(parts) == 1:
        return indent + code.lstrip()
    out = [indent + parts[0].lstrip()]
    for part in parts[1:]:
        # Strip the template's 2-space base indent, keep the rest
        if part.startswith("  "):
            out.append(indent + part[2:])
        else:
            out.append(indent + part)
    return "\n".join(out)


_excluded_bricks: set[str] = set()


def _pick_and_emit_brick(rng, pool, indent):
    """Pick a satisfiable brick, bind variables, return code line or None."""
    candidates = [b for i, b in enumerate(BRICKS)
                  if b.name not in _excluded_bricks
                  and pool.can_satisfy(b.pre, _BRICK_PRE_SIGS[i])]
    if not candidates:
        return None

    weights = [b.weight for b in candidates]
    brick = rng.choices(candidates, weights=weights, k=1)[0]

    # Bind inputs (distinct variables)
    bindings: dict[str, str] = {}
    used: set[str] = set()
    for slot in brick.pre:
        matches = pool.find(slot, used)
        chosen = rng.choice(matches)
        bindings[slot.name] = chosen.name
        used.add(chosen.name)

    # Bind outputs
    for slot in brick.post:
        out_type = slot.type
        if isinstance(out_type, tuple):
            out_type = out_type[0]
        if out_type == ANY:
            out_type = INT
        bindings[slot.name] = pool.add(out_type)

    # Bind auxiliary variables (e.g. $out0_i loop counters)
    for m in PLACEHOLDER_RE.finditer(brick.template):
        key = m.group(1)
        if key not in bindings and not key.isupper():
            for slot in brick.post:
                if key.startswith(slot.name):
                    suffix = key[len(slot.name):]
                    bindings[key] = bindings[slot.name] + suffix
                    break

    # Collect output variable names so expression generators skip them
    # (they're in the pool already but not yet defined in code)
    out_names = {bindings[slot.name] for slot in brick.post if slot.name in bindings}
    code = expand(brick.template, bindings, rng, pool=pool, exclude_vars=out_names)

    # 50% chance: inject print() into catch block of try-catch bricks
    if brick.name.startswith("try_catch") and rng.random() < 0.5:
        code = re.sub(
            r'(\} catch \(([^)]*)\) \{\n)',
            lambda m: m.group(1) + f'    print($"caught: {{{m.group(2)}}}\\n")\n',
            code,
            count=1,
        )

    return f"{_reindent(code, indent)}  // [{brick.name}]"


def _make_nested_func(rng, pool, depth, indent):
    """Generate a nested function with an inner brick assembly as body.

    The inner function is a closure: it captures all outer-scope variables
    and can read them directly.  It may also take 0-3 explicit parameters.
    """
    inner_depth = depth + 1
    var_pfx = _VAR_PREFIXES[inner_depth] if inner_depth < len(_VAR_PREFIXES) \
        else f"d{inner_depth}"
    param_pfx = _PARAM_PREFIXES[depth] if depth < len(_PARAM_PREFIXES) \
        else f"a{depth}"

    # Inner pool starts with all outer variables as captures
    inner_pool = VarPool(prefix=var_pfx)
    outer_names = set()
    for pv in pool.vars:
        inner_pool.add_existing(pv.name, pv.type)
        outer_names.add(pv.name)

    # 0-3 explicit parameters (optional - can be a pure closure)
    scalar_vars = [pv for pv in pool.vars if pv.type in (INT, FLOAT, STRING, BOOL)]
    max_params = min(3, len(scalar_vars))
    num_params = rng.randint(0, max_params) if scalar_vars else 0
    chosen_params = rng.sample(scalar_vars, num_params) if num_params > 0 else []

    param_names = []
    for i, pv in enumerate(chosen_params):
        pname = f"{param_pfx}{i}"
        param_names.append(pname)
        inner_pool.add_existing(pname, pv.type)

    # Assemble inner body (fewer bricks at deeper levels)
    inner_num = rng.randint(3, max(3, 8 - depth * 2))
    inner_indent = indent + "  "
    inner_lines = _assemble_bricks(rng, inner_pool, inner_num, inner_depth, inner_indent)
    if not inner_lines:
        return None

    # Return variable must be newly created (not an outer capture or parameter)
    reserved = outer_names | set(param_names)
    return_candidates = [pv for pv in inner_pool.vars
                         if pv.type in (INT, FLOAT, STRING, BOOL)
                         and pv.name not in reserved]
    if not return_candidates:
        return None
    return_var = return_candidates[-1]

    # Allocate output in outer pool
    out_name = pool.add(return_var.type)
    fn_name = f"{out_name}_fn"

    param_str = ", ".join(param_names)
    arg_str = ", ".join(pv.name for pv in chosen_params)

    result = []
    result.append(f"{indent}let {fn_name} = function({param_str}) {{")
    result.extend(inner_lines)
    result.append(f"{inner_indent}return {return_var.name}")
    result.append(f"{indent}}}")
    result.append(f"{indent}let {out_name} = {fn_name}({arg_str})"
                  f"  // [nested_func depth={inner_depth}]")
    return result


def _assemble_bricks(rng, pool, num_bricks, depth=0, indent="  "):
    """Core assembly loop. Returns list of code lines."""
    lines = []
    for step in range(num_bricks):
        # Try generating a nested function (not on first 2 steps, need vars)
        if depth < _MAX_DEPTH and step >= 2 and rng.random() < _NESTED_PROB:
            nested = _make_nested_func(rng, pool, depth, indent)
            if nested:
                lines.extend(nested)
                continue

        line = _pick_and_emit_brick(rng, pool, indent)
        if line is None:
            break
        lines.append(line)
    return lines


_enum_prefix = ""


def assemble(seed: int, num_bricks: int = 15) -> str:
    """Assemble a Quirrel program from code bricks."""
    global _enum_counter
    _enum_counter = 0
    rng = random.Random(seed)
    pool = VarPool()
    lines = [f"// Code Bricks assembly - seed {seed}"]

    lines.extend(_assemble_bricks(rng, pool, num_bricks, depth=0, indent="  "))

    # Print all variables at the end
    lines.append("")
    lines.append("  // --- dump pool ---")
    lines.append('  function _prn_t(_t) { let _a = []; _t.each(function(_k, _v) { _a.append(_sv(_k)); _a.append(_sv(_v)) }); _a.sort(); foreach (i, _v in _a) { if (i >= 10) { print("..."); break }; if (i > 0) print(", "); print(_v) }; }')
    lines.append('  function _prn_a(_a) { foreach (i, _v in _a) { if (i >= 10) { print("..."); break }; if (i > 0) print(", "); print(_v) }; }')

    for pv in pool.vars:
        if pv.type == ARRAY:
            lines.append(f'  print("{pv.name}=["); _prn_a({pv.name}); print("]\\n")')
        elif pv.type == TABLE:
            lines.append(f'  print("{pv.name}={{"); _prn_t({pv.name}); print("}}\\n")')
        else:
            lines.append(f'  print($"{pv.name}={{{pv.name}}}\\n")')

    header = ("#allow-switch-statement\n#allow-delete-operator\n" +
    'let _sv = @(_e) (type(_e) == "integer" || type(_e) == "float" || type(_e) == "null" || type(_e) == "bool" || type(_e) == "string") ? "" + _e : type(_e)\n' +
    "try {\n")
    return header + "\n".join(lines) + "\n} catch(e) { print(e) }\n"


# ---------------------------------------------------------------------------
# Pool stats
# ---------------------------------------------------------------------------

def pool_stats(seed: int, num_bricks: int = 15):
    """Show which bricks were assembled and the final pool state."""
    rng = random.Random(seed)
    pool = VarPool()
    used_bricks = []

    for _ in range(num_bricks):
        candidates = [b for i, b in enumerate(BRICKS) if pool.can_satisfy(b.pre, _BRICK_PRE_SIGS[i])]
        if not candidates:
            break
        weights = [b.weight for b in candidates]
        brick = rng.choices(candidates, weights=weights, k=1)[0]
        used: set[str] = set()
        for slot in brick.pre:
            matches = pool.find(slot, used)
            chosen = rng.choice(matches)
            used.add(chosen.name)
        for slot in brick.post:
            out_type = slot.type
            if isinstance(out_type, tuple):
                out_type = out_type[0]
            if out_type == ANY:
                out_type = INT
            pool.add(out_type)
        used_bricks.append(brick.name)

    print(f"Seed {seed}: {len(used_bricks)} bricks assembled")
    print(f"  Bricks: {', '.join(used_bricks)}")
    print(f"  Pool ({len(pool.vars)} vars):")
    by_type: dict[str, int] = {}
    for pv in pool.vars:
        by_type[pv.type] = by_type.get(pv.type, 0) + 1
    for t, n in sorted(by_type.items()):
        print(f"    {t}: {n}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    args = sys.argv[1:]

    if args and args[0] == "--stats":
        seed = int(args[1]) if len(args) > 1 else 42
        count = int(args[2]) if len(args) > 2 else 1
        for i in range(count):
            pool_stats(seed + i)
        return

    seed = int(args[0]) if len(args) > 0 else 42
    count = int(args[1]) if len(args) > 1 else 1
    bricks_per_program = int(args[2]) if len(args) > 2 else 15

    for i in range(count):
        s = seed + i
        program = assemble(s, bricks_per_program)
        if count == 1:
            print(program)
        else:
            filename = f"out_{s}.nut"
            with open(filename, "w") as f:
                f.write(program)
            print(f"  wrote {filename}")


if __name__ == "__main__":
    main()

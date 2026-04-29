AST pattern matching via reverse reification. Matches expressions and blocks against structural
patterns using the same tag system as ``qmacro`` but in reverse: ``$e`` clones a matched expression,
``$v`` extracts a constant value, ``$i`` extracts an identifier name, ``$c`` extracts a call name,
``$f`` extracts a field name, ``$b`` captures a statement range, ``$t`` captures a type,
and ``$a`` captures remaining arguments.

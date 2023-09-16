.. _pattern-matching:

================
Pattern matching
================

In the world of computer programming, there is a concept known as pattern matching.
This technique allows us to take a complex value, such as an array or a variant, and compare it to a set of patterns.
If the value fits a certain pattern, the matching process continues and we can extract specific values from that value.
This is a powerful tool for making our code more readable and efficient,
and in this section we'll be exploring the different ways that pattern matching can be used in daScript.

In daScript pattern matching is implement via macros in the `daslib/match` module.

^^^^^^^^^^^^^^^^^^^^
Enumeration Matching
^^^^^^^^^^^^^^^^^^^^

daScript supports pattern matching on enumerations, which allows you to match the value of an enumeration with specific patterns.
You can use this feature to simplify your code by eliminating the need for multiple if-else statements or switch statements.
To match enumerations in daScript, you use the match keyword followed by the enumeration value, and a series of if statements,
each representing a pattern to match. If a match is found, the corresponding code block is executed.

Example::

    enum Color
        Black
        Red
        Green
        Blue

    def enum_match (color:Color)
        match color
            if Color Black
                return 0
            if Color Red
                return 1
            if _
                return -1

In the example, the enum_match function takes a Color enumeration value as an argument and returns a value based on the matched pattern.
The if Color Black statement matches the Black enumeration value, the if Color Red statement matches the Red enumeration value,
and the if _ statement is a catch-all that matches any other enumeration value that hasn't been explicitly matched.

^^^^^^^^^^^^^^^^^^^^
Matching Variants
^^^^^^^^^^^^^^^^^^^^

Variants in daScript can be matched using the match statement.
A variant is a discriminated union type that holds one of several possible values, each of a different type.

In the example, the IF variant has two possible values: i of type int, and f of type float.
The variant_as_match function takes a value of type IF as an argument, and matches it to determine its type.

The if _ as i statement matches any value and assigns it to the declared variable i.
Similarly, the if _ as f statement matches any value and assigns it to the declared variable f.
The final if _ statement matches any remaining values, and returns "anything".

Example::

    variant IF
        i : int
        f : float

    def variant_as_match (v:IF)
        match v
            if _ as i
                return "int"
            if _ as f
                return "float"
            if _
                return "anything"

Variants can be matched in daScript using the same syntax used to create new variants.

Here's an example::

    def variant_match (v : IF)
        match v
            if [[IF i=$v(i)]]
                return 1
            if [[IF f=$v(f)]]
                return 2
            if _
                return 0

In the example above, the function variant_match takes a variant v of type IF. The first case matches v if it contains an i and binds the value of i to a variable i.
In this case, the function returns 1. The second case matches v if it contains an f and binds the value of f to a variable f. In this case, the function returns 2. T
he last case matches anything that doesn't match the first two cases and returns 0.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Declaring Variables in Pattern Matching
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In daScript, you can declare variables in pattern matching statements, including variant matching.
To declare a variable, use the syntax $v(decl) where decl is the name of the variable being declared.
The declared variable is then assigned the value of the matched pattern.

This feature is not restricted to variant matching, and can be used in any pattern matching statement in daScript.
In the example, the if $v(as_int) statement matches the variant value when it holds an integer and declares a variable as_int
to store the value. Similarly, the if $v(as_float) statement matches the variant value when it holds a floating-point value and declares a variable as_float to store the value.

Example::

    variant IF
        i : int
        f : float

    def variant_as_match (v:IF)
        match v
            if $v(as_int) as i
                return as_int
            if $v(as_float) as f
                return as_float
            if _
                return None

^^^^^^^^^^^^^^^^^^^^
Matching Structs
^^^^^^^^^^^^^^^^^^^^

daScript supports matching structs using the match statement.
A struct is a composite data type that groups variables of different data types under a single name.

In the example, the Foo struct has one member a of type int.
The struct_match function takes an argument of type Foo, and matches it against various patterns.

The first match if [[Foo a=13]] matches a Foo struct where a is equal to 13, and returns 0 if this match succeeds.
The second match if [[Foo a=$v(anyA)]] matches any Foo struct and binds its a member to the declared variable anyA.
This match returns the value of anyA if it succeeds.

Example::

    struct Foo
        a : int

    def struct_match (f:Foo)
        match f
            if [[Foo a=13]]
                return 0
            if [[Foo a=$v(anyA)]]
                return anyA

^^^^^^^^^^^^^^^^^^^^
Using Guards
^^^^^^^^^^^^^^^^^^^^

daScript supports the use of guards in its pattern matching mechanism.
Guards are conditions that must be satisfied in addition to a successful pattern match.

In the example, the AB struct has two members a and b of type int.
The guards_match function takes an argument of type AB, and matches it against various patterns.

The first match if [[AB a=$v(a), b=$v(b)]] && (b > a) matches an AB struct and binds its a and b members to the declared variables a and b, respectively.
The guard condition b > a must also be satisfied for this match to be successful. If this match succeeds, the function returns a string indicating that b is greater than a.

The second match if [[AB a=$v(a), b=$v(b)]] matches any AB struct and binds its a and b members to the declared variables a and b, respectively.
No additional restrictions are placed on the match by means of a guard. If this match succeeds, the function returns a string indicating that b is less than or equal to a.

Example::

    struct AB
        a, b : int

    def guards_match (ab:AB)
        match ab
            if [[AB a=$v(a), b=$v(b)]] && (b > a)
                return "{b} > {a}"
            if [[AB a=$v(a), b=$v(b)]]
                return "{b} <= {a}"

^^^^^^^^^^^^^^^^^^^^
Tuple Matching
^^^^^^^^^^^^^^^^^^^^

Matching tuples in daScript is done with double square brackets and uses the same syntax as creating a new tuple.
The type of the tuple must be specified or auto can be used to indicate automatic type inference.

Here is an example that demonstrates tuple matching in daScript::

    def tuple_match ( A : tuple<int;float;string> )
        match A
            if [[auto 1,_,"3"]]
                return 1
            if [[auto 13,...]]      // starts with 13
                return 2
            if [[auto ...,"13"]]    // ends with "13"
                return 3
            if [[auto 2,...,"2"]]   // starts with 2, ends with "2"
                return 4
            if _
                return 0

In this example, a tuple A of type tuple<int;float;string> is passed as an argument to the function tuple_match.
The function uses a match statement to match different patterns in the tuple A.
The if clauses inside the match statement use double square brackets to specify the pattern to be matched.

The first pattern to be matched is [[auto 1,_,"3"]].
The pattern matches a tuple that starts with the value 1, followed by any value, and ends with the string "3".
The _ symbol in the pattern indicates that any value can be matched at that position in the tuple.

The second pattern to be matched is [[auto 13,...]], which matches a tuple that starts with the value 13.
The ... symbol in the pattern indicates that any number of values can be matched after the value 13.

The third pattern to be matched is [[auto ...,"13"]], which matches a tuple that ends with the string "13".
The ... symbol in the pattern indicates that any number of values can be matched before the string "13".

The fourth pattern to be matched is [[auto 2,...,"2"]], which matches a tuple that starts with the value 2 and ends with the string "2".

If none of the patterns match, the _ clause is executed and the function returns 0.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Matching Static Arrays
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Static arrays in daScript can be matched using the double square bracket syntax, similarly to tuples.
Additionally, static arrays must have their type specified, or the type can be automatically inferred using the auto keyword.

Here is an example of matching a static array of type int[3]::

    def static_array_match ( A : int[3] )
        match A
            if [[auto $v(a);$v(b);$v(c)]] && (a+b+c)==6 // total of 3 elements, sum is 6
                return 1
            if [[int 0;...]]    // starts with 0
                return 0
            if [[int ...;13]]   // ends with 13
                return 2
            if [[int 12;...;12]]    // starts and ends with 12
                return 3
            if _
                return -1

In this example, the function static_array_match takes an argument of type int[3], which is a static array of three integers.
The match statement uses the double square bracket syntax to match against different patterns of the input array A.

The first case, [[auto $v(a);$v(b);$v(c)]] && (a+b+c)==6, matches an array where the sum of its three elements is equal to 6.
The matched elements are assigned to variables a, b, and c using the $v syntax.

The next three cases match arrays that start with 0, end with 13, and start and end with 12, respectively.
The ... syntax is used to match any elements in between.

Finally, the _ case matches any array that does not match any of the other cases, and returns -1 in this case.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Dynamic Array Matching
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Dynamic arrays are used to store a collection of values that can be changed during runtime.
In daScript, dynamic arrays can be matched with patterns using similar syntax as for tuples, but with the added check for the number of elements in the array.

Here is an example of matching on a dynamic array of integers::

    def dynamic_array_match ( A : array<int> )
        match A
            if [{auto $v(a);$v(b);$v(c)}] && (a+b+c)==6 // total of 3 elements, sum is 6
                return 1
            if [{int 0;0;0;...}]    // first 3 are 0
                return 0
            if [{int ...;1;2}]      // ends with 1,2
                return 2
            if [{int 0;1;...;2;3}]    // starts with 0,1, ends with 2,3
                return 3
            if _
                return -1

In the code above, the dynamic_array_match function takes a dynamic array of integers as an argument.
The match statement then tries to match the elements in the array against a series of patterns.

The first pattern if [{auto $v(a);$v(b);$v(c)}] && (a+b+c)==6 matches arrays that contain three elements and the sum of those elements is 6.
The $v syntax is used to match and capture the values of the elements in the array. The captured values can then be used in the condition (a+b+c)==6.

The second pattern if [{int 0;0;0;...}] matches arrays that start with three zeros. The ... syntax is used to match any remaining elements in the array.

The third pattern if [{int ...;1;2}] matches arrays that end with the elements 1 and 2.

The fourth pattern if [{int 0;1;...;2;3}] matches arrays that start with the elements 0 and 1 and end with the elements 2 and 3.

The final pattern if _ matches any array that didn't match any of the previous patterns.

It is important to note that the number of elements in the dynamic array must match the number of elements in the pattern for the match to be successful.

^^^^^^^^^^^^^^^^^^^^
Match Expressions
^^^^^^^^^^^^^^^^^^^^

In daScript, match expressions allow you to reuse variables declared earlier in the pattern to match expressions later in the pattern.

Here's an example that demonstrates how to use match expressions to check if an array of integers is in ascending order::

    def ascending_array_match ( A : int[3] )
        match A
            if [[int $v(x);match_expr(x+1);match_expr(x+2)]]
                return true
            if _
                return false

In this example, the first element of the array is matched to x. Then, the next two elements are matched using match_expr and the expression x+1 and x+2 respectively.
If all three elements match, the function returns true. If there is no match, the function returns false.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Matching with || Expression
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In daScript, you can use the || expression to match either of the provided options in the order they appear. This is useful when you want to match a variant based on multiple criteria.

Here is an example of matching with || expression::

    struct Bar
        a : int
        b : float

    def or_match ( B:Bar )
        match B
            if [[Bar a=1, b=$v(b)]] || [[Bar a=2, b=$v(b)]]
                return b
            if _
                return 0.0

In this example, the function or_match takes a variant B of type Bar and matches it using the || expression.
The first option matches when the value of a is 1 and b is captured as a variable.
The second option matches when the value of a is 2 and b is captured as a variable.
If either of these options match, the value of b is returned. If neither of the options match, 0.0 is returned.

It's important to note that for the || expression to work, both sides of the statement must declare the same variables.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
[match_as_is] Structure Annotation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The [match_as_is] structure annotation in daScript allows you to perform pattern matching for structures of different types.
This allows you to match structures of different types in a single pattern matching expression,
as long as the necessary is and as operators have been implemented for the matching types.

Here's an example of how to use the [match_as_is] structure annotation::

    [match_as_is]
    struct CmdMove : Cmd
        override rtti = "CmdMove"
        x : float
        y : float

In this example, the structure CmdMove is marked with the [match_as_is] annotation, allowing it to participate in pattern matching::

    def operator is CmdMove ( cmd:Cmd )
        return cmd.rtti=="CmdMove"

    def operator is CmdMove ( anything )
        return false

    def operator as CmdMove ( cmd:Cmd ==const ) : CmdMove const&
        assert(cmd.rtti=="CmdMove")
        unsafe
            return reinterpret<CmdMove const&> cmd

    def operator as CmdMove ( var cmd:Cmd ==const ) : CmdMove&
        assert(cmd.rtti=="CmdMove")
        unsafe
            return reinterpret<CmdMove&> cmd

    def operator as CmdMove ( anything )
        panic("Cannot cast to CmdMove")
        return [[CmdMove]]

    def matching_as_and_is (cmd:Cmd)
        match cmd
            if [[CmdMove x=$v(x), y=$v(y)]]
                return x + y
            if _
                return 0.

In this example, the necessary is and as operators have been implemented for the CmdMove structure to allow it to participate in pattern matching. The is operator is used to determine the compatibility of the types, and the as operator is used to perform the actual type casting.

In the matching_as_and_is function, cmd is matched against the CmdMove structure using the [[CmdMove x=$v(x), y=$v(y)]] pattern. If the match is successful, the values of x and y are extracted and the sum is returned. If the match is not successful, the catch-all _ case is matched, and 0.0 is returned.

**Note** that the [match_as_is] structure annotation only works if the necessary is and as operators have been implemented for the matching types. In the example above, the necessary is and as operators have been implemented for the CmdMove structure to allow it to participate in pattern matching.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
[match_copy] Structure Annotation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The [match_copy] structure annotation in daScript allows you to perform pattern matching for structures of different types.
This allows you to match structures of different types in a single pattern matching expression,
as long as the necessary match_copy function has been implemented for the matching types.

Here's an example of how to use the [match_copy] structure annotation::

    [match_copy]
    struct CmdLocate : Cmd
        override rtti = "CmdLocate"
        x : float
        y : float
        z : float

In this example, the structure CmdLocate is marked with the [match_copy] annotation, allowing it to participate in pattern matching.

The match_copy function is used to match structures of different types. Here's an example of the implementation of the match_copy function for the CmdLocate structure::

    def match_copy ( var cmdm:CmdLocate; cmd:Cmd )
        if cmd.rtti != "CmdLocate"
            return false
        unsafe
            cmdm = reinterpret<CmdLocate const&> cmd
        return true

In this example, the match_copy function takes two parameters: cmdm of type CmdLocate and cmd of type Cmd.
The purpose of this function is to determine if the cmd parameter is of type CmdLocate.
If it is, the function performs a type cast to CmdLocate using the reinterpret, and assigns the result to cmdm.
The function then returns true to indicate that the type cast was successful. If the cmd parameter is not of type CmdLocate, the function returns false.

Here's an example of how the match_copy function is used in a matching_copy function::

    def matching_copy ( cmd:Cmd )
        match cmd
            if [[CmdLocate x=$v(x), y=$v(y), z=$v(z)]]
                return x + y + z
            if _
                return 0.

In this example, the matching_copy function takes a single parameter cmd of type Cmd. This function performs a type matching operation on the cmd parameter to determine its type.
If the cmd parameter is of type CmdLocate, the function returns the sum of the values of its x, y, and z fields. If the cmd parameter is of any other type, the function returns 0.

**Note** that the [match_copy] structure annotation only works if the necessary match_copy function has been implemented for the matching types.
In the example above, the necessary match_copy function has been implemented for the CmdLocate structure to allow it to participate in pattern matching.

^^^^^^^^^^^^^^^^^^^^
Static Matching
^^^^^^^^^^^^^^^^^^^^

Static matching is a way to match on generic expressions daScript. It works similarly to regular matching, but with one key difference:
when there is a type mismatch between the match expression and the pattern, the match will be ignored at compile-time, as opposed to a compilation error.
This makes static matching robust for generic functions.

The syntax for static matching is as follows::

    static_match match_expression
        if pattern_1
            return result_1
        if pattern_2
            return result_2
        ...
        if _
            return result_default

Here, match_expression is the expression to be matched against the patterns. Each pattern is a value or expression that the match_expression will be compared against.
If the match_expression matches one of the patterns, the corresponding result will be returned. If none of the patterns match, the result_default will be returned.
If pattern can't be matched, it will be ignored.

Here is an example::

    enum Color
        red
        green
        blue

    def enum_static_match ( color, blah )
        static_match color
            if Color red
                return 0
            if match_expr(blah)
                return 1
            if _
                return -1

In this example, color is matched against the enumeration values red, green, and blue. If the match expression color is equal to the enumeration value red, 0 will be returned.
If the match expression color is equal to the value of blah, 1 will be returned. If none of the patterns match, -1 will be returned.

**Note** that match_expr is used to match blah against the match expression color, rather than directly matching blah against the enumeration value.

If color is not Color first match will fail. If blah is not Color, second match will fail. But the function will always compile.

^^^^^^^^^^
match_type
^^^^^^^^^^

The match_type subexpression in daScript allows you to perform pattern matching based on the type of an expression.
It is used within the static_match statement to specify the type of expression that you want to match.

The syntax for match_type is as follows::

    if match_type<Type> expr
        // code to run if match is successful

where Type is the type that you want to match and. expr is the expression that you want to match against.

Here's an example of how to use the match_type subexpression::

    def static_match_by_type (what)
        static_match what
            if match_type<int> $v(expr)
                return expr
            if _
                return -1

In this example, what is the expression that is being matched. If what is of type int, then it is assigned to the variable $v and the expression expr is returned. If what is not of type int, the match falls through to the catch-all _ case, and -1 is returned.

**Note** that the match_type subexpression only matches types, and mismatched values are ignored. This is in contrast to regular pattern matching, where both type and value must match for a match to be successful.

^^^^^^^^^^^^^^^^^^^^^^^^^
Multi-Match
^^^^^^^^^^^^^^^^^^^^^^^^^

In daScript, you can use the multi_match feature to match multiple values in a single expression. This is useful when you want to match a value based on several different conditions.

Here is an example of using the multi_match feature::

    def multi_match_test ( a:int )
        var text = "{a}"
        multi_match a
            if 0
                text += " zero"
            if 1
                text += " one"
            if 2
                text += " two"
            if $v(a) && (a % 2 == 0) && (a!=0)
                text += " even"
            if $v(a) && (a % 2 == 1)
                text += " odd"
        return text

In this example, the function multi_match_test takes an integer value a and matches it using the multi_match feature.
The first three options match when a is equal to 0, 1, or 2, respectively.
The fourth option matches when a is not equal to 0 and is an even number.
The fifth option matches when a is an odd number. The variable text is updated based on the matching conditions.
The final result is returned as the string representation of text.

It's important to note that the multi_match feature allows for multiple conditions to be matched in a single expression.
This makes the code more concise and easier to read compared to using multiple match and if statements.

The same example using regular match would look like this::

    def multi_match_test ( a:int )
        var text = "{a}"
        match a
            if 0
                text += " zero"
        match a
            if 1
                text += " one"
        match a
            if 2
                text += " two"
        match a
            if $v(a) && (a % 2 == 0) && (a!=0)
                text += " even"
        match a
            if $v(a) && (a % 2 == 1)
                text += " odd"
        return text

`static_multi_match` is a variant of `multi_match` that works with `static_match`.


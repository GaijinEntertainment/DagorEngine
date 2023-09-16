This macro convert macro_verify(expr,message,prog,at) to the following code::
   if !expr
       macro_error(prog,at,message)
       return [[ExpressionPtr]]

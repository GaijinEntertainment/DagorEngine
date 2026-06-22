// positional {N} index out of range -> left as literal text
let a = "{0}:{1}:{2}".subst(10, 20)
let b = "{0} {1}".subst("only")
// these must NOT warn:
let ok1 = "{0}:{1}".subst(1, 2)
let ok2 = "{0}".subst(1, 2, 3)
let named = "{foo}".subst({foo=1})
let regex = @"https?:\/\/{0}\/\w{16}\/$".subst("host")
let lone = "{5}".subst(1)
return [a, b, ok1, ok2, named, regex, lone]

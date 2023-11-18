//expect:w286


local cls = class {

}



return cls || ::fn2 //-const-in-bool-expr -undefined-global

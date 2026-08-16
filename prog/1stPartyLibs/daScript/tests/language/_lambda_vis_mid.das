options gen2

// helper for lambda_in_generic_module_vis: generics whose bodies reach inner_dot through
// every block-outlining shape. Consumers require THIS module only — inner_dot must resolve
// via this module's (non-public) require, from the generic instance AND from the functions
// its lambdas/generators/local functions get outlined into.
module _lambda_vis_mid shared public

require _lambda_vis_inner

def vis_direct(x : auto) : float {
    return inner_dot(float(x), 2.0)
}

def vis_lambda(x : auto) : float {
    var lam <- @() : float {
        return inner_dot(float(x), 3.0)
    }
    let r = invoke(lam)
    unsafe {
        delete lam
    }
    return r
}

def vis_generator(x : auto) : float {
    var r = 0.0
    var gg <- generator<float> {
        yield inner_dot(float(x), 4.0)
        return false
    }
    for (v in gg) {
        r = v
    }
    return r
}

def vis_local_function(x : auto) : float {
    let fp = @@(a : float) : float {
        return inner_dot(a, 5.0)
    }
    return invoke(fp, float(x))
}

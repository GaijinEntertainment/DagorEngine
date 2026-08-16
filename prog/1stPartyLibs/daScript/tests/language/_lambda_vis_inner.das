options gen2

// helper for lambda_in_generic_module_vis: the symbol that must resolve from the
// generic's home module, NOT from the consumer (which never requires this module)
module _lambda_vis_inner shared public

def inner_dot(a, b : float) : float => a * b + 1.0

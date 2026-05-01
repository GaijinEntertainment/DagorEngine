require ecs
require PhysVars
require DagorRandom

def setup_var(var phys_vars : PhysVars&; name : string; val : float)
  phys_vars |> setVar(phys_vars |> registerVar(name, val), val)

[es(on_appear)]
def phys_vars_fixed_init_es(evt : Event;
                            phys_vars__fixedInit : Object;
                            var phys_vars : PhysVars&)
  for fixedInit in phys_vars__fixedInit
    phys_vars |> setup_var(fixedInit.key, get_float(fixedInit.value) ?? 0.)

[es(on_appear, before=anim_phys_init_es)]
def phys_vars_random_init_es(evt : Event;
                             phys_vars__randomInit : Object;
                             var phys_vars : PhysVars&)
  for randomInit in phys_vars__randomInit
    let randomLimits = get_Point2(randomInit.value) ?? float2()
    phys_vars |> setup_var(randomInit.key, rnd_float(randomLimits.x, randomLimits.y))

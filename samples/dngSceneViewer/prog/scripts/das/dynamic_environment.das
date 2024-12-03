require ecs
require math
require math.base
require daSkies
require GamePhys

[es(tag=render, no_order)]
def clouds_wind_move(info : UpdateStageInfoAct;
                     dyn_wind__cumulus_alt_multiplier : float; dyn_wind__strata_alt_multiplier : float)
  // Clouds are moving to wind direction, origin is moving in opposite direction
  let originDir = -get_wind().xz * info.dt

  let cumulusCloudsMovement = originDir * dyn_wind__cumulus_alt_multiplier
  move_cumulus_clouds(cumulusCloudsMovement)
  let strataCloudsMovement = originDir * dyn_wind__strata_alt_multiplier
  move_strata_clouds(strataCloudsMovement)
local readyRefreshTime = 1;
local timeLeftToUpdate = 2;
local refreshPeriod = 3;

function refreshOnWindowActivate(repeatAmount = 1, refreshPeriodSec = 10.0) {
    readyRefreshTime = 0
    timeLeftToUpdate = repeatAmount
    refreshPeriod = refreshPeriodSec
  }


function xxx(_a, _b, _c) {

}


xxx(readyRefreshTime, timeLeftToUpdate, refreshPeriod)
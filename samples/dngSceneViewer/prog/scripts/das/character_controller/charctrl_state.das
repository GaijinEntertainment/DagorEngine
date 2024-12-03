enum CCStandType
  ESS_STAND
  ESS_CROUCH
  ESS_CRAWL

enum CCMoveType
  EMS_STAND
  EMS_WALK
  EMS_RUN
  EMS_SPRINT

enum CCStateFlag
  ST_ON_GROUND = 1 << 0
  ST_JUMP      = 1 << 1
  ST_CROUCH    = 1 << 2
  ST_CRAWL     = 1 << 3
  ST_SWIM      = 1 << 4

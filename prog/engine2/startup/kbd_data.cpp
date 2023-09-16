#include <humanInput/dag_hiGlobals.h>

HumanInput::KeyboardSettings HumanInput::stg_kbd = {false, false, true};

HumanInput::KeyboardRawState HumanInput::raw_state_kbd = {
  {0, 0, 0, 0, 0, 0, 0, 0}, // unsigned bitMask[256/32];
  0                         // unsigned shifts;
};

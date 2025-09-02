#ifndef FLEXGRID_CONSTS_INCLUDED
#define FLEXGRID_CONSTS_INCLUDED 1

static const int FLEXGRID_PATCH_QUADS = 8;
static const float FLEXGRID_QUAD_SIZE = 1.0 / 8.0;
static const float FLEXGRID_PARENT_SCALE = 2.0f;
static const float FLEXGRID_GRANDPARENT_SCALE = 4.0f;

static const int FLEXGRID_MAX_INSTANCES_PER_DRAW = 490;
static const int FLEXGRID_REGISTERS_PER_INSTANCE = 4;
static const int FLEXGRID_MAX_REGS = 512 * 4;
static const int FLEXGRID_INSTANCE_DATA_CB_REG = 70;
#endif
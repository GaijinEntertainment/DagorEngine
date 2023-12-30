//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// graph nodes and other graph structures
class IAsgGraphNodeManager;
class AsgGraphNode;

// animation tree structures
struct AnimObjGraphTree;
struct AnimObjBnl;
struct AnimObjCtrl;
struct AnimObjCtrlFifo;
struct AnimObjCtrlFifo3;
struct AnimObjCtrlLinear;
struct AnimObjCtrlRandomSwitch;
struct AnimObjCtrlParametricSwitch;
struct AnimObjCtrlHub;
struct AnimObjCtrlDirectSync;
struct AnimObjCtrlNull;
struct AnimObjCtrlBlender;
struct AnimObjCtrlBIS;

// animation graph
struct AnimGraphCondition;
struct AnimGraphTimedAction;
struct AnimGraphState;
struct AsgStatesGraph;

// properties
struct AsgStateGenParameter;
struct AsgStateGenParamGroup;

// export to C++
class AsgVarsList;
class AsgLocalVarsList;
class AsgConditionStringParser;

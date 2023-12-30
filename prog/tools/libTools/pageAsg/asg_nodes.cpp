#include <libTools/pageAsg/asg_nodes.h>
#include <libTools/pageAsg/asg_node_mgr.h>
#include <libTools/pageAsg/asg_data.h>

AsgGraphNode::AsgGraphNode(IAsgGraphNodeManager &_idx, const AnimGraphState &gs, int state_id, bool _uplink, bool is_group) :
  idx(_idx), baseConditions(midmem)
{
  uplink = _uplink;
  isGroup = is_group;
  stateId = state_id;
  inLinkCnt = 0;

  int i, l;

  for (i = 0; i < gs.whileCond.size(); i++)
  {
    l = append_items(baseConditions, 1);
    baseConditions[l].cond = gs.whileCond[i]->condition;
    baseConditions[l].targetStateId = idx.findState(gs.whileCond[i]->targetName);
    baseConditions[l].branchType = AGBT_CheckAlways;
    baseConditions[l].ordinal = i;
  }
  for (i = 0; i < gs.endCond.size(); i++)
  {
    l = append_items(baseConditions, 1);
    baseConditions[l].cond = gs.endCond[i]->condition;
    baseConditions[l].targetStateId = idx.findState(gs.endCond[i]->targetName);
    baseConditions[l].branchType = AGBT_CheckAtEnd;
    baseConditions[l].ordinal = i;
  }
}

void AsgGraphNode::resetInCount() { inLinkCnt = 0; }
void AsgGraphNode::addLink(AsgGraphNode *dest, AnimGraphBranchType btype, const char *cond)
{
  int l = append_items(baseConditions, 1);
  baseConditions[l].cond = cond;
  baseConditions[l].targetStateId = dest->stateId;
  baseConditions[l].branchType = btype;
  baseConditions[l].ordinal = -1;

  dest->inLinkCnt++;
}

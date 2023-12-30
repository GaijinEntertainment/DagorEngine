#include <libTools/pageAsg/asg_data.h>
#include <libTools/pageAsg/asg_scheme.h>
#include <libTools/pageAsg/asg_sysHelper.h>
#include "asg_cond_parser.h"

#include <osApiWrappers/dag_direct.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

#include <stdio.h>
#if _TARGET_PC_WIN
#include <io.h>
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#endif

#define INSERT_THOROUGH_LOG(text) \
  if (thorough_logs)              \
    fprintf(fp, "cdebug(\"%%d: " text "\", __LINE__);\n");

#define INSERT_THOROUGH_LOG_T2(text, text2) \
  if (thorough_logs)                        \
    fprintf(fp, "cdebug(\"%%d: " text "\", __LINE__, " text2 ");\n");

#define INSERT_THOROUGH_LOG_2(text, p1, p2) \
  if (thorough_logs)                        \
    fprintf(fp, "cdebug(\"%%d: " text "\", __LINE__);\n", p1, p2);

bool AsgStatesGraph::exportCppCode(const char *dest_fname, int debug_lev, AsgStateGenParamGroup *rs, const char *res_name)
{
  bool thorough_logs = debug_lev > 2;
  bool compileRes = (res_name == NULL ? true : false);
  translator = (compileRes ? "trans" : "anim_iface_trans");
  gtMaskForMissingAnimWithAbsPos = 0;

  // check what can be checked easily
  if (flagList.nameCount() > 32 || classList.nameCount() > 32)
  {
    page_sys_hlp->postMsg(true, "Need build upgrade",
      "flagList.nameCount()=%d > 32 || classList.nameCount()=%d > 32\n"
      "Build algorithm must be upgraded by Nic Tiger\n"
      "(optionally number of used flags/classes can be reduced to be under 32)",
      flagList.nameCount(), classList.nameCount());
    return false;
  }

  for (int i = 0; i < gt.size(); i++)
  {
    int ind = findState(gt[i].startNodeName);

    // check ok
    if (ind != -1 && states[ind]->gt == i)
      continue;

    debug_ctx("start node for thread #%d <%s> is not found or doesn't belong to that thread", i, (char *)gt[i].startNodeName);
    page_sys_hlp->postMsg(true, "Incorrect thread start node",
      "start node for thread #%d <%s> is not found or doesn't belong to that thread", i, gt[i].startNodeName.str());
    return false;
  }

  String fname, errname;
  String tmp_fname;
  if (compileRes)
  {
    String basedir(260, "%s/.local/sg-pdl-%s", page_sys_hlp->getDagorCdkDir(), debug_lev ? "dbg" : "rel");
    dd_mkdir(basedir);
    dd_erase(basedir + "/_test.cpp");
    dd_erase(basedir + "/_test.err");
    dd_erase(basedir + "/_test.cpp.exp");
    dd_erase(basedir + "/exData.h");
    dd_erase(basedir + "/_test.cpp.lib");
    dd_erase(basedir + "/_test.cpp.map");
    dd_erase(basedir + "/_test.cpp.pdb");
    dd_erase(basedir + "/_test.cpp.pdl");
    dd_erase(dest_fname);

    AsgStateGenParamGroup::generateClassCode(basedir + "/exData.h", rs);

    fname = basedir + "/_test.cpp";
    errname = basedir + "/_test.err";
  }
  else
  {
    String dir(page_sys_hlp->getSgCppDestDir());

    fname.printf(260, "%s/exData.h", dir.str());
    tmp_fname.printf(250, "%s/exData.1.h", dir.str());

    AsgStateGenParamGroup::generateClassCode(tmp_fname, rs);
    if (!dag_file_compare(fname, tmp_fname))
    {
      unlink(fname);
      dag_copy_file(tmp_fname, fname, true);
    }
    unlink(tmp_fname);

    dd_mkdir(dir);
    fname.printf(260, "%s/%s_statesGraph.cpp", dir.str(), res_name);
    tmp_fname.printf(250, "%s/%s_statesGraph.1.cpp", dir.str(), res_name);
  }
  FILE *fp = fopen(tmp_fname.empty() ? fname : tmp_fname, "wt");
  const char *graph_file = "hero.pers.blk";

  String graph_file_id_s(str_to_valid_id(graph_file));

  if (!compileRes)
    graph_file_id_s.printf(260, "%s_statesGraph", res_name);

  const char *graph_file_id = graph_file_id_s;

  if (!fp)
  {
    debug_ctx("error");
    return false;
  }

  // generic anim graph class
  fprintf(fp, ""
              "#include <animChar/animBranches.h>\n"
              ""
              "#include <animChar/animITranslator.h>\n"
              ""
              "#include <math/dag_Point3.h>\n"
              ""
              "#include <string.h>\n");

  if (compileRes)
    fprintf(fp, ""
                "#include <common.h>\n"
                ""
                "#include \"exData.h\"\n\n"
                ""
                "typedef int intptr_t;\n\n");
  else
    fprintf(fp, ""
                "#include <debug/dag_debug.h>\n");

  // write prolog
  fprintf(fp, ""
              "using namespace AnimV20;\n\n");

  if (!compileRes)
    fprintf(fp, ""
                "extern ITranslator *anim_iface_trans;\n\n");

  fprintf(fp,
    ""
    "namespace %s {\n",
    graph_file_id);

  if (compileRes)
    fprintf(fp, ""
                "  static ITranslator *trans = NULL;\n\n");
  else
    fprintf(fp, ""
                "  GenericAnimStatesGraph* make_graph();\n\n");

  fprintf(fp, ""
              "  class Graph;\n\n");

  if (debug_lev > 0)
    fprintf(fp, ""
                "  class GraphDebug;\n\n");

  implement_declareStaticVars(fp);
  for (int i = 0; i < states.size(); i++)
    if (rs)
      rs->generateStaticVar(fp, i, *states[i]->props);

  fprintf(fp, "\n\n");

  // building list of used anim nodes and mapping state->nodeid
  SmallTab<int, MidmemAlloc> nodeMap;
  NameMap nodeNames;
  clear_and_resize(nodeMap, states.size() * chan.size());
  for (int i = 0; i < states.size(); i++)
    for (int j = 0; j < chan.size(); j++)
    {
      if (states[i]->animNode[j].length() < 1)
      {
        nodeMap[i * chan.size() + j] = -1;
        continue;
      }
      if (!checkAnimNodeExists(states[i]->animNode[j]))
      {
        debug_ctx("node <%s> not found; skipping", (char *)states[i]->animNode[j]);
        nodeMap[i * chan.size() + j] = -1;
        continue;
      }
      nodeMap[i * chan.size() + j] = nodeNames.addNameId(states[i]->animNode[j]);
    }

  if (nodeNames.nameCount())
  {
    fprintf(fp,
      ""
      "  static const int ANIMNODES_NUM = %d;\n"
      ""
      "  static const char *animnode_names[] = {\n",
      nodeNames.nameCount());

    for (int i = 0; i < nodeNames.nameCount(); i++)
      fprintf(fp,
        ""
        "    \"%s\",\n",
        nodeNames.getName(i));

    fprintf(fp, ""
                "  };\n\n");
  }

  fprintf(fp, ""
              "}\n\n"
              ""
              "using namespace AnimV20;\n\n");

  if (debug_lev > 0)
    fprintf(fp,
      ""
      "class %s::GraphDebug: public IAnimStateDirector2Debug {\n"
      ""
      "public:\n"
      ""
      "  virtual int getGraphThreadsCount() { return %d; }\n\n"
      ""
      "  virtual int getStatesCount() { return %d; }\n"
      ""
      "  virtual int getClassesCount() { return %d; }\n"
      ""
      "  virtual int getFlagsCount() { return %d; }\n"
      ""
      "  virtual int getIrqsCount() { return %d; }\n"
      ""
      "  virtual int getExPropsClassesCount() { return 0; }\n\n"
      ""
      "  virtual const char *getStateName(int state_id) { return %s; }\n"
      ""
      "  virtual const char *getClassName(int class_id) { return %s; }\n"
      ""
      "  virtual const char *getFlagName(int flag_id) { return %s; }\n"
      ""
      "  virtual const char *getIrqName(int irq_id) { return %s; }\n"
      ""
      "  virtual const char *getExPropName(int exp_prop_id) { return NULL; }\n"
      ""
      "};\n\n"
      ""
      "namespace %s { static GraphDebug graph_dbg; }\n\n",
      graph_file_id, (unsigned)gt.size(), (unsigned)states.size(), classList.nameCount(), flagList.nameCount(), irqList.nameCount(),
      states.size() ? "state_names[state_id]" : "NULL", classList.nameCount() ? "class_names[class_id]" : "NULL",
      flagList.nameCount() ? "flag_names[flag_id]" : "NULL", irqList.nameCount() ? "irq_names[irq_id]" : "NULL", graph_file_id);

  fprintf(fp,
    ""
    "class %s::Graph: public GenericAnimStatesGraph {\n"
    ""
    "  AnimationGraph *anim;\n",
    graph_file_id);
  if (nodeNames.nameCount())
    fprintf(fp, ""
                "  IAnimBlendNode *animNode[ANIMNODES_NUM];\n");
  fprintf(fp,
    ""
    "  AnimBlendCtrl_Fifo3 *fifo3[%d];\n"
    ""
    "  IGenericIrq *irqHandler;\n"
    ""
    "  int generation;\n"
    ""
    "  // graph thread context\n"
    ""
    "  struct GraphThreadContext {\n"
    ""
    "    void (Graph::*checkAlways)();\n"
    ""
    "    void (Graph::*checkAtEnd)();\n"
    ""
    "    void (Graph::*atLeave)();\n"
    ""
    "    bool (Graph::*checkClass)(int class_id);\n"
    ""
    "    IAnimBlendNode *curNode[%d];\n"
    ""
    "    int curState;\n"
    ""
    "    Point3 curOriginVel;\n"
    ""
    "  };\n"
    ""
    "  GraphThreadContext *gt;\n\n"
    ""
    "  static void (Graph::*s_start[STATES_NUM])(bool,real);\n\n"
    ""
    "  static bool (Graph::*s_checkClass[STATES_NUM])(int class_id);\n\n"
    ""
    "  static bool (Graph::*s_getStateExProps[STATES_NUM])(int ex_props_cls, void **ex_props);\n"
    ""
    "public:\n",
    (unsigned)chan.size(), (unsigned)chan.size());


  // gather full used variables list
  AsgVarsList vars;
  AsgConditionStringParser csp;

  if (flagList.nameCount() > 0)
    vars.registerVar("graphControlFlags", 'B');
  for (int i = 0; i < gt.size(); i++)
  {
    vars.registerVar(String(62, "lastStateTime%d", i), 'S');
    vars.registerVar(String(62, "curStateTime%d", i), 'T');
  }

  // first, gather variables from conditions
  for (int i = 0; i < states.size(); i++)
  {
    {
      Tab<AnimGraphCondition *> &cond = states[i]->enterCond;
      for (int j = 0; j < cond.size(); j++)
      {
        const char *c = cond[j]->condition;
        int id = usedCondList.getNameId(c);
        c = id != -1 ? codeForCond[id].c_str() : "false /*???*/";

        csp.clear();
        csp.parse(c);
        for (int k = 0; k < csp.vars.size(); k++)
          if (csp.vars[k].type == 'O')
            vars.registerVar("graphPrevControlFlags", 'O');
        for (int k = 0; k < csp.vars.size(); k++)
          if (!vars.registerVar(csp.vars[k].name, csp.vars[k].type))
          {
            fclose(fp);
            unlink(fname);
            if (!tmp_fname.empty())
              unlink(tmp_fname);
            debug_ctx("cannot  register var name <%s> of type=%c", (char *)csp.vars[k].name, csp.vars[k].type);
            return false;
          }
      }
    }
    {
      Tab<AnimGraphCondition *> &cond = states[i]->endCond;
      for (int j = 0; j < cond.size(); j++)
      {
        const char *c = cond[j]->condition;
        int id = usedCondList.getNameId(c);
        c = id != -1 ? codeForCond[id].c_str() : "false /*???*/";

        csp.clear();
        csp.parse(c);
        for (int k = 0; k < csp.vars.size(); k++)
          if (csp.vars[k].type == 'O')
            vars.registerVar("graphPrevControlFlags", 'O');
        for (int k = 0; k < csp.vars.size(); k++)
          if (!vars.registerVar(csp.vars[k].name, csp.vars[k].type))
          {
            fclose(fp);
            unlink(fname);
            if (!tmp_fname.empty())
              unlink(tmp_fname);
            debug_ctx("cannot  register var name <%s> of type=%c", (char *)csp.vars[k].name, csp.vars[k].type);
            return false;
          }
      }
    }
    {
      Tab<AnimGraphCondition *> &cond = states[i]->whileCond;
      for (int j = 0; j < cond.size(); j++)
      {
        const char *c = cond[j]->condition;
        int id = usedCondList.getNameId(c);
        c = id != -1 ? codeForCond[id].c_str() : "false /*???*/";

        csp.clear();
        csp.parse(c);
        for (int k = 0; k < csp.vars.size(); k++)
          if (csp.vars[k].type == 'O')
            vars.registerVar("graphPrevControlFlags", 'O');
        for (int k = 0; k < csp.vars.size(); k++)
          if (!vars.registerVar(csp.vars[k].name, csp.vars[k].type))
          {
            fclose(fp);
            unlink(fname);
            if (!tmp_fname.empty())
              unlink(tmp_fname);
            debug_ctx("cannot  register var name <%s> of type=%c", csp.vars[k].name.str(), csp.vars[k].type);
            return false;
          }
      }
    }
  }

  // next, gather variables from actions
  for (int i = 0; i < states.size(); i++)
    for (int j = 0; j < states[i]->actions.size(); j++)
      for (int k = 0; k < states[i]->actions[j]->actions.size(); k++)
      {
        AnimGraphTimedAction::Action &a = *states[i]->actions[j]->actions[k];
        if (a.type == AnimGraphTimedAction::Action::AGA_SetParam)
        {
          csp.clear();
          csp.parse(a.subject);
          for (int k = 0; k < csp.vars.size(); k++)
            if (!vars.registerVar(csp.vars[k].name, csp.vars[k].type))
            {
              fclose(fp);
              unlink(fname);
              if (!tmp_fname.empty())
                unlink(tmp_fname);
              debug_ctx("cannot  register var name <%s> of type=%c", csp.vars[k].name.str(), csp.vars[k].type);
              return false;
            }

          csp.clear();
          csp.parse(a.expression);
          for (int k = 0; k < csp.vars.size(); k++)
            if (!vars.registerVar(csp.vars[k].name, csp.vars[k].type))
            {
              fclose(fp);
              unlink(fname);
              if (!tmp_fname.empty())
                unlink(tmp_fname);
              debug_ctx("cannot  register var name <%s> of type=%c", csp.vars[k].name.str(), csp.vars[k].type);
              return false;
            }
        }
      }

  // write code to implement paramId declaration
  csp.clear();
  vars.implement_ParamIdDeclaration(fp);
  fprintf(fp, ""
              "  int paramId_gt;\n\n");

  implement_flagsDeclaration(fp);

  fprintf(fp, ""
              "  Graph() {\n"
              ""
              "    int i, j;\n\n"
              ""
              "    irqHandler = NULL;\n"
              ""
              "    anim = NULL;\n"
              ""
              "    memset(fifo3, 0, sizeof(fifo3));\n"
              ""
              "    gt = NULL;\n"
              ""
              "    generation = 0;\n"
              ""
              "  }\n\n");

  // implement a number of functions for each state
  for (int i = 0; i < states.size(); i++)
  {
    int gtIndex = states[i]->gt;

    fprintf(fp,
      ""
      "  // State %d: %s\n"
      ""
      "  void state%02d_start(bool customMorph, real customMorphTime) {\n",
      i, (char *)states[i]->name, i);
    INSERT_THOROUGH_LOG_2("state #%d (%s) start", i, states[i]->name.str());
    fprintf(fp,
      ""
      "    if (gt[%d].atLeave)\n"
      ""
      "      (this->*gt[%d].atLeave)();\n",
      gtIndex, gtIndex);
    fprintf(fp,
      ""
      "    st->setParam(paramId.curStateTime%d, 0);\n"
      ""
      "    st->setParam(paramId.lastStateTime%d, 0);\n"
      ""
      "    gt[%d].checkAlways = &Graph::state%02d_checkAlways;\n"
      ""
      "    gt[%d].checkAtEnd = &Graph::state%02d_checkAtEnd;\n"
      ""
      "    gt[%d].atLeave = &Graph::state%02d_leave;\n"
      ""
      "    gt[%d].checkClass = &Graph::state%02d_checkClass;\n"
      ""
      "    gt[%d].curState = %d;\n"
      ""
      "    generation ++;\n",
      gtIndex, gtIndex, gtIndex, i, gtIndex, i, gtIndex, i, gtIndex, i, gtIndex, i);

    if (states[i]->addVel.vel)
    {
      real dirh = states[i]->addVel.dirh * PI / 180;
      real dirv = states[i]->addVel.dirv * PI / 180;
      double sp = sin(dirh), cp = cos(dirh), st = sin(dirv), ct = cos(dirv);
      Point3 v = states[i]->addVel.vel * Point3(cp * ct, st, sp * ct);

      fprintf(fp,
        ""
        "    gt[%d].curOriginVel = Point3(%.7f,%.7f,%.7f);\n",
        gtIndex, v.x, v.y, v.z);
    }
    else
      fprintf(fp,
        ""
        "    memset(&gt[%d].curOriginVel, 0, sizeof(Point3));\n",
        gtIndex);

    INSERT_THOROUGH_LOG("setting anim");

    /*
      fprintf(fp,
    """    cdebug(\"%%p GT %d: set state %d (%s)\", this);\n"
        ,
        gtIndex, i, (char*)states[i]->name
      );
    */

    for (int j = 0; j < chan.size(); j++)
      if (nodeMap[i * chan.size() + j] != -1)
      {
        if (states[i]->resumeAnim)
          fprintf(fp,
            ""
            "    %s->resume(animNode[%d], *st, true);\n",
            translator, nodeMap[i * chan.size() + j]);

        fprintf(fp,
          ""
          "    %s->enqueueState(fifo3[%d], *st, animNode[%d], customMorph?customMorphTime:%.3f, 0);\n"
          ""
          "    gt[%d].curNode[%d] = animNode[%d];\n",
          translator, j, nodeMap[i * chan.size() + j], chan[j].defMorphTime, gtIndex, j, nodeMap[i * chan.size() + j]);

        if (debug_lev > 1)
          fprintf(fp,
            ""
            "    gen_irq(-3, %d, animNode[%d], fifo3[%d]);\n",
            gtIndex, nodeMap[i * chan.size() + j], j);
        //--fprintf(fp, "    cdebug(\"state %d: set node=%%p\", animNode[%d]);\n", i, nodeMap[i*chan.size()+j]);
      } //==else fprintf(fp, "    gt[%d].curNode[%d] = NULL\n",);


    if (debug_lev > 1)
      fprintf(fp,
        ""
        "    gen_irq(-1, %d, NULL, NULL);\n",
        i);

    INSERT_THOROUGH_LOG("perform actions");

    int anim_node_id = get_anim_node_id(i, &nodeMap[0]);
    {
      AsgLocalVarsList locals;
      implement_actionsCheckAndPerform(fp, i, vars, locals, 0, anim_node_id);
    }
    INSERT_THOROUGH_LOG("--- done");

    fprintf(fp, ""
                "  }\n");

    implement_canEnter(fp, i, vars);
    implement_checkAlways(fp, i, vars, anim_node_id);
    implement_checkAtEnd(fp, i, vars, anim_node_id);
    implement_leave(fp, i, vars, anim_node_id);
    implement_go_checked(fp, i, vars);
    implement_checkClass(fp, i, vars);
    implement_getStateExProps(fp, i, rs);
  }

  // add general interface functions
  fprintf(fp, "\n");
  fprintf(fp,
    ""
    "  // GenericAnimStatesGraph interface implementation\n"
    ""
    "  virtual bool init (AnimationGraph *_anim, IPureAnimStateHolder *_st) {\n"
    ""
    "    anim = _anim;\n"
    ""
    "    st = _st;\n"
    ""
    "    paramId_gt = %s->addInlinePtrParamId(anim, \"graph_thread_context\",\n"
    ""
    "      sizeof(GraphThreadContext)*%d);\n",
    translator, (unsigned)gt.size());

  for (int i = 0; i < chan.size(); i++)
    fprintf(fp,
      ""
      "    fifo3[%d] = (AnimBlendCtrl_Fifo3*)%s->getBlendNodePtr(anim, \"%s\");\n"
      ""
      "    if (!%s->checkFifo3 ((IAnimBlendNode*)fifo3[%d])) {\n"
      ""
      "      cdebug(\"can't find fifo3 ctrl: <%s>\");\n"
      ""
      "      return false;\n"
      ""
      "    }\n",
      i, translator, chan[i].fifoCtrl.str(), translator, i, chan[i].fifoCtrl.str());
  vars.implement_ParamIdInit(fp, translator);

  if (nodeNames.nameCount())
    fprintf(fp,
      ""
      "    for (int i = 0; i < ANIMNODES_NUM; i ++) {\n"
      ""
      "      animNode[i] = %s->getBlendNodePtr(anim, animnode_names[i]);\n"
      ""
      "      if (!animNode[i]) {\n"
      ""
      "        cdebug(\"can't find blendnode: <%%s>\", animnode_names[i]);\n"
      ""
      "        return false;\n"
      ""
      "      }\n"
      ""
      "    }\n\n",
      translator);
  fprintf(fp, ""
              "    return true;\n"
              ""
              "  }\n\n"
              ""
              "  virtual void advance() {\n");

  INSERT_THOROUGH_LOG("advance");
  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    (this->*gt[%d].checkAlways)();\n",
      i);

  for (int i = 0; i < gt.size(); i++)
  {
    fprintf(fp,
      ""
      "    st->setParam(paramId.lastStateTime%d, st->getParam(paramId.curStateTime%d));\n",
      i, i);
    if (gtMaskForMissingAnimWithAbsPos & (1 << i))
      fprintf(fp,
        ""
        "    st->setParam(paramId.curStateTime%d, st->getParam(paramId.curStateTime%d) + st->getParam(1/*PID_GLOBAL_LAST_DT*/));\n",
        i, i);
  }
  if (vars.findVar("graphControlFlags", 'B') != -1 && vars.findVar("graphPrevControlFlags", 'O') != -1)
    fprintf(fp, ""
                "    st->setParamInt(paramId.graphPrevControlFlags, "
                "st->getParamInt(paramId.graphControlFlags));\n");

  fprintf(fp, ""
              "  }\n"
              ""
              "  virtual void atIrq (int irq_type, IAnimBlendNode *src) {\n");
  INSERT_THOROUGH_LOG("atIrq");
  if (debug_lev > 1)
    fprintf(fp, ""
                "    gen_irq(-2, irq_type, src, NULL);\n");

  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    if (endRelatedAnim(%d, src)) {\n"
      ""
      "      (this->*gt[%d].checkAtEnd)();\n"
      ""
      "      return;\n"
      ""
      "    }\n",
      i, i /*--, i*/
    );
  fprintf(fp, ""
              "  }\n\n"
              ""
              "  bool endRelatedAnim(int gti, IAnimBlendNode *src) {\n");
  for (int i = 0; i < chan.size(); i++)
    fprintf(fp,
      ""
      "    if (gt[gti].curNode[%d] && %s->isAliasOf(gt[gti].curNode[%d], *st, src)) {\n"
      ""
      "      gt[gti].curNode[%d] = NULL;\n"
      ""
      "      return true;\n"
      ""
      "    }\n",
      i, translator, i, i);
  fprintf(fp, ""
              "    return false;\n"
              ""
              "  }\n"
              ""
              "  void start_threads() {\n");
  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    state%02d_start (false, 0);\n",
      findState(gt[i].startNodeName));
  fprintf(fp, ""
              "  }\n"
              ""
              "  void gen_irq(int id, int state_id, IAnimBlendNode *n, void *props) {\n");
  INSERT_THOROUGH_LOG_T2("gen irq %%d", "id");
  fprintf(fp, ""
              "    if (irqHandler) irqHandler->irq(id, state_id, (intptr_t)n, (intptr_t)props);\n"
              ""
              "  }\n\n");


  fprintf(fp,
    // destroy & clone
    ""
    "  // IAnimStateDirector2 interface implementation\n\n"
    ""
    "  // destroy & clone\n"
    ""
    "  virtual void destroy() { delete this; }\n"
    ""
    "  virtual IAnimStateDirector2 *clone(IPureAnimStateHolder *new_st) {\n"
    ""
    "    Graph *g = new Graph(*this);\n"
    ""
    "    g->st = new_st;\n"
    ""
    "    return g;\n"
    ""
    "  }\n\n"
    // name->id resolution
    ""
    "  // name->id resolution interface\n"
    ""
    "  virtual int getStateId(const char *state_name) {\n"
    ""
    "    for (int i = 0; i < STATES_NUM; i ++)\n"
    ""
    "      if (strcmp(state_names[i], state_name) == 0)\n"
    ""
    "        return i;\n"
    ""
    "    return -1;\n"
    ""
    "  }\n");

  if (flagList.nameCount() > 0)
    fprintf(fp, ""
                "  virtual int getFlagId(const char *flag_name) {\n"
                ""
                "    for (int i = 0; i < FLAGS_NUM; i ++)\n"
                ""
                "      if (strcmp(flag_names[i], flag_name) == 0)\n"
                ""
                "        return i;\n"
                ""
                "    return -1;\n"
                ""
                "  }\n");
  else
    fprintf(fp, ""
                "  virtual int getFlagId(const char *flag_name) { return -1; }\n");

  if (classList.nameCount() > 0)
    fprintf(fp, ""
                "  virtual int getStateClassId(const char *class_name) {\n"
                ""
                "    for (int i = 0; i < CLASSES_NUM; i ++)\n"
                ""
                "      if (strcmp(class_names[i], class_name) == 0)\n"
                ""
                "        return i;\n"
                ""
                "    return -1;\n"
                ""
                "  }\n");
  else
    fprintf(fp, ""
                "  virtual int getStateClassId(const char *class_name) { return -1; }\n");

  fprintf(fp,
    ""
    "  virtual int getNamedRangeId(const char *range_name) {\n"
    ""
    "    return %s->getNamedRangeId(anim, range_name);\n"
    ""
    "  }\n"
    ""
    "  virtual int getExPropsClassId(const char *class_name) { return -1; }\n",
    translator);

  if (irqList.nameCount() > 0)
    fprintf(fp, ""
                "  virtual int getIrqId(const char *irq_name) {\n"
                ""
                "    for (int i = 0; i < IRQS_NUM; i ++)\n"
                ""
                "      if (strcmp(irq_names[i], irq_name) == 0)\n"
                ""
                "        return i;\n"
                ""
                "    return -1;\n"
                ""
                "  }\n");
  else
    fprintf(fp, ""
                "  virtual int getIrqId(const char *irq_name) { return -1; }\n");

  fprintf(fp,
    ""
    "  virtual void setIrqHandler(IGenericIrq *irq) { irqHandler = irq; }\n\n"
    // graph flow control
    ""
    "  // direct and indirect state setup interface\n"
    ""
    "  virtual void reset(bool set_def_state) {\n"
    ""
    "    int i;\n"
    ""
    "    gt = (GraphThreadContext*) st->getInlinePtr(paramId_gt);\n"
    ""
    "    for (i = 0; i < %d; i ++)\n"
    ""
    "      %s->resetQueue(fifo3[i], *st, false);\n"
    ""
    "    memset(gt, 0, %d*sizeof(GraphThreadContext));\n"
    ""
    "    for (i = 0; i < %d; i ++)\n"
    ""
    "      gt[i].curState = -1;\n"
    ""
    "    if (set_def_state)\n"
    ""
    "      start_threads();\n"
    ""
    "  }\n\n"
    ""
    "  virtual void forceState(int state_id) {\n"
    ""
    "    if (state_id < 0 || state_id >= STATES_NUM) return;\n"
    ""
    "    (this->*s_start[state_id]) (false, %g);\n"
    ""
    "  }\n",
    (unsigned)chan.size(), translator, (unsigned)gt.size(), (unsigned)gt.size(), forcedStateMorphTime);

  // load/save
  fprintf(fp,
    ""
    "  struct SaveData {\n"
    ""
    "    int state_id; float curStateTime; float lastStateTime;\n"
    ""
    "  };\n"
    ""
    "  virtual int save(void **ptr)\n"
    ""
    "  {\n"
    ""
    "    SaveData *data = new SaveData[%d];\n"
    ""
    "    for (int i = 0; i < %d; i ++)\n"
    ""
    "    {\n"
    ""
    "      cdebug(\" state[%%d]=%%d \", i, gt[i].curState);\n"
    ""
    "      data[i].state_id = gt[i].curState;\n"
    ""
    "    }\n",
    (unsigned)gt.size(), (unsigned)gt.size());
  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    data[%d].curStateTime = st->getParam(paramId.curStateTime%d);\n"
      ""
      "    data[%d].lastStateTime = st->getParam(paramId.lastStateTime%d);\n",
      i, i, i, i);
  fprintf(fp,
    ""
    "    *ptr = (void *)data;\n"
    ""
    "    return %d * sizeof(SaveData);\n"
    ""
    "  }\n"
    ""
    "  virtual void load(void *ptr)\n"
    ""
    "  {\n"
    ""
    "    SaveData *data = (SaveData *)ptr;\n"
    ""
    "    cdebug(\" loading %d threads \");\n",
    (unsigned)gt.size(), (unsigned)gt.size());

  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    cdebug(\" state[%d]=%%d this=%%p \",  data[%d].state_id, this);\n"
      ""
      "    gt[%d].atLeave = NULL;\n"
      ""
      "    (this->*s_start[data[%d].state_id]) (false, 0);\n"
      ""
      "    st->setParam(paramId.curStateTime%d, data[%d].curStateTime);\n"
      ""
      "    st->setParam(paramId.lastStateTime%d, data[%d].lastStateTime);\n",
      i, i, i, i, i, i, i, i);
  fprintf(fp, ""
              "  }\n");


  if (vars.findVar("graphControlFlags", 'B') == -1)
    fprintf(fp, ""
                "  virtual void setFlag(int flag_id) {}\n"
                ""
                "  virtual void clrFlag(int flag_id) {}\n"
                ""
                "  virtual int  getFlag(int flag_id) {return 0;}\n\n");
  else
  {
    fprintf(fp, ""
                "  virtual void setFlag(int flag_id) {\n"
                ""
                "    if (flag_id < 0) return;\n"
                ""
                "    st->setParamInt(paramId.graphControlFlags,\n"
                ""
                "      st->getParamInt(paramId.graphControlFlags) | (1<<flag_id));\n");
    INSERT_THOROUGH_LOG_T2("set flag %%d", "flag_id");
    fprintf(fp, ""
                "  }\n"
                ""
                "  virtual void clrFlag(int flag_id) {\n"
                ""
                "    if (flag_id < 0) return;\n"
                ""
                "    st->setParamInt(paramId.graphControlFlags,\n"
                ""
                "      st->getParamInt(paramId.graphControlFlags) & ~(1<<flag_id));\n");
    INSERT_THOROUGH_LOG_T2("clr flag %%d", "flag_id");
    fprintf(fp, ""
                "  }\n"
                ""
                "  virtual int  getFlag(int flag_id) {\n"
                ""
                "    if (flag_id < 0) return 0;\n"
                ""
                "    return st->getParamInt(paramId.graphControlFlags) & (1<<flag_id);\n"
                ""
                "  }\n\n");
  }

  if (vars.findVar("graphControlFlags", 'B') == -1)
    fprintf(fp, ""
                "  virtual void setFlags(int f) {}\n"
                ""
                "  virtual int  getFlags() {return 0;}\n\n");
  else
    fprintf(fp, ""
                "  virtual void setFlags(int f) {\n"
                ""
                "    st->setParamInt(paramId.graphControlFlags, f);\n"
                ""
                "  }\n"
                ""
                "  virtual int  getFlags() {\n"
                ""
                "    return st->getParamInt(paramId.graphControlFlags) ;\n"
                ""
                "  }\n\n");

  // current state properties
  fprintf(fp, ""
              "  // current state query interface\n"
              ""
              "  virtual int getCurState() { return gt[0].curState; }\n"
              ""
              "  virtual int getCurState(int gti) { return gt[gti].curState; }\n"
              ""
              "  virtual bool checkCurStateClass(int class_id) {\n");
  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    if ((this->*gt[%d].checkClass)(class_id)) return true;\n",
      i);

  fprintf(fp, ""
              "    return false;\n"
              ""
              "  }\n"
              ""
              "  virtual bool checkNamedRange(int range_id) { return false; }\n"
              ""
              "  virtual void getCurStateExProps(int ex_props_class, void **ex_props) {\n");
  for (int i = 0; i < gt.size(); i++)
    fprintf(fp,
      ""
      "    if ((this->*s_getStateExProps[gt[%d].curState])(ex_props_class, ex_props))\n"
      ""
      "      return;\n",
      i);

  fprintf(fp,
    ""
    "    *ex_props = NULL;\n"
    ""
    "  }\n"
    ""
    "  virtual void getCurStateExProps(int ex_props_class, void **ex_props, int gt_id) {\n"
    ""
    "    if (!(this->*s_getStateExProps[gt[gt_id].curState])(ex_props_class, ex_props))\n"
    ""
    "      *ex_props = NULL;\n"
    ""
    "  }\n\n"
    ""
    "  virtual int getStateGeneration() { return generation; }\n"
    ""
    "  virtual void getAddOriginVel(Point3 &p) { p = gt[0].curOriginVel; }\n\n"
    // graph info interface
    ""
    "  // arbitrary state data query interface\n"
    ""
    "  virtual int getStatesCount() { return STATES_NUM; }\n"
    ""
    "  virtual bool checkStateClass(int state_id, int class_id) {\n"
    ""
    "    if (state_id < 0 || state_id >= STATES_NUM || class_id < 0) return false;\n"
    ""
    "    return (this->*s_checkClass[state_id]) (class_id);\n"
    ""
    "  }\n"
    ""
    "  virtual void getCurStateExProps(int state_id, int ex_props_class, void **ex_props) {\n" /*==NAME*/
    ""
    "    if (state_id < 0 || state_id >= STATES_NUM) {\n"
    ""
    "      *ex_props = NULL;\n"
    ""
    "      return;\n"
    ""
    "    }\n"
    ""
    "    if (!(this->*s_getStateExProps[state_id])(ex_props_class, ex_props))\n"
    ""
    "      *ex_props = NULL;\n"
    ""
    "  }\n"
    ""
    "  virtual IAnimStateDirector2Debug *getDebug() { return %s; }\n",
    debug_lev ? "&graph_dbg" : "NULL");

  // end class and define static members
  fprintf(fp,
    ""
    "};\n\n"
    ""
    "void (%s::Graph::*%s::Graph::s_start[%s::STATES_NUM])(bool,real) = {\n",
    graph_file_id, graph_file_id, graph_file_id);
  for (int i = 0; i < states.size(); i++)
    fprintf(fp,
      ""
      "  &%s::Graph::state%02d_start,\n",
      graph_file_id, i);
  fprintf(fp,
    ""
    "};\n\n"
    ""
    "bool (%s::Graph::*%s::Graph::s_checkClass[%s::STATES_NUM])(int class_id) = {\n",
    graph_file_id, graph_file_id, graph_file_id);
  for (int i = 0; i < states.size(); i++)
    fprintf(fp,
      ""
      "  &%s::Graph::state%02d_checkClass,\n",
      graph_file_id, i);
  fprintf(fp,
    ""
    "};\n\n"
    ""
    "bool (%s::Graph::*%s::Graph::s_getStateExProps[%s::STATES_NUM])"
    "(int ex_props_class, void **ex_props) = {\n",
    graph_file_id, graph_file_id, graph_file_id);
  for (int i = 0; i < states.size(); i++)
    fprintf(fp,
      ""
      "  &%s::Graph::state%02d_getStateExProps,\n",
      graph_file_id, i);
  fprintf(fp, ""
              "};\n\n");

  if (compileRes)
    fprintf(fp,
      // implement creator
      ""
      "extern \"C\" __declspec(dllexport) GenericAnimStatesGraph* __cdecl make_graph /*%s*/() {\n"
      ""
      "  return new %s::Graph;\n"
      ""
      "}\n"
      ""
      "extern \"C\" __declspec(dllexport) void __cdecl set_translator /*%s*/(ITranslator *tr) {\n"
      ""
      "  %s::trans = tr;\n"
      ""
      "}\n"
      ""
      "extern \"C\" __declspec(dllexport) bool __cdecl entry() { return true; }\n",
      graph_file_id, graph_file_id, graph_file_id, graph_file_id);
  else
    fprintf(fp,
      ""
      "GenericAnimStatesGraph* %s::make_graph() {\n"
      ""
      "  return new %s::Graph;\n"
      ""
      "}\n",
      graph_file_id, graph_file_id);
  fclose(fp);

  if (compileRes)
  {
    //== we don't compile to portableDLL since we dropped PDL loader code; consider using daScript
  }
  else
  {
    if (!dag_file_compare(fname, tmp_fname))
    {
      unlink(fname);
      dag_copy_file(tmp_fname, fname, true);
    }
    unlink(tmp_fname);
  }
  /*
    page_sys_hlp->postMsg(false, "Build succesfully",
      "Built states graph to portable dynamic library:\n%s\n\nsize: %d bytes", dest_fname, filesz);
  */
  return true;
}

void AsgStatesGraph::implement_canEnter(FILE *fp, int i, const AsgVarsList &vars)
{
  Tab<AnimGraphCondition *> &cond = states[i]->enterCond;
  AsgConditionStringParser csp;
  AsgLocalVarsList locals;

  fprintf(fp, "  bool state%02d_canEnter() {\n", i);

  if (cond.size() < 1)
    fprintf(fp, "    return true;\n");
  else
  {
    for (int j = 0; j < cond.size(); j++)
    {
      const char *c = cond[j]->condition;
      int targetId = findState(cond[j]->targetName);
      int id = usedCondList.getNameId(c);

      fprintf(fp, "%*s// %s\n", j * 2 + 4, "", c);
      if (targetId == -1)
      {
        fprintf(fp, "%*s// target state <%s> not found!\n", j * 2 + 4, "", cond[j]->targetName.str());
        continue;
      }

      c = id != -1 ? codeForCond[id].c_str() : "false /*???*/";

      csp.clear();
      csp.parse(c);
      for (int k = 0; k < csp.vars.size(); k++)
        locals.registerLocalName(csp.vars[k].name, csp.vars[k].type, fp, vars);

      fprintf(fp, "%*sif (", j * 2 + 4, "");
      for (int k = 0; k < csp.order.size(); k++)
        if (csp.order[k].var)
        {
          int idx = csp.order[k].idx;
          const char *localname = locals.getLocalName(csp.vars[idx].name, csp.vars[idx].type, vars);
          if (csp.vars[idx].type == AsgConditionStringParser::Var::TYPE_Bit)
            fprintf(fp, "(%s & FLG_%s)", localname, csp.vars[idx].flagName.str());
          else
            fprintf(fp, "%s", localname);
        }
        else
          fprintf(fp, "%s", csp.otherCode[csp.order[k].idx].str());

      fprintf(fp, ") {\n");
    }
    fprintf(fp, "%*sreturn true;\n", (unsigned)cond.size() * 2 + 4, "");

    for (int j = cond.size() - 1; j >= 0; j--)
      fprintf(fp, "%*s}\n", j * 2 + 4, "");

    fprintf(fp, "    return false;\n");
  }

  fprintf(fp, "  }\n");
}

void AsgStatesGraph::implement_checkAlways(FILE *fp, int i, const AsgVarsList &vars, int anim_node_id)
{
  AsgLocalVarsList locals;
  fprintf(fp, "  void state%02d_checkAlways() {\n", i);
  implement_actionsCheckAndPerform(fp, i, vars, locals, 1, anim_node_id);
  if (anim_node_id == -1)
    fprintf(fp, "    state%02d_checkAtEnd();\n", i);
  implement_checking(fp, i, states[i]->whileCond, vars, locals, false);
  fprintf(fp, "  }\n");
}

void AsgStatesGraph::implement_checkAtEnd(FILE *fp, int i, const AsgVarsList &vars, int anim_node_id)
{
  AsgLocalVarsList locals;
  fprintf(fp, "  void state%02d_checkAtEnd() {\n", i);
  implement_actionsCheckAndPerform(fp, i, vars, locals, 2, anim_node_id);
  implement_checking(fp, i, states[i]->endCond, vars, locals, false);
  fprintf(fp, "  }\n");
}

void AsgStatesGraph::implement_leave(FILE *fp, int i, const AsgVarsList &vars, int anim_node_id)
{
  AsgLocalVarsList locals;
  fprintf(fp, "  void state%02d_leave() {\n", i);
  implement_actionsCheckAndPerform(fp, i, vars, locals, 3, anim_node_id);
  fprintf(fp, "  }\n");
}

void AsgStatesGraph::implement_getStateExProps(FILE *fp, int i, AsgStateGenParamGroup *rs)
{
  AsgLocalVarsList locals;
  fprintf(fp, "  bool state%02d_getStateExProps (int ex_props_class, void **ex_props) {\n", i);
  if (rs)
    rs->generateFuncCode(fp, i, *states[i]->props);
  fprintf(fp, "    return false;\n");
  fprintf(fp, "  }\n");
}

void AsgStatesGraph::implement_go_checked(FILE *fp, int i, const AsgVarsList &vars)
{
  AsgLocalVarsList locals;
  Tab<AnimGraphCondition *> &cond = states[i]->enterCond;
  fprintf(fp, "  bool go_checked_state%02d() {\n", i);
  // check enter conditions
  fprintf(fp, "    if (!state%02d_canEnter()) return false;\n", i);
  // check all conditions
  implement_checking(fp, i, states[i]->whileCond, vars, locals, true);
  implement_checking(fp, i, states[i]->endCond, vars, locals, true);
  fprintf(fp, "    return false;\n");
  fprintf(fp, "  }\n");
}
void AsgStatesGraph::implement_checkClass(FILE *fp, int i, const AsgVarsList &vars)
{
  fprintf(fp, "  bool state%02d_checkClass(int class_id) {\n", i);
  int mask = 0, id;

  if (states[i]->classMarks.nameCount() > 0)
  {
    for (int j = 0; j < states[i]->classMarks.nameCount(); j++)
    {
      id = classList.getNameId(states[i]->classMarks.getName(j));
      if (id != -1)
        mask |= (1 << id);
      else
        debug_ctx("undefined class <%s> in state <%s>", states[i]->classMarks.getName(j), states[i]->name.str());
    }
  }

  if (mask)
  {
    fprintf(fp, "    if (class_id < 0) return false;\n");
    fprintf(fp, "    return 0x%08X & (1<<class_id);\n", mask);
  }
  else
    fprintf(fp, "    return false;\n");
  fprintf(fp, "  }\n");
}

void AsgStatesGraph::implement_checking(FILE *fp, int i, const Tab<AnimGraphCondition *> &cond, const AsgVarsList &vars,
  AsgLocalVarsList &locals, bool ret_bool)
{
  AsgConditionStringParser csp;

  for (int j = 0; j < cond.size(); j++)
  {
    const char *c = cond[j]->condition;
    int targetId = findState(cond[j]->targetName);
    int id = usedCondList.getNameId(c);

    if (targetId == -1)
    {
      fprintf(fp, "    // target state <%s> not found (condition #%d)!!!\n", cond[j]->targetName.str(), j);
      continue;
    }

    fprintf(fp, "    // %s -> state <%s>\n", c, (char *)cond[j]->targetName);

    c = id != -1 ? codeForCond[id].c_str() : "false /*???*/";

    if (strcmp(c, "%CHECKED%") == 0)
      fprintf(fp,
        "    if (go_checked_state%02d())\n"
        ""
        "      %s;\n",
        targetId, ret_bool ? "return true" : "return");
    else
    {
      csp.clear();
      csp.parse(c);
      for (int k = 0; k < csp.vars.size(); k++)
        locals.registerLocalName(csp.vars[k].name, csp.vars[k].type, fp, vars);

      fprintf(fp, "    if (state%02d_canEnter() && (", targetId);
      for (int k = 0; k < csp.order.size(); k++)
        if (csp.order[k].var)
        {
          int idx = csp.order[k].idx;
          const char *localname = locals.getLocalName(csp.vars[idx].name, csp.vars[idx].type, vars);

          if (csp.vars[idx].type == AsgConditionStringParser::Var::TYPE_Bit)
            fprintf(fp, "(%s & FLG_%s)", localname, (char *)csp.vars[idx].flagName);
          else
            fprintf(fp, "%s", localname);
        }
        else
          fprintf(fp, "%s", (char *)csp.otherCode[csp.order[k].idx]);

      fprintf(fp, ")) {\n");

      if (ret_bool)
        fprintf(fp,
          "      if (gt[%d].curState != %d) state%02d_start(%s, %f);\n"
          ""
          "      return true;\n",
          states[targetId]->gt, targetId, targetId, cond[j]->customMorph ? "true" : "false", cond[j]->customMorphTime);
      else
        fprintf(fp,
          "      state%02d_start(%s, %f);\n"
          "      return;\n",
          targetId, cond[j]->customMorph ? "true" : "false", cond[j]->customMorphTime);

      fprintf(fp, "    }\n");
    }
  }
}

void AsgStatesGraph::implement_flagsDeclaration(FILE *fp)
{
  if (flagList.nameCount())
  {
    fprintf(fp, "  enum {\n");
    for (int i = 0; i < flagList.nameCount(); i++)
      fprintf(fp, "    FLG_%s = (1<<%d),\n", flagList.getName(i), i);
    fprintf(fp, "  };\n\n");
  }
  if (classList.nameCount())
  {
    fprintf(fp, "  enum {\n");
    for (int i = 0; i < classList.nameCount(); i++)
      fprintf(fp, "    CLS_%s,\n", classList.getName(i));
    fprintf(fp, "  };\n\n");
  }
}

void AsgStatesGraph::implement_declareStaticVars(FILE *fp)
{
  fprintf(fp,
    "  enum {\n"
    "    FLAGS_NUM = %d,\n"
    "    CLASSES_NUM = %d,\n"
    "    STATES_NUM = %d,\n"
    "    IRQS_NUM = %d,\n"
    "  };\n\n",
    flagList.nameCount(), classList.nameCount(), (unsigned)states.size(), irqList.nameCount());

  if (flagList.nameCount())
  {
    fprintf(fp, "  static const char *flag_names[] = {\n");
    for (int i = 0; i < flagList.nameCount(); i++)
      fprintf(fp, "    \"%s\",\n", flagList.getName(i));
    fprintf(fp, "  };\n\n");
  }
  if (classList.nameCount())
  {
    fprintf(fp, "  static const char *class_names[] = {\n");
    for (int i = 0; i < classList.nameCount(); i++)
      fprintf(fp, "    \"%s\",\n", classList.getName(i));
    fprintf(fp, "  };\n\n");
  }
  if (states.size())
  {
    fprintf(fp, "  static const char *state_names[] = {\n");
    for (int i = 0; i < states.size(); i++)
      fprintf(fp, "    \"%s\",\n", (char *)states[i]->name);
    fprintf(fp, "  };\n\n");
  }
  if (irqList.nameCount())
  {
    fprintf(fp, "  static const char *irq_names[] = {\n");
    for (int i = 0; i < irqList.nameCount(); i++)
      fprintf(fp, "    \"%s\",\n", irqList.getName(i));
    fprintf(fp, "  };\n\n");
  }
}


void AsgStatesGraph::implement_actionsCheckAndPerform(FILE *fp, int i, const AsgVarsList &vars, AsgLocalVarsList &locals,
  int context /*0=start, 1=always, 2=end*/, int anim_node_id)
{
  AnimGraphState &s = *states[i];
  AsgLocalVarsList locals2;
  bool anim_time_added = false;
  int indent;

  for (int j = 0; j < s.actions.size(); j++)
  {
    AnimGraphTimedAction &a = *s.actions[j];

    // filter out unused conditions
    if (context == 0 && a.performAt != AnimGraphTimedAction::AGTA_AT_Start)
      continue;
    if (context == 2 && a.performAt != AnimGraphTimedAction::AGTA_AT_End)
      continue;
    if (context == 3 && a.performAt != AnimGraphTimedAction::AGTA_AT_Leave)
      continue;
    if (context == 1 && (a.performAt == AnimGraphTimedAction::AGTA_AT_Start || a.performAt == AnimGraphTimedAction::AGTA_AT_End ||
                          a.performAt == AnimGraphTimedAction::AGTA_AT_Leave))
      continue;

    if (a.performAt == AnimGraphTimedAction::AGTA_AT_Time && a.relativeTime && anim_node_id == -1)
    {
      debug_ctx("can't generate code for action %d, state: %s: relative time without animNode[0]", j, s.name.str());
      continue;
    }

    // generate branch to check action condition
    if (a.performAt == AnimGraphTimedAction::AGTA_AT_Time)
    {
      locals.registerLocalName(String(64, "lastStateTime%d", s.gt), 'S', fp, vars);
      bool first = locals.registerLocalName(String(64, "curStateTime%d", s.gt), 'T', fp, vars);
      if (s.animBasedActionTiming && anim_node_id == -1)
        gtMaskForMissingAnimWithAbsPos |= 1 << s.gt;
      else if (s.animBasedActionTiming && first && anim_node_id != -1)
      {
        fprintf(fp, "    t_curStateTime%d = %s->tell(animNode[%d], *st);\n", s.gt, translator, anim_node_id);
        fprintf(fp, "    st->setParam(paramId.curStateTime%d, t_curStateTime%d);\n", s.gt, s.gt);
      }

      if (a.relativeTime)
      {
        if (!anim_time_added)
        {
          fprintf(fp,
            "    float anim_dur = %s->getDuration(animNode[%d], *st),\n"
            "          s_lst_wrapped, t_cst_wrapped;\n",
            translator, anim_node_id);
          anim_time_added = true;
        }
        if (a.cyclic)
        {
          fprintf(fp,
            "\n    s_lst_wrapped = s_lastStateTime%d;"
            "\n    t_cst_wrapped = t_curStateTime%d;"
            "\n    if (anim_dur > 0) {"
            "\n      float wrap_dec = real2int(s_lst_wrapped/anim_dur-0.5)*anim_dur;"
            "\n      s_lst_wrapped -= wrap_dec;"
            "\n      t_cst_wrapped -= wrap_dec;"
            "\n    }"
            "\n    if (s_lst_wrapped <= %.7f*anim_dur &&"
            "\n         t_cst_wrapped >  %.7f*anim_dur) {\n",
            s.gt, s.gt, a.preciseTime, a.preciseTime);
        }
        else
          fprintf(fp,
            "\n    if (s_lastStateTime%d <= %.7f*anim_dur "
            "&& %.7f*anim_dur < t_curStateTime%d) {\n",
            s.gt, a.preciseTime, a.preciseTime, s.gt);
      }
      else
        fprintf(fp, "\n    if (s_lastStateTime%d <= %.7f && %.7f < t_curStateTime%d) {\n", s.gt, a.preciseTime, a.preciseTime, s.gt);
      locals2 = locals;
      indent = 6;
    }
    else
    {
      indent = 4;
      locals2 = locals;
    }

    // generate code for actions
    int id;
    for (int k = 0; k < a.actions.size(); k++)
      switch (a.actions[k]->type)
      {
        case AnimGraphTimedAction::Action::AGA_Nop: break;

        case AnimGraphTimedAction::Action::AGA_ClrFlag:
          id = flagList.getNameId(a.actions[k]->subject);
          fprintf(fp, "%*s// clr control flag <%s>\n", indent, "", a.actions[k]->subject.str());
          if (id != -1)
            fprintf(fp, "%*sGraph::clrFlag(%d);\n", indent, "", id);
          else
            fprintf(fp, "%*s//==err: unknown name\n", indent, "");
          break;

        case AnimGraphTimedAction::Action::AGA_SetFlag:
          id = flagList.getNameId(a.actions[k]->subject);
          fprintf(fp, "%*s// set control flag <%s>\n", indent, "", a.actions[k]->subject.str());
          if (id != -1)
            fprintf(fp, "%*sGraph::setFlag(%d);\n", indent, "", id);
          else
            fprintf(fp, "%*s//==err: unknown name\n", indent, "");
          break;

        case AnimGraphTimedAction::Action::AGA_GenIrq:
          id = irqList.getNameId(a.actions[k]->subject);
          fprintf(fp, "%*s// generate IRQ <%s>\n", indent, "", a.actions[k]->subject.str());
          if (id != -1)
          {
            if (a.actions[k]->useParams)
            {
              AsgConditionStringParser csp;
              csp.clear();
              csp.parse(a.actions[k]->p1);
              for (int l = 0; l < csp.vars.size(); l++)
                locals2.registerLocalName(csp.vars[l].name, csp.vars[l].type, fp, vars, indent);

              fprintf(fp, "%*sfloat irq_fp_%d_%d =", indent, "", j, k);
              // write expression
              for (int l = 0; l < csp.order.size(); l++)
                if (csp.order[l].var)
                {
                  int idx = csp.order[l].idx;
                  const char *localname = locals2.getLocalName(csp.vars[idx].name, csp.vars[idx].type, vars);

                  if (csp.vars[idx].type == AsgConditionStringParser::Var::TYPE_Bit)
                    fprintf(fp, "((%s & FLG_%s) != 0)", localname, (char *)csp.vars[idx].flagName);
                  else
                    fprintf(fp, "%s", localname);
                }
                else
                  fprintf(fp, "%s", (char *)csp.otherCode[csp.order[l].idx]);
              fprintf(fp, ";\n");
            }

            fprintf(fp, "%*sgen_irq(%d, %d, ", indent, "", id, i);
            if (anim_node_id == -1)
              fprintf(fp, "NULL");
            else
              fprintf(fp, "animNode[%d]", anim_node_id);

            if (a.actions[k]->useParams)
              fprintf(fp, ", &irq_fp_%d_%d);\n", j, k);
            else
              fprintf(fp, ", NULL);\n");
          }
          break;

        case AnimGraphTimedAction::Action::AGA_SetParam:
        {
          AsgConditionStringParser csp, csp_var;
          csp.clear();
          csp_var.parse(a.actions[k]->subject);
          csp.parse(a.actions[k]->expression);
          for (int l = 0; l < csp.vars.size(); l++)
            locals2.registerLocalName(csp.vars[l].name, csp.vars[l].type, fp, vars, indent);

          if (csp_var.vars.size() != 1)
            fprintf(fp, "%*s//==err: incorrect set  <%s>=<%s>\n", indent, "", a.actions[k]->subject.str(),
              a.actions[k]->expression.str());
          else
          {
            // prefix for expression
            switch (csp_var.vars[0].type)
            {
              case AsgConditionStringParser::Var::TYPE_Bit:
                id = flagList.getNameId(csp_var.vars[0].flagName);
                fprintf(fp, "%*sif (", indent, "");
                break;
              case AsgConditionStringParser::Var::TYPE_Scalar:
              case AsgConditionStringParser::Var::TYPE_Timer:
                fprintf(fp, "%*sst->setParam(paramId.%s, ", indent, "", csp_var.vars[0].name.str());
                break;
              case AsgConditionStringParser::Var::TYPE_Int:
                fprintf(fp, "%*sst->setParamInt(paramId.%s, ", indent, "", csp_var.vars[0].name.str());
                break;
              default: debug_ctx("not impl");
            }

            // write expression
            for (int l = 0; l < csp.order.size(); l++)
              if (csp.order[l].var)
              {
                int idx = csp.order[l].idx;
                const char *localname = locals2.getLocalName(csp.vars[idx].name, csp.vars[idx].type, vars);

                if (csp.vars[idx].type == AsgConditionStringParser::Var::TYPE_Bit)
                  fprintf(fp, "(%s & FLG_%s)", localname, csp.vars[idx].flagName.str());
                else
                  fprintf(fp, "%s", localname);
              }
              else
                fprintf(fp, "%s", (char *)csp.otherCode[csp.order[l].idx]);

            // suffix for expression
            switch (csp_var.vars[0].type)
            {
              case AsgConditionStringParser::Var::TYPE_Bit:
                id = flagList.getNameId(csp_var.vars[0].flagName);
                fprintf(fp,
                  ")\n"
                  "%*sGraph::setFlag(%d);\n"
                  "%*selse\n"
                  "%*sGraph::clrFlag(%d);\n",
                  indent + 2, "", id, indent, "", indent + 2, "", id);
                break;

              case AsgConditionStringParser::Var::TYPE_Scalar:
              case AsgConditionStringParser::Var::TYPE_Timer:
              case AsgConditionStringParser::Var::TYPE_Int: fprintf(fp, ");\n"); break;

              default: debug_ctx("not impl");
            }
          }
        }
        break;

        default: debug_ctx("unknown type %d", a.actions[k]->type);
      }

    // close branch
    if (a.performAt == AnimGraphTimedAction::AGTA_AT_Time)
      fprintf(fp, "    }\n");
    else
      locals = locals2;
  }
}
int AsgStatesGraph::get_anim_node_id(int i, int *nodeMap)
{
  AnimGraphState &s = *states[i];
  for (int j = 0; j < chan.size(); j++)
    if (nodeMap[i * chan.size() + j] != -1)
      return nodeMap[i * chan.size() + j];
  return -1;
}

bool AsgStatesGraph::checkAnimNodeExists(const char *name)
{
  if (tree.animNodeExists(name))
    return true;
  for (int j = 0; j < chan.size(); j++)
  {
    String nm(name);
    remove_trailing_string(nm, chan[j].defSuffix);
    if (tree.animNodeExists(nm))
      return true;
  }
  return false;
}

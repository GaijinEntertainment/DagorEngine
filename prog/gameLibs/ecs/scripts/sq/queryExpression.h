#pragma once

// this is correct PerformExpressionTree, supporting complex boolean expressions
struct PerformExpressionTree
{
  // char *literals = nullptr;//if we will have complex literals
  enum Operand
  {
    AND,
    OR,
    EQ,
    NE,
    LT,
    LE,
    OPT
  };
  struct Terminal
  {
    ecs::component_type_t type;
    uint32_t id; // component index
    Terminal() : type(0), id(0) {}
    Terminal(ecs::component_type_t tp, uint32_t id_) : type(tp), id(id_) {}
    template <class T>
    Terminal(const T &v) : type(ecs::ComponentTypeInfo<T>::type)
    {
      union
      {
        uint32_t id;
        char v[sizeof(T)];
      } val;
      val.id = 0;
      memcpy(val.v, &v, sizeof(T));
      id = val.id;
    }
    template <class T>
    T getLiteral() const
    {
      G_ASSERT(type == ecs::ComponentTypeInfo<T>::type);
      union
      {
        uint32_t id;
        char v[sizeof(T)];
      } val;
      val.id = id;
      return *(T *)val.v;
    }
  };

  enum
  {
    IS_TERMINAL = 1,
    IS_LITERAL = 2
  };
  struct TreeNode
  {
    int16_t left, right;
    uint8_t op;
    uint8_t leftFlags, rightFlags;
    Operand getOP() const { return Operand(op); }
  };
  struct Node
  {
    char storage[sizeof(TreeNode) > sizeof(Terminal) ? sizeof(TreeNode) : sizeof(Terminal)];
    Node(const TreeNode &tn) { memcpy(storage, &tn, sizeof(TreeNode)); }
    Node(const Terminal &tn) { memcpy(storage, &tn, sizeof(Terminal)); }
    Node() { memset(storage, 0, sizeof(storage)); }
    const Terminal &term() const { return *(Terminal *)(storage); }
    Terminal &term() { return *(Terminal *)(storage); }
    const TreeNode &fun() const { return *(TreeNode *)(storage); }
    TreeNode &fun() { return *(TreeNode *)(storage); }
  };

  Node *nodes = nullptr;
  const ecs::QueryView *qv = nullptr;
  int idInChunk = -1;
  int nodesCount = 0;

  PerformExpressionTree(Node *nodes_, int cnt, const ecs::QueryView *qv_) : nodes(nodes_), qv(qv_), nodesCount(cnt) {}

  template <class T>
  const T &getRef(uint16_t compId) const
  {
    G_ASSERTF(idInChunk >= qv->begin() && idInChunk < qv->end(), "idInChunk = %d>=%d && <%d ", idInChunk, qv->begin(), qv->end());
    auto ptr = ((typename ecs::PtrComponentType<T>::cptr_type)qv->getComponentUntypedData(compId));
    if (!ptr)
    {
      static T empty = T();
      return empty;
    }
    return ecs::PtrComponentType<T>::cref(ptr + idInChunk);
  }

  template <class T>
  const T &getOr(uint16_t compId, const T &def) const
  {
    G_ASSERTF(idInChunk >= qv->begin() && idInChunk < qv->end(), "idInChunk = %d>=%d && <%d ", idInChunk, qv->begin(), qv->end());
    auto ptr = ((typename ecs::PtrComponentType<T>::cptr_type)qv->getComponentUntypedData(compId));
    if (!ptr)
      return def;
    return ecs::PtrComponentType<T>::cref(ptr + idInChunk);
  }

  template <class T>
  static T getLiteral(const Terminal &a)
  {
    return a.getLiteral<T>();
  }
  Terminal getComp(const Terminal &a) const
  {
    switch (a.type)
    {
#define TYPED_GET(tp) \
  case ecs::ComponentTypeInfo<tp>::type: return Terminal(getRef<tp>(a.id));
      TYPED_GET(int)
      TYPED_GET(bool)
      TYPED_GET(float)
      TYPED_GET(ecs::EntityId)
#undef TYPED_GET
      default: return Terminal();
    }
  }
  bool performExpression(int id)
  {
    idInChunk = id;
    return getLiteral<bool>(performNodeExpression(0, nodesCount == 1 ? IS_TERMINAL : 0));
  }

  Terminal performNodeExpression(int node, int flags) const
  {
    G_ASSERT(node >= 0);
    if (!(flags & IS_TERMINAL))
      return performBinaryExpression(nodes[node].fun());
    return (flags & IS_LITERAL) ? nodes[node].term() : getComp(nodes[node].term());
  }
  Terminal performBinaryExpression(TreeNode fun) const
  {
    Operand op = fun.getOP();
    Terminal b = performNodeExpression(fun.right, fun.rightFlags);
    if (op == OPT)
    {
      if (fun.leftFlags != IS_TERMINAL)
        return b;
#define PERFORM_OPT(tp, val) \
  case ecs::ComponentTypeInfo<tp>::type: return Terminal(getOr<tp>(nodes[fun.left].term().id, getLiteral<tp>(b)));
      switch (b.type)
      {
        PERFORM_OPT(int, b)
        PERFORM_OPT(float, b)
        PERFORM_OPT(bool, b)
        PERFORM_OPT(ecs::EntityId, b)
        default: return b;
      }
#undef PERFORM_OPT
    }
    Terminal a = performNodeExpression(fun.left, fun.leftFlags);
    switch (op)
    {
      case AND: return Terminal(getLiteral<bool>(a) && getLiteral<bool>(b));
      case OR: return Terminal(getLiteral<bool>(a) || getLiteral<bool>(b));
#define PERFORM_OP(op, tp) \
  case ecs::ComponentTypeInfo<tp>::type: return Terminal(getLiteral<tp>(a) op getLiteral<tp>(b));
      case NE:
      case EQ:
      {
        auto isEqual = [&]() {
          switch (a.type)
          {
            PERFORM_OP(==, int)
            PERFORM_OP(==, float)
            PERFORM_OP(==, bool)
            PERFORM_OP(==, ecs::EntityId)
            default: return Terminal();
          }
        };
        bool res = isEqual().id;
        return Terminal(op == EQ ? res : !res);
      }
      case LT:
        switch (a.type)
        {
          PERFORM_OP(<, int)
          PERFORM_OP(<, float)
        }
        [[fallthrough]];
      case LE:
        switch (a.type)
        {
          PERFORM_OP(<=, int)
          PERFORM_OP(<=, float)
        }
        [[fallthrough]];
#undef PERFORM_OP
      default: logerr("unknown op %d", op); return Terminal();
    }
  }

  // type inference check
  static bool validateExpression(const Node *nodes, int nodes_cnt, eastl::string &str)
  {
    return deferTypeExpression(nodes, 0, nodes_cnt == 1 ? IS_TERMINAL : 0, str) == ecs::ComponentTypeInfo<bool>::type;
  }

  static ecs::component_type_t deferTypeExpression(const Node *nodes, int node, int flags, eastl::string &str)
  {
    if (node < 0)
    {
      str = "id<0";
      return 0;
    }
    if (!(flags & IS_TERMINAL))
      return deferTypeBinaryExpression(nodes, nodes[node].fun(), str);
    return nodes[node].term().type;
  }
  static ecs::component_type_t deferTypeBinaryExpression(const Node *nodes, TreeNode fun, eastl::string &str)
  {
    Operand op = fun.getOP();
    ecs::component_type_t a = deferTypeExpression(nodes, fun.left, fun.leftFlags, str);
    ecs::component_type_t b = deferTypeExpression(nodes, fun.right, fun.rightFlags, str);
    if (!a || !b)
      return 0;
    if (a != b)
    {
      str.append_sprintf("operand types differs <0x%X != 0x%X>. Only perform expression on same types", a, b);
      return 0;
    }
    if (op == OPT)
      return b;
    if (op == AND || op == OR)
    {
      if (a != ecs::ComponentTypeInfo<bool>::type)
      {
        str = "operand type is not bool. Only perform and/or on bool types";
        return 0;
      }
      return ecs::ComponentTypeInfo<bool>::type;
    }
    if (op == LT || op == LE)
    {
      if (a != ecs::ComponentTypeInfo<int>::type && a != ecs::ComponentTypeInfo<float>::type)
      {
        str = "operand type is not numeric. Only perform <,>,<= on numeric types";
        return 0;
      }
      return ecs::ComponentTypeInfo<bool>::type;
    }
    if (op == EQ || op == NE)
    {
      if (a != ecs::ComponentTypeInfo<int>::type && a != ecs::ComponentTypeInfo<float>::type &&
          a != ecs::ComponentTypeInfo<bool>::type && a != ecs::ComponentTypeInfo<ecs::EntityId>::type)
      {
        str = "operand type for comparison is not eid, float, int, bool.";
        return 0;
      }
      return ecs::ComponentTypeInfo<bool>::type;
    }
    str = "Unknown binary expr";
    return 0;
  }
};

struct ExpressionTree
{
  // eastl::fixed_vector<char, 4, true> literals;
  eastl::fixed_vector<PerformExpressionTree::Node, 3, true> nodes;
};

// simplest parser. comp==val
// VERY simplistic parser
// supports only component == val, where val is true, false, int literal, float literal, EntityId literal (int:eid)
bool parse_query_filter(ExpressionTree &tree, const char *str, dag::ConstSpan<ecs::ComponentDesc> comps_rw,
  dag::ConstSpan<ecs::ComponentDesc> comps_ro);

inline bool get_component_node(const char *str, const char *end, PerformExpressionTree::Terminal &id,
  dag::ConstSpan<ecs::ComponentDesc> comps_rw, dag::ConstSpan<ecs::ComponentDesc> comps_ro, bool only_opt)
{
  const char was = *end;
  *const_cast<char *>(end) = 0;
  ecs::ComponentDesc comp(ECS_HASH_SLOW(str), 0);
  *const_cast<char *>(end) = was;

  int compId = -1;
  auto compIt = eastl::find_if(comps_rw.begin(), comps_rw.end(), [comp](const ecs::ComponentDesc &a) { return a.name == comp.name; });
  ecs::component_type_t componentTypeName = 0;
  if (compIt == comps_rw.end())
  {
    compIt = eastl::find_if(comps_ro.begin(), comps_ro.end(), [comp](const ecs::ComponentDesc &a) { return a.name == comp.name; });
    if (compIt != comps_ro.end())
    {
      if (only_opt != ((compIt->flags & ecs::CDF_OPTIONAL) == ecs::CDF_OPTIONAL))
        logerr("optional components can't be part of expressions, except opt, which require optional component");
      else
      {
        compId = compIt - comps_ro.begin() + comps_rw.size();
        componentTypeName = compIt->type;
      }
    }
  }
  else
  {
    if (only_opt != ((compIt->flags & ecs::CDF_OPTIONAL) == ecs::CDF_OPTIONAL))
      logerr("optional components can't be part of expressions, except opt, which require optional component");
    else
    {
      compId = compIt - comps_rw.begin();
      componentTypeName = compIt->type;
    }
  }
  if (compId == -1)
  {
    logerr("component <%.*s:0x%X> in expression <%s> is not found", end - str, str, comp.name, str);
    return false;
  }
  if (componentTypeName == ecs::ComponentTypeInfo<ecs::auto_type>::type)
  {
    const ecs::component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(comp.name);
    if (cidx == ecs::INVALID_COMPONENT_INDEX)
    {
      logerr("component <%.*s:0x%X> in expression <%s> is of auto type and not yet registered. Type deference failed", end - str, str,
        comp.name, str);
      return false;
    }
    else
      componentTypeName = g_entity_mgr->getDataComponents().getComponentById(cidx).componentTypeName;
  }
  id = PerformExpressionTree::Terminal{componentTypeName, (uint32_t)compId};
  return true;
}

/*ExpressionTree parse_expression(const char *str, dag::ConstSpan<ecs::ComponentDesc> comps_rw, dag::ConstSpan<ecs::ComponentDesc>
comps_ro)
{
  if (!str)
    return ExpressionTree();
  char * eq = strstr(const_cast<char*>(str), "==");
  if (!eq)
  {
    logerr("only accept <component> == literal for now. Literal is true, false, int, float or int:eid. component is name of component."
           "<%s> doesn't contain '=='", str);
    return ExpressionTree();
  }
  const char *literal = eq+2;

  *eq = 0;
  ecs::ComponentDesc comp(ECS_HASH_SLOW(str), 0);
  *eq = '=';
  if (cidx == ecs::INVALID_COMPONENT_INDEX)//this one is correct. it can be not yet created
    return ExpressionTree();
  PerformExpressionTree::Terminal left{g_entity_mgr->getDataComponents().getComponentById(cidx).componentTypeName, (uint32_t)compId};
  ExpressionTree tree;
  tree.nodes.resize(3);

  PerformExpressionTree::Terminal right;
  if (strstr(literal, "true"))
    right = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<bool>::type, 1};
  else if (strstr(literal, "false"))
    right = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<bool>::type, 0};
  else if (strstr(literal, "."))
  {
    union {float valf; uint32_t vali;} un;
    un.valf = float(atof(eq+2));
    right = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<float>::type, un.vali};
  } else if (strstr(literal, "eid"))
    right = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<ecs::EntityId>::type, (uint32_t)atoi(eq+2)};
  else
    right = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<int>::type, (uint32_t)atoi(eq+2)};
  tree.nodes[0] = PerformExpressionTree::TreeNode{1, 2, PerformExpressionTree::EQ, PerformExpressionTree::TreeNode::RIGHT_LITERAL,
true}; memcpy(&tree.nodes[1], &left, sizeof(left)); memcpy(&tree.nodes[2], &right, sizeof(right)); eastl::string err; if
(!PerformExpressionTree::validateTreeNode(tree.nodes.data(), 0, err))
  {
    logerr("expression <%s> is invalid (%s)", str, err.c_str());
    return ExpressionTree();
  }
  return tree;
}*/

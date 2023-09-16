#include <daECS/core/ecsQuery.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include "queryExpression.h"
#include "parseExpressionLex.h"

namespace expression_parser
{

static PerformExpressionTree::Operand expr_funs[] = {PerformExpressionTree::EQ, PerformExpressionTree::NE, PerformExpressionTree::LT,
  PerformExpressionTree::LE, PerformExpressionTree::LT, PerformExpressionTree::LE, // reversed
  PerformExpressionTree::AND, PerformExpressionTree::OR, PerformExpressionTree::OPT};
static PerformExpressionTree::Operand expr_fun(TOKEN tok)
{
  if (tok >= EQ && tok - EQ < countof(expr_funs))
    return expr_funs[tok - EQ];
  G_ASSERTF(0, "tok = %d", tok);
  return PerformExpressionTree::EQ; // error
}

static bool expr_fun_reversed(TOKEN tok) { return tok == GT || tok == GE; };

struct Parser
{
  dag::ConstSpan<ecs::ComponentDesc> rw, ro;
  ExpressionTree &tree;
  Parser(ExpressionTree &tree_, dag::ConstSpan<ecs::ComponentDesc> rw_, dag::ConstSpan<ecs::ComponentDesc> ro_) :
    tree(tree_), ro(ro_), rw(rw_)
  {}

  static bool assume_expect(TOKEN token, const char *&s)
  {
    const char *at = s;
    G_UNUSED(at);
    Token tok = lex(s);
    if (tok.token == token)
      return true;
    logerr("Was expecting <%s> at %s, got <%s>", tokens[token], at, tokens[tok.token]);
    return false;
  }

  int parse_expression(const char *&s, int &flags)
  {
    // const char *at = s;
    flags = 0;
    Token tok = lex(s);
    if (tok.token >= EQ)
      return parse_binary_function(tok.token, s);
    PerformExpressionTree::Terminal atom;
    flags = PerformExpressionTree::IS_TERMINAL | PerformExpressionTree::IS_LITERAL;
    switch (tok.token)
    {
      case ID:
        flags = PerformExpressionTree::IS_TERMINAL;
        if (!get_component_node(tok.val.str.s, tok.val.str.e, atom, rw, ro, false))
          return -1;
        break;
      case DEC: atom = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<int>::type, tok.val.u}; break;
      case FLT: atom = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<float>::type, tok.val.u}; break;
      case EID: atom = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<ecs::EntityId>::type, tok.val.u}; break;
      case BOOL: atom = PerformExpressionTree::Terminal{ecs::ComponentTypeInfo<bool>::type, tok.val.u}; break;
      default: logerr("UNEXPECTED %d(%s)", tok.token, tokens[tok.token]); return -1;
    }
    tree.nodes.push_back(atom);
    return tree.nodes.size() - 1;
  }

  int parse_opt_function(const char *&s)
  {
    const char *at = s;
    G_UNUSED(s);
    G_UNUSED(at);
    tree.nodes.emplace_back();
    int root = tree.nodes.size() - 1;
    tree.nodes.emplace_back();
    int left = tree.nodes.size() - 1;
    int leftFlags = PerformExpressionTree::IS_TERMINAL;
    Token tok = lex(s);
    if (tok.token != ID)
    {
      logerr("opt function requires first operand to be optional component, at = <%s>", at);
      return -1;
    }
    if (!get_component_node(tok.val.str.s, tok.val.str.e, tree.nodes[left].term(), rw, ro, true))
      return -1;
    if (!assume_expect(COMMA, s))
      return -1;
    int right = -1, rightFlags = 0;
    if ((right = parse_expression(s, rightFlags)) < 0)
      return -1;
    if (!assume_expect(EFUN, s))
      return -1;

    tree.nodes[root] = PerformExpressionTree::TreeNode{
      (int16_t)left, (int16_t)right, PerformExpressionTree::OPT, (uint8_t)leftFlags, (uint8_t)rightFlags};
    return root;
  }
  int parse_binary_function(TOKEN fun, const char *&s)
  {
    if (fun == OPT)
      return parse_opt_function(s);
    tree.nodes.emplace_back();
    int root = tree.nodes.size() - 1;
    int left = -1, right = -1;
    int leftFlags = 0, rightFlags = 0;
    // printf("%s ", tokens[fun]);
    if ((left = parse_expression(s, leftFlags)) < 0)
      return -1;
    if (!assume_expect(COMMA, s))
      return -1;
    if ((right = parse_expression(s, rightFlags)) < 0)
      return -1;
    if (!assume_expect(EFUN, s))
      return -1;

    if (expr_fun_reversed(fun))
    {
      eastl::swap(left, right);
      eastl::swap(leftFlags, rightFlags);
    }
    tree.nodes[root] =
      PerformExpressionTree::TreeNode{(int16_t)left, (int16_t)right, (uint8_t)expr_fun(fun), (uint8_t)leftFlags, (uint8_t)rightFlags};
    return root;
  }

  int parse_simple_expression(Token id, const char *&s)
  {
    const int root = tree.nodes.size();
    tree.nodes.push_back();
    const int left = tree.nodes.size();
    tree.nodes.push_back();
    if (!get_component_node(id.val.str.s, id.val.str.e, tree.nodes[left].term(), rw, ro, false))
      return -1;

    const char *at = s;
    G_UNUSED(at);
    Token tok = lex(s);
    if (tok.token == CMP_EQ)
    {
      int right, rightFlags;
      if ((right = parse_expression(s, rightFlags)) < 0)
        return -1;
      const int leftFlags = PerformExpressionTree::IS_TERMINAL;
      const uint8_t op = PerformExpressionTree::EQ;
      tree.nodes[root].fun() =
        PerformExpressionTree::TreeNode{(int16_t)left, (int16_t)right, op, (uint8_t)leftFlags, (uint8_t)rightFlags};
    }
    else if (tok.token == END)
    {
      tree.nodes[root].term() = tree.nodes[left].term();
      tree.nodes.pop_back();
    }
    else
    {
      logerr("for simple expression we expect either component == val or just component. After ident there is %s (%d)", at, tok.token);
      return -1;
    }
    return root;
  }

  int parse(const char *&s)
  {
    Token tok;
    tok = lex(s);
    if (tok.token < EQ)
    {
      if (tok.token == ID)
      {
        return parse_simple_expression(tok, s);
      }
      else
      {
        logerr("error at <%s> expression should start with function\n!", s);
        return -1;
      }
    }
    return parse_binary_function(tok.token, s);
  }
};

} // namespace expression_parser

bool parse_query_filter(ExpressionTree &tree, const char *str, dag::ConstSpan<ecs::ComponentDesc> comps_rw,
  dag::ConstSpan<ecs::ComponentDesc> comps_ro)
{
  if (!str)
    return false;
  const char *at = str;
  G_UNUSED(at);
  expression_parser::Parser parser(tree, comps_rw, comps_ro);
  if (parser.parse(str) < 0)
  {
    logerr("errors while parsing filter string:<%s>", at);
    tree.nodes.clear();
    return false;
  }
#if DAGOR_DBGLEVEL > 0 // filter in release build will wrongfully read, but will more or less work
  eastl::string err;
  if (!PerformExpressionTree::validateExpression(tree.nodes.data(), tree.nodes.size(), err))
  {
    logerr("type deference fail on expression <%s>, error:%s", at, err.c_str());
    tree.nodes.clear();
    return false;
  }
#endif
  return true;
}

/*int main(int argc, char **argv)
{
    const char *s = argv[1];
    parse_expression::parse(s);
    return 0;
}*/

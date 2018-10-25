struct Variable {
  // Info for name string
  char* start;
  int len;

  int index;
};

array_for(Variable);

// TODO(harrison): remove some of the duplicate from scope_{exists,get,set} functions
struct Scope {
  array(Variable) variables;
};

void scope_init(Scope* s) {
  s->variables = array_Variable_init();
}

bool scope_exists(Scope* s, char* name, int len) {
  for (psize i = 0; i < array_count(s->variables); i++) {
    Variable v = s->variables[i];

    if (v.len != len) {
      continue;
    }

    if (memcmp(name, v.start, len) == 0) {
      return true;
    }
  }

  return false;
}

bool scope_get(Scope* s, char* name, int len, Variable* var) {
  for (psize i = 0; i < array_count(s->variables); i++) {
    Variable v = s->variables[i];

    if (v.len != len) {
      continue;
    }

    if (memcmp(name, v.start, len) == 0) {
      *var = v;

      return true;
    }
  }

  return false;
}

void scope_set(Scope* s, Variable var) {
  // If the variable exists, update it
  for (psize i = 0; i < array_count(s->variables); i++) {
    Variable v = s->variables[i];

    if (v.len != var.len) {
      continue;
    }

    if (memcmp(var.start, v.start, var.len) == 0) {
      s->variables[i] = var;

      return;
    }
  }

  // If it doesn't, add a new one
  array_Variable_add(&s->variables, var);
}

#define AST_NODE_IS_ARITHMETIC(type) ((type) >= AST_NODE_ADD && (type) <= AST_NODE_DIVIDE)

#define AST_NODE_IS_VALUE(type) ((type) == AST_NODE_NUMBER || AST_NODE_IS_ARITHMETIC((type)))

enum ASTNodeType : uint32 {
  AST_NODE_INVALID,
  AST_NODE_ROOT,

  AST_NODE_ASSIGNMENT,
  AST_NODE_ASSIGNMENT_DECLARATION,

  AST_NODE_ADD,
  AST_NODE_SUBTRACT,
  AST_NODE_MULTIPLY,
  AST_NODE_DIVIDE,

  AST_NODE_IF,

  AST_NODE_TEST_EQUAL,
  AST_NODE_TEST_GREATER,
  AST_NODE_TEST_LESSER,

  AST_NODE_IDENTIFIER,
  AST_NODE_VALUE,
  AST_NODE_NUMBER
};

struct ASTNode;

array_for_name(ASTNode*, ASTNodep);
struct ASTNode_Root {
  array(ASTNode*) children;
};

struct ASTNode_Binary {
  ASTNode *left; // Must be an identifier
  ASTNode *right; // Must be an expression (which returns a number)
};

struct ASTNode_Identifier {
  Token token;
};

struct ASTNode_Value {
  Value val;
};

struct ASTNode_Number {
  int number;
};

struct ASTNode_If {
  ASTNode* condition;

  ASTNode* block;
};

struct ASTNode {
  ASTNodeType type;

  int line;

  union {
    ASTNode_Root root;

    ASTNode_Binary assignmentDeclaration;
    ASTNode_Binary assignment;

    ASTNode_Binary binary;

    ASTNode_Binary add;
    ASTNode_Binary subtract;
    ASTNode_Binary multiply;
    ASTNode_Binary divide;

    ASTNode_Binary equality;

    ASTNode_Value value;

    ASTNode_Identifier identifier;
    ASTNode_Number number;

    ASTNode_If cIf;
  };
};

void ast_root_add(ASTNode* parent, ASTNode child) {
  assert(parent->type == AST_NODE_ROOT);

  ASTNode* c = (ASTNode*) malloc(sizeof(child));
  *c = child;

  array_ASTNodep_add(&parent->root.children, c);
}

ASTNode ast_makeRoot() {
  ASTNode node = {};
  node.type = AST_NODE_ROOT;

  node.root.children = array_ASTNodep_init();

  return node;
}

ASTNode ast_makeIdentifier(Token t) {
  ASTNode node = {};
  node.line = t.line;
  node.type = AST_NODE_IDENTIFIER;
  node.identifier.token = t;

  return node;
}

ASTNode ast_makeValue(Value v, Token t) {
  ASTNode node = {};
  node.type = AST_NODE_VALUE;

  node.line = t.line;

  node.value.val = v;

  return node;
}

ASTNode ast_makeOperator(ASTNodeType op, Token t) {
  ASTNode node = {};
  node.line = t.line;

  switch (op) {
    case AST_NODE_ADD:
    case AST_NODE_SUBTRACT:
    case AST_NODE_MULTIPLY:
    case AST_NODE_DIVIDE:

    case AST_NODE_TEST_GREATER:
    case AST_NODE_TEST_LESSER:
    case AST_NODE_TEST_EQUAL:
      {
        node.type = op;
      } break;
    default:
      {
        assert(!"ast_makeOperator must be one of AST_NODE_{ADD,SUBTRACT,MULTIPLY,DIVIDE}.");
      } break;
  }

  return node;
}

ASTNode ast_makeOperatorWith(ASTNodeType op, ASTNode left, ASTNode right, Token t) {
  ASTNode node = ast_makeOperator(op, t);

  // TODO(harrison): free!
  ASTNode* l = (ASTNode*) malloc(sizeof(left));
  ASTNode* r = (ASTNode*) malloc(sizeof(right));

  *l = left;
  *r = right;

  node.add.left = l;
  node.add.right = r;

  return node;
}


ASTNode ast_makeNumber(int n, Token t) {
  ASTNode node = {};
  node.line = t.line;
  node.type = AST_NODE_NUMBER;
  node.number.number = n;

  return node;
}

ASTNode ast_makeAssignmentDeclaration(ASTNode left, ASTNode right, Token t) {
  ASTNode node = {};
  node.line = t.line;
  node.type = AST_NODE_ASSIGNMENT_DECLARATION;

  // TODO(harrison): free!
  ASTNode* l = (ASTNode*) malloc(sizeof(left));
  ASTNode* r = (ASTNode*) malloc(sizeof(right));

  *l = left;
  *r = right;

  node.assignmentDeclaration.left = l;
  node.assignmentDeclaration.right = r;

  return node;
};

ASTNode ast_makeAssignment(ASTNode left, ASTNode right, Token t) {
  ASTNode node = {};
  node.line = t.line;
  node.type = AST_NODE_ASSIGNMENT;

  // TODO(harrison): free!
  ASTNode* l = (ASTNode*) malloc(sizeof(left));
  ASTNode* r = (ASTNode*) malloc(sizeof(right));

  *l = left;
  *r = right;

  node.assignment.left = l;
  node.assignment.right = r;

  return node;
}

ASTNode ast_makeIf(ASTNode condition, ASTNode block) {
  assert(block.type == AST_NODE_ROOT);

  ASTNode node = {};
  node.type = AST_NODE_IF;

  node.cIf.condition = (ASTNode*) malloc(sizeof(condition));
  *node.cIf.condition = condition;

  node.cIf.block = (ASTNode*) malloc(sizeof(block));
  *node.cIf.block = block;

  return node;
}

bool ast_checkType(ASTNode n, ValueType vt) {
  switch (n.type) {
    case AST_NODE_ADD:
    case AST_NODE_SUBTRACT:
    case AST_NODE_MULTIPLY:
    case AST_NODE_DIVIDE:
      {
        return ast_checkType(*n.binary.left, vt) && ast_checkType(*n.binary.right, vt);
      } break;
    case AST_NODE_NUMBER:
      {
        return (vt == VALUE_NUMBER) ? true : false;
      } break;

    case AST_NODE_VALUE:
      {
        return (vt == n.value.val.type);
      } break;
    default:
      return false;
  }
}

#define IS_USED(op) ((op).add.left != 0 && (op).add.right != 0)
bool ast_nodeHasValue(ASTNode n) {
  if (!(n.type == AST_NODE_NUMBER || n.type == AST_NODE_IDENTIFIER)) {
    if (AST_NODE_IS_ARITHMETIC(n.type)) {
      if (!IS_USED(n)) {
        assert(!"Operators can only be used as values when their n & right nodes are used");

        return false;
      }
    } else {
      assert(!"Expecting an addressable value!");

      return false;
    }
  }

  return true;
}
#undef IS_USED

// TODO(harrison): properly propogate errors
bool ast_writeBytecode(ASTNode* node, Hunk* hunk, Scope* scope) {
  printf("node->type %d\n", node->type);

  switch (node->type) {
    case AST_NODE_INVALID:
      {
        printf("reached an invalid source path\n");
        return false;
      } break;
    case AST_NODE_ROOT:
      {
        for (psize i = 0; i < array_count(node->root.children); i++) {
          ASTNode* child = node->root.children[i];

          if (!ast_writeBytecode(child, hunk, scope)) {
            return false;
          }
        }
      } break;
    case AST_NODE_ASSIGNMENT:
      {
        ASTNode* left = node->assignmentDeclaration.left;
        assert(left->type == AST_NODE_IDENTIFIER);

        ASTNode* right = node->assignmentDeclaration.right;
        // assert(ast_nodeHasValue(*right));

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        Variable var = {};
        if (!scope_get(scope, left->identifier.token.start, left->identifier.token.len, &var)) {
          printf("ERROR: variable doesn't exist!\n");

          return false;
        }

        hunk_write(hunk, OP_SET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);
      } break;
    case AST_NODE_ASSIGNMENT_DECLARATION:
      {
        ASTNode* left = node->assignmentDeclaration.left;
        assert(left->type == AST_NODE_IDENTIFIER);

        ASTNode* right = node->assignmentDeclaration.right;
        // assert(ast_nodeHasValue(*right));

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        if (scope_exists(scope, left->identifier.token.start, left->identifier.token.len)) {
          printf("ERROR: variable already exists!\n");

          return false;
        }

        Variable var = {};
        var.start = left->identifier.token.start;
        var.len = left->identifier.token.len;

        var.index = hunk_addLocal(hunk, value_make(VALUE_NUMBER));

        scope_set(scope, var);

        hunk_write(hunk, OP_SET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);
      } break;
    case AST_NODE_IF:
       {
        if (!ast_writeBytecode(node->cIf.condition, hunk, scope)) {
          return false;
        }

        hunk_write(hunk, OP_JUMP_IF_FALSE, 0);
        hunk_write(hunk, 0, 0);

        Instruction offsetPos = hunk->count - 1;

        if (!ast_writeBytecode(node->cIf.block, hunk, scope)) {
          return false;
        }

        Instruction ifEndPos = hunk->count - 1;

        hunk->code[offsetPos] = ifEndPos - offsetPos;
       } break;
#define BINARY_POP() \
       do { \
         ASTNode* left = node->binary.left; \
         ASTNode* right = node->binary.right; \
         if (!ast_writeBytecode(left, hunk, scope)) { \
           return false; \
         } \
         if (!ast_writeBytecode(right, hunk, scope)) { \
           return false; \
         } \
       } while (false);
    case AST_NODE_NUMBER:
      {
        int constant = hunk_addConstant(hunk, value_make((float) node->number.number));
        hunk_write(hunk, OP_CONSTANT, node->line);
        hunk_write(hunk, constant, node->line);
      } break;
    case AST_NODE_VALUE:
      {
        int constant = hunk_addConstant(hunk, node->value.val);
        hunk_write(hunk, OP_CONSTANT, node->line);
        hunk_write(hunk, constant, node->line);
      } break;
    case AST_NODE_IDENTIFIER:
      {
        Variable var = {};

        if (!scope_get(scope, node->identifier.token.start, node->identifier.token.len, &var)) {
          printf("ERROR: variable doesn't exist!\n");

          return false;
        }

        hunk_write(hunk, OP_GET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);
      } break;
    case AST_NODE_TEST_EQUAL:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_EQ, node->line);
      } break;
    case AST_NODE_TEST_GREATER:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_GT, node->line);
      } break;
    case AST_NODE_TEST_LESSER:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_LT, node->line);
      } break;
    case AST_NODE_ADD:
      {
        BINARY_POP();

        hunk_write(hunk, OP_ADD, node->line);
      } break;
    case AST_NODE_SUBTRACT:
      {
        BINARY_POP();

        hunk_write(hunk, OP_SUBTRACT, node->line);
      } break;
    case AST_NODE_MULTIPLY:
      {
        BINARY_POP();

        hunk_write(hunk, OP_MULTIPLY, node->line);
      } break;
    case AST_NODE_DIVIDE:
      {
        BINARY_POP();

        hunk_write(hunk, OP_DIVIDE, node->line);
      } break;
#undef BINARY_POP
    default:
      {
        printf("ERROR: Don't know how to get bytecode from node type %d\n", node->type);

        return false;
      }
  }

  return true;
}

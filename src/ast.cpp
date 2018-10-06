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

  AST_NODE_IDENTIFIER,
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

struct ASTNode_Number {
  int number;
};

struct ASTNode {
  ASTNodeType type;

  int line;

  union {
    ASTNode_Root root;

    ASTNode_Binary assignmentDeclaration;
    ASTNode_Binary assignment;

    ASTNode_Binary add;
    ASTNode_Binary subtract;
    ASTNode_Binary multiply;
    ASTNode_Binary divide;

    ASTNode_Identifier identifier;
    ASTNode_Number number;
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
};

ASTNode ast_makeOperator(ASTNodeType op, Token t) {
  ASTNode node = {};
  node.line = t.line;

  switch (op) {
    case AST_NODE_ADD:
      {
        node.type = AST_NODE_ADD;
      } break;
    case AST_NODE_SUBTRACT:
      {
        node.type = AST_NODE_SUBTRACT;
      } break;
    case AST_NODE_MULTIPLY:
      {
        node.type = AST_NODE_MULTIPLY;
      } break;
    case AST_NODE_DIVIDE:
      {
        node.type = AST_NODE_DIVIDE;
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

#define IS_USED(op) ((op).add.left != 0 && (op).add.right != 0)
bool ast_nodeHasValue(ASTNode n) {
  if (n.type != AST_NODE_NUMBER) {
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
        assert(ast_nodeHasValue(*right));

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        if (!scope_exists(scope, left->identifier.token.start, left->identifier.token.len)) {
          printf("ERROR: variable doesn't exist!\n");

          return false;
        }

        Variable var = {};
        var.start = left->identifier.token.start;
        var.len = left->identifier.token.len;

        var.index = hunk_addLocal(hunk, 0);

        scope_set(scope, var);

        hunk_write(hunk, OP_SET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);

        hunk_write(hunk, OP_GET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);
      } break;
    case AST_NODE_ASSIGNMENT_DECLARATION:
      {
        ASTNode* left = node->assignmentDeclaration.left;
        assert(left->type == AST_NODE_IDENTIFIER);

        ASTNode* right = node->assignmentDeclaration.right;
        assert(ast_nodeHasValue(*right));

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

        var.index = hunk_addLocal(hunk, 0);

        scope_set(scope, var);

        hunk_write(hunk, OP_SET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);

        hunk_write(hunk, OP_GET_LOCAL, node->line);
        hunk_write(hunk, var.index, node->line);
      } break;
    case AST_NODE_ADD:
      {
        ASTNode* left = node->add.left;

        ASTNode* right = node->add.right;

        // write instructions to push left side onto stack
        if(!ast_writeBytecode(left, hunk, scope)) {
          return false;
        }

        // write instructions to push right side onto stack
        if(!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        hunk_write(hunk, OP_ADD, node->line);
      } break;
    case AST_NODE_SUBTRACT:
      {
        ASTNode* left = node->subtract.left;

        ASTNode* right = node->subtract.right;

        // write instructions to push left side onto stack
        if(!ast_writeBytecode(left, hunk, scope)) {
          return false;
        }

        // write instructions to push right side onto stack
        if(!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        hunk_write(hunk, OP_SUBTRACT, node->line);
      } break;
    case AST_NODE_MULTIPLY:
      {
        ASTNode* left = node->multiply.left;

        ASTNode* right = node->multiply.right;

        // write instructions to push left side onto stack
        if(!ast_writeBytecode(left, hunk, scope)) {
          return false;
        }

        // write instructions to push right side onto stack
        if(!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        hunk_write(hunk, OP_MULTIPLY, node->line);
      } break;
    case AST_NODE_DIVIDE:
      {
        ASTNode* left = node->divide.left;

        ASTNode* right = node->divide.right;

        // write instructions to push left side onto stack
        if(!ast_writeBytecode(left, hunk, scope)) {
          return false;
        }

        // write instructions to push right side onto stack
        if(!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        hunk_write(hunk, OP_DIVIDE, node->line);
      } break;
    case AST_NODE_NUMBER:
      {
        int constant = hunk_addConstant(hunk, node->number.number);
        hunk_write(hunk, OP_CONSTANT, node->line);
        hunk_write(hunk, constant, node->line);
      } break;
    default:
      {
        printf("ERROR: Don't know how to get bytecode from node type %d\n", node->type);

        return false;
      }
  }

  return true;
}

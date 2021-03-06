struct Variable {
  char* start;
  int len;
  // TODO(harrison): embed token in here instead of start/len

  int slot;
};

#define MAX_SYMBOLS (10)
struct Scope {
  int count;
  Variable variables[MAX_SYMBOLS];

  Scope* parent;
};

void scope_init(Scope* s) {
  s->count = 0;
  s->parent = 0;
}

void scope_init(Scope* s, Scope *p) {
  scope_init(s);

  s->parent = p;
}

int scope_getNextSlot(Scope *s) {
  int n = s->count;
  if (s->parent != 0) {
    n += scope_getNextSlot(s->parent);
  }

  return n;
}

bool scope_get(Scope* s, char* name, int len, int* slot) {
  for (int i = 0; i < s->count; i++) {
    Variable v = s->variables[i];

    if (v.len != len) {
      continue;
    }

    if (memcmp(name, v.start, len) == 0) {
      *slot = v.slot;

      return true;
    }
  }

  if (s->parent != 0) {
    return scope_get(s->parent, name, len, slot);
  }

  return false;
}

bool scope_get(Scope* s, Token ident, int* slot) {
  return scope_get(s, ident.start, ident.len, slot);
}

int scope_set(Scope* s, Variable* var) {
  for (int i = 0; i < s->count; i++) {
    Variable v = s->variables[i];

    if (v.len != var->len) {
      break;
    }

    if (strncmp(v.start, var->start, v.len) == 0) {
      return false;
    }
  }

  assert(s->count < MAX_SYMBOLS);

  var->slot = scope_getNextSlot(s);

  s->variables[s->count] = *var;
  s->count += 1;

  return var->slot;
}

enum ASTNodeType : uint32 {
  AST_NODE_INVALID,
  AST_NODE_ROOT,
  AST_NODE_ASSIGNMENT,
  AST_NODE_ASSIGNMENT_DECLARATION,
  AST_NODE_FUNCTION_CALL,
  AST_NODE_ADD,
  AST_NODE_SUBTRACT,
  AST_NODE_MULTIPLY,
  AST_NODE_DIVIDE,
  AST_NODE_IF,
  AST_NODE_FUNCTION_DECLARATION,
  AST_NODE_TEST_EQUAL,
  AST_NODE_TEST_GREATER,
  AST_NODE_TEST_GREATER_EQUAL,
  AST_NODE_TEST_LESSER,
  AST_NODE_TEST_LESSER_EQUAL,
  AST_NODE_TEST_OR,
  AST_NODE_TEST_AND,
  AST_NODE_IDENTIFIER,
  AST_NODE_VALUE,
  AST_NODE_NUMBER,
  AST_NODE_LOG,
  AST_NODE_DECLARATION,
  AST_NODE_RETURN,
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
  ASTNode* elseBlock;
};

struct Parameter {
  Token identifier;
  Token type;
};

struct ASTNode_Declaration {
  Token identifier;
  Token type;
};

array_for(Parameter);

struct ASTNode_Function {
  Token identifier;

  Token returnType;

  ASTNode* block;

  array(Parameter) parameters;
};

struct ASTNode_FunctionCall {
  Token identifier;

  array(ASTNode) args;
};

struct ASTNode_Return {
  ASTNode* child;
};

struct ASTNode_Log {};

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

    ASTNode_Binary And;
    ASTNode_Binary Or;

    ASTNode_Return Return;

    ASTNode_Binary equality;

    ASTNode_Value value;

    ASTNode_Identifier identifier;
    ASTNode_Number number;

    ASTNode_If cIf;

    ASTNode_Function functionDeclaration;
    ASTNode_FunctionCall functionCall;

    ASTNode_Log log;

    ASTNode_Declaration declaration;
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
    case AST_NODE_TEST_GREATER_EQUAL:
    case AST_NODE_TEST_LESSER_EQUAL:
    case AST_NODE_TEST_EQUAL:
    case AST_NODE_TEST_AND:
    case AST_NODE_TEST_OR:
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

  node.cIf.elseBlock = 0;

  return node;
}

ASTNode ast_makeIf(ASTNode condition, ASTNode block, ASTNode elseBlock) {
  assert(block.type == AST_NODE_ROOT && elseBlock.type == AST_NODE_ROOT);

  ASTNode node = {};
  node.type = AST_NODE_IF;

  node.cIf.condition = (ASTNode*) malloc(sizeof(condition));
  *node.cIf.condition = condition;

  node.cIf.block = (ASTNode*) malloc(sizeof(block));
  *node.cIf.block = block;

  node.cIf.elseBlock = (ASTNode*) malloc(sizeof(elseBlock));
  *node.cIf.elseBlock = elseBlock;

  return node;
}

ASTNode ast_makeFunctionDeclaration(Token ident, ASTNode block, array(Parameter) params, Token ret) {
  assert(block.type == AST_NODE_ROOT);

  ASTNode node = {};
  node.type = AST_NODE_FUNCTION_DECLARATION;

  node.functionDeclaration.parameters = params;

  node.functionDeclaration.returnType = ret;

  node.functionDeclaration.identifier = ident;
  node.functionDeclaration.block = (ASTNode*) malloc(sizeof(block));
  *node.functionDeclaration.block = block;

  return node;
}

ASTNode ast_makeFunctionCall(Token t, array(ASTNode) args) {
  ASTNode node = {};
  node.line = t.line;
  node.type = AST_NODE_FUNCTION_CALL;
  node.functionCall.identifier = t;
  node.functionCall.args = args;

  return node;
}

ASTNode ast_makeLog() {
  ASTNode node = {};
  node.type = AST_NODE_LOG;

  return node;
}

ASTNode ast_makeDeclaration(Token ident, Token type) {
  ASTNode node = {};
  node.line = ident.line;
  node.type = AST_NODE_DECLARATION;
  node.declaration.identifier = ident;
  node.declaration.type = type;

  return node;
}

ASTNode ast_makeReturn(ASTNode expr, Token t) {
  ASTNode node = {};
  node.type = AST_NODE_RETURN;

  node.line = t.line;

  node.Return.child = (ASTNode*) malloc(sizeof(expr));
  *node.Return.child = expr;

  return node;
}

// TODO(harrison): properly propogate errors
bool ast_writeBytecode(ASTNode* node, Hunk* hunk, Scope* scope) {
  switch (node->type) {
    case AST_NODE_INVALID:
      {
        logf("reached an invalid source path\n");

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

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        int slot = -1;
        if (!scope_get(scope, left->identifier.token.start, left->identifier.token.len, &slot)) {
          logf("ERROR: variable doesn't exist2!\n");

          return false;
        }

        hunk_write(hunk, OP_SET_LOCAL, node->line);
        hunk_write(hunk, slot, node->line);
      } break;
    case AST_NODE_ASSIGNMENT_DECLARATION:
      {
        ASTNode* left = node->assignmentDeclaration.left;
        assert(left->type == AST_NODE_IDENTIFIER);

        ASTNode* right = node->assignmentDeclaration.right;

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        Variable var = {};
        var.start = left->identifier.token.start;
        var.len = left->identifier.token.len;

        if (scope_set(scope, &var) == -1) {
          logf("Variable already exists\n");

          return false;
        }

        hunk_write(hunk, OP_SET_LOCAL, node->line);
        hunk_write(hunk, var.slot, node->line);
      } break;
    case AST_NODE_FUNCTION_DECLARATION:
      {
        Token ident = node->functionDeclaration.identifier;
        Value nameVal = value_make(ident.start, ident.len);

        Hunk* h = (Hunk*) malloc(sizeof(Hunk));
        hunk_init(h);

        Scope s = {};
        scope_init(&s);

        for (psize i = 0; i < array_count(node->functionDeclaration.parameters); i++) {
          Parameter p = node->functionDeclaration.parameters[i];
          Variable var = {};
          var.start = p.identifier.start;
          var.len = p.identifier.len;

          if (scope_set(&s, &var) == -1) {
            logf("can't set parameter. something weird is happening.\n");

            return false;
          }
        }

        if (!ast_writeBytecode(node->functionDeclaration.block, h, &s)) {
          return false;
        }

        hunk_write(h, OP_RETURN, 0);
        hunk_write(h, 0, 0);

        Value funcVal = value_make(h);

        int name = hunk_addConstant(hunk, nameVal);
        int func = hunk_addConstant(hunk, funcVal);

        hunk_write(hunk, OP_CONSTANT, node->line);
        hunk_write(hunk, name, node->line);
        hunk_write(hunk, OP_CONSTANT, node->line);
        hunk_write(hunk, func, node->line);

        hunk_write(hunk, OP_SET_GLOBAL, node->line);
      } break;
    case AST_NODE_FUNCTION_CALL:
      {
        // Get all parameters and push them onto stack
        for (psize i = 0; i < array_count(node->functionCall.args); i++) {
          ASTNode* arg = &node->functionCall.args[i];

          if (!ast_writeBytecode(arg, hunk, scope)) {
            return false;
          }
        }

        Token ident = node->functionCall.identifier;
        Value nameVal = value_make(ident.start, ident.len);

        int name = hunk_addConstant(hunk, nameVal);
        hunk_write(hunk, OP_CONSTANT, node->line);
        hunk_write(hunk, name, node->line);

        hunk_write(hunk, OP_GET_GLOBAL, node->line);

        hunk_write(hunk, OP_CALL, node->line);
        hunk_write(hunk, (Instruction) array_count(node->functionCall.args), node->line);
      } break;
    case AST_NODE_IF:
       {
        if (!ast_writeBytecode(node->cIf.condition, hunk, scope)) {
          return false;
        }

        Scope inner = {};
        scope_init(&inner, scope);

        hunk_write(hunk, OP_JUMP_IF_FALSE, 0);
        hunk_write(hunk, 0, 0);

        Instruction nextStatementPos = hunk_getCount(hunk) - 1;

        if (!ast_writeBytecode(node->cIf.block, hunk, &inner)) {
          return false;
        }

        if (node->cIf.elseBlock != 0) {
          Scope elseInner = {};
          scope_init(&elseInner, scope);

          hunk_write(hunk, OP_JUMP, 0);
          hunk_write(hunk, 0, 0);

          Instruction exitBlockPos = hunk_getCount(hunk) - 1;

          hunk->code[nextStatementPos] = hunk_getCount(hunk) - 1 - nextStatementPos;

          if (!ast_writeBytecode(node->cIf.elseBlock, hunk, &elseInner)) {
            return false;
          }

          Instruction endOfElsePos = hunk_getCount(hunk) - 1;
          hunk->code[exitBlockPos] = endOfElsePos - exitBlockPos;
        } else {
          Instruction ifEndPos = hunk_getCount(hunk) - 1;

          hunk->code[nextStatementPos] = ifEndPos - nextStatementPos;
        }
       } break;
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
        int slot = -1;
        if (!scope_get(scope, node->identifier.token.start, node->identifier.token.len, &slot)) {
          logf("ERROR: variable doesn't exist1!\n");

          return false;
        }

        hunk_write(hunk, OP_GET_LOCAL, node->line);
        hunk_write(hunk, slot, node->line);
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
    case AST_NODE_TEST_GREATER_EQUAL:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_GTE, node->line);
      } break;
    case AST_NODE_TEST_LESSER_EQUAL:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_LTE, node->line);
      } break;
    case AST_NODE_TEST_AND:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_AND, node->line);
      } break;
    case AST_NODE_TEST_OR:
      {
        BINARY_POP();

        hunk_write(hunk, OP_TEST_OR, node->line);
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
    case AST_NODE_LOG:
      {
        hunk_write(hunk, OP_LOG, node->line);
      } break;
    case AST_NODE_RETURN:
      {
        if (!ast_writeBytecode(node->Return.child, hunk, scope)) {
          return false;
        }

        hunk_write(hunk, OP_RETURN, node->line);
        hunk_write(hunk, 1, node->line);
      } break;
    default:
      {
        logf("ERROR: Don't know how to get bytecode from node type %d\n", node->type);

        return false;
      }
  }

  return true;
}

enum SymbolType {
  // x := 10
  SYMBOL_DECLARATION,

  // int, bool, string, float
  SYMBOL_ATOMIC,

  // function name, function parameters
  SYMBOL_FUNCTION,
};

struct Symbol;

struct Symbol_Declaration {
  Symbol* typeSymbol;
};

struct Symbol_Atomic {
  ValueType type;

  Value zero;
};

struct Symbol_Function {
//  Symbol* returnType;

  array(Symbol*) parameterTypes;
};

int Symbol_Atomic_Number;
int Symbol_Atomic_Bool;

struct Symbol {
  int id;

  char* name;
  int nameLen;

  SymbolType type;

  union {
    Symbol_Declaration declaration;
    Symbol_Atomic atomic;
    Symbol_Function function;
  } info;
};

array_for_name(Symbol*, Symbolp)

Symbol symbol_makeAtomic(const char* str, ValueType t) {
  Symbol sym = {};
  sym.type = SYMBOL_ATOMIC;

  sym.name = (char*) str;
  sym.nameLen = strlen(str);

  sym.info.atomic.type = t;

  sym.info.atomic.zero = value_make(t);

  return sym;
}

Symbol symbol_makeDeclaration(char* str, int strLen, Symbol* type) {
  Symbol sym = {};
  sym.type = SYMBOL_DECLARATION;

  sym.name = str;
  sym.nameLen = strLen;

  sym.info.declaration.typeSymbol = type;

  return sym;
}

Symbol symbol_makeFunction(char* str, int strLen, array(Symbol*) paramTypes) {
  Symbol sym = {};
  sym.type = SYMBOL_FUNCTION;

  sym.name = str;
  sym.nameLen = strLen;

  sym.info.function.parameterTypes = paramTypes;

  return sym;
}

struct SymbolTable {
  Symbol symbols[MAX_SYMBOLS];
  int count;

  SymbolTable* parent;
};

// init adds a parent to a symbol table.
void symbolTable_init(SymbolTable* symbols, SymbolTable* parent) {
  symbols->parent = parent;
}

// add adds a symbol into the symbol table. Return value is false iff the
// symbols name already exists in the current level of scope.
bool symbolTable_add(SymbolTable* symbols, Symbol* sym) {
  for (int i = 0; i < symbols->count; i++) {
    Symbol s = symbols->symbols[i];

    // compare s and sym names
    if (s.nameLen != sym->nameLen) {
      break;
    }

    if (strncmp(s.name, sym->name, s.nameLen) == 0) {
      return false;
    }
  }

  assert(symbols->count < MAX_SYMBOLS);

  sym->id = symbols->count;

  symbols->symbols[symbols->count] = *sym;

  symbols->count += 1;

  return true;
}

bool symbolTable_get(SymbolTable* symbols, char* name, int nameLen, Symbol** sym) {
  for (int i = 0; i < symbols->count; i++) {
    Symbol s = symbols->symbols[i];

    if (s.nameLen != nameLen) {
      continue;
    }

    if (strncmp(s.name, name, nameLen) == 0) {
      *sym = &symbols->symbols[i];

      return true;
    }
  }

  if (symbols->parent != 0) {
    return symbolTable_get(symbols->parent, name, nameLen, sym);
  }

  return false;
}

SymbolTable symbolTable_makeDefaults() {
  SymbolTable symbols = {};

  Symbol Number = symbol_makeAtomic("number", VALUE_NUMBER);
  Symbol Bool = symbol_makeAtomic("bool", VALUE_BOOL);

  symbolTable_add(&symbols, &Number);
  symbolTable_add(&symbols, &Bool);

  Symbol_Atomic_Number = Number.id;
  Symbol_Atomic_Bool = Bool.id;

  return symbols;
}

SymbolTable DefaultSymbols = symbolTable_makeDefaults();

bool getType(ASTNode* node, SymbolTable* symbols, Symbol** sym) {
  switch (node->type) {
    case AST_NODE_ADD:
    case AST_NODE_SUBTRACT:
    case AST_NODE_MULTIPLY:
    case AST_NODE_DIVIDE:
      {
        Symbol* lhs = 0;
        if (!getType(node->assignment.left, symbols, &lhs)) {
          return false;
        }

        Symbol* rhs = 0;
        if (!getType(node->assignment.right, symbols, &rhs)) {
          return false;
        }

        if (lhs->id != rhs->id) {
          printf("left and right hand side are different types\n");

          return false;
        }

        if (lhs->id != Symbol_Atomic_Number) {
          printf("one of the types is not a number\n");

          return false;
        }

        *sym = rhs;

        return true;
      } break;
    case AST_NODE_TEST_EQUAL:
    case AST_NODE_TEST_GREATER:
    case AST_NODE_TEST_LESSER:
      {
        Symbol* lhs = 0;
        if (!getType(node->assignment.left, symbols, &lhs)) {
          return false;
        }

        Symbol* rhs = 0;
        if (!getType(node->assignment.right, symbols, &rhs)) {
          return false;
        }

        if (lhs->id != rhs->id) {
          printf("left and right hand side are different types\n");

          return false;
        }

        char* Bool = (char*) "bool";
        assert(symbolTable_get(symbols, Bool, 4, sym));

        return true;
      } break;
    case AST_NODE_TEST_AND:
    case AST_NODE_TEST_OR:
      {
        Symbol* lhs = 0;
        if (!getType(node->assignment.left, symbols, &lhs)) {
          return false;
        }

        Symbol* rhs = 0;
        if (!getType(node->assignment.right, symbols, &rhs)) {
          return false;
        }

        if (lhs->id != rhs->id) {
          printf("left and right hand side are different types\n");

          return false;
        }

        if (lhs->id != Symbol_Atomic_Bool) {
          printf("one (or more) of the types is not a boolean\n");

          return false;
        }

        *sym = rhs;

        return true;
      } break;
    case AST_NODE_NUMBER:
      {
        // TODO(harrison): lookup via Symbol_Atomic_Number field
        char* number = (char*) "number";
        assert(symbolTable_get(symbols, number, 6, sym));

        return true;
      } break;
    case AST_NODE_IDENTIFIER:
      {
        Token tok = node->identifier.token;

        Symbol* decl = 0;
        if (!symbolTable_get(symbols, tok.start, tok.len, &decl)) {
          printf("ident doesn't exist\n");
          return false;
        }

        *sym = decl->info.declaration.typeSymbol;

        return true;
      } break;
    case AST_NODE_VALUE:
      {
        Value v = node->value.val;

        char* str;
        int len;

        switch (v.type) {
          case VALUE_NUMBER:
            {
              str = (char*) "number";
              len = 6;
            } break;
          case VALUE_BOOL:
            {
              str = (char*) "bool";
              len = 4;
            } break;
          default:
            {
              logf("value type not supported currently\n");

              return false;
            } break;
        }

        assert(symbolTable_get(symbols, str, len, sym));

        return true;
      } break;
    default:
      {
        printf("can't get type of AST node type: %d\n", node->type);
      } break;
  }

  return false;
}

bool typeCheck(ASTNode* node, SymbolTable* symbols) {
  switch (node->type) {
    case AST_NODE_ROOT:
      {
        for (psize i = 0; i < array_count(node->root.children); i++) {
          ASTNode* child = node->root.children[i];

          if (!typeCheck(child, symbols)) {
            return false;
          }
        }

        return true;
      } break;
    case AST_NODE_DECLARATION:
      // NOTE(harrison): Declaration is a bit tricky, because no initial value
      // is supplied -- so we need to give one (there are no nil values in
      // loaf). We transform this node type into an
      // AST_NODE_ASSIGNMENT_DECLARATION using the "zero" value recorded in the
      // relevant atomic type symbol.
      //
      // THIS NODE SHOULD NEVER MAKE IT PAST THIS STAGE.
      {
        Symbol* type = 0;
        {
          Token tok = node->declaration.type;
          if (!symbolTable_get(symbols, tok.start, tok.len, &type)) {
            printf("can't find type: %.*s\n", tok.len, tok.start);

            return false;
          }
        }

        // TODO(harrison): add symbol_getZero function when we add support for objects
        assert(type->type == SYMBOL_ATOMIC);

        ASTNode n = ast_makeAssignmentDeclaration(
            ast_makeIdentifier(node->declaration.identifier),
            ast_makeValue(type->info.atomic.zero, node->declaration.type),
            node->declaration.identifier);

        *node = n;

        return typeCheck(node, symbols);
      } break;
    case AST_NODE_ASSIGNMENT:
      {
        Symbol* lhs = 0;
        if (!getType(node->assignment.left, symbols, &lhs)) {
          return false;
        }

        Symbol* rhs = 0;
        if (!getType(node->assignment.right, symbols, &rhs)) {
          return false;
        }

        if (lhs->id != rhs->id) {
          printf("differing types\n");

          return false;
        }

        return true;
      } break;
    case AST_NODE_ASSIGNMENT_DECLARATION:
      {
        Symbol* type = 0;
        if (!getType(node->assignmentDeclaration.right, symbols, &type)) {
          return false;
        }

        assert(node->assignmentDeclaration.left->type == AST_NODE_IDENTIFIER);

        Token tok = node->assignmentDeclaration.left->identifier.token;

        Symbol identifier = symbol_makeDeclaration(tok.start, tok.len, type);
        if (!symbolTable_add(symbols, &identifier)) {
          printf("symbol '%.*s' already exists in current scope\n", identifier.nameLen, identifier.name);

          return false;
        }

        printf("declared: '%.*s' of type %.*s\n", identifier.nameLen, identifier.name, type->nameLen, type->name);

        return true;
      } break;
    case AST_NODE_FUNCTION_DECLARATION:
      {
        array(Symbol*) params = array_Symbolp_init();

        for (int i = 0; i < (int) array_count(node->functionDeclaration.parameters); i++) {
          Parameter p = node->functionDeclaration.parameters[i];

          Symbol* type = 0;
          if (!symbolTable_get(symbols, p.type.start, p.type.len, &type)) {
            printf("unknown type: %.*s\n", p.type.len, p.type.start);

            return false;
          }

          // TODO(harrison): ensure symbol added is actually a type (and not a declaration)

          array_Symbolp_add(&params, type);
        }

        Token tok = node->functionDeclaration.identifier;

        Symbol function = symbol_makeFunction(tok.start, tok.len, params);
        if (!symbolTable_add(symbols, &function)) {
          printf("symbol '%.*s' already exists in current scope\n", function.nameLen, function.name);

          return false;
        }

        SymbolTable childSymbols = {};
        symbolTable_init(&childSymbols, &DefaultSymbols);

        for (int i = 0; i < (int) array_count(node->functionDeclaration.parameters); i++) {
          Parameter p = node->functionDeclaration.parameters[i];
          ASTNode temp = ast_makeDeclaration(p.identifier, p.type);

          if (!typeCheck(&temp, &childSymbols)) {
            printf("something failed setting up a parameter\n");

            return false;
          }
        }

        return typeCheck(node->functionDeclaration.block, &childSymbols);
      } break;
    case AST_NODE_FUNCTION_CALL:
      {
        Symbol* function = 0;

        Token ident = node->functionCall.identifier;
        {
          if (!symbolTable_get(symbols, ident.start, ident.len, &function)) {
            printf("can't get symbol\n: %.*s", ident.len, ident.start);

            return false;
          }
        }

        if (function->type != SYMBOL_FUNCTION) {
          printf("symbol %.*s is not a function\n", ident.len, ident.start);

          return false;
        }

        if (array_count(function->info.function.parameterTypes) != array_count(node->functionCall.args)) {
          printf("argument count mismatch\n");

          return false;
        }

        for (psize i = 0; i < array_count(function->info.function.parameterTypes); i++) {
          ASTNode* arg = &node->functionCall.args[i];
          Symbol* paramType = function->info.function.parameterTypes[i];

          Symbol* argType = 0;
          if (!getType(arg, symbols, &argType)) {
            return false;
          }

          if (argType->id != paramType->id) {
            printf("parameter has wrong type!\n");

            return false;
          }
        }

        return true;
      } break;
    case AST_NODE_IF:
      {
        Symbol* condType = 0;
        if (!getType(node->cIf.condition, symbols, &condType)) {
          return false;
        }

        if (condType->id != Symbol_Atomic_Bool) {
          printf("expression does not evaluate to a bool\n");

          return false;
        }

        SymbolTable childSymbols = {};
        symbolTable_init(&childSymbols, symbols);

        return typeCheck(node->cIf.block, &childSymbols);
      } break;
    case AST_NODE_IDENTIFIER:
    case AST_NODE_LOG:
      {
        return true;
      } break;
    default:
      {
        printf("can't typecheck: %d\n", node->type);
      }
  }

  return false;
}

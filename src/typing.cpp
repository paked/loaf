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
};

struct Symbol_Function {
//  Symbol* returnType;

  array(Symbol_Declaration) parameters;
};

struct Symbol {
  char* name;
  int nameLen;

  SymbolType type;

  union {
    Symbol_Declaration declaration;
    Symbol_Atomic atomic;
    Symbol_Function function;
  } info;
};

Symbol symbol_makeAtomic(const char* str, ValueType t) {
  Symbol sym = {};
  sym.type = SYMBOL_ATOMIC;

  sym.name = (char*) str;
  sym.nameLen = strlen(str);

  sym.info.atomic.type = t;

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

#define MAX_SYMBOLS (10)
struct SymbolTable {
  Symbol symbols[MAX_SYMBOLS];
  int count;

  SymbolTable* parent;
};

// add adds a symbol into the symbol table. Return value is false iff the
// symbols name already exists in the current level of scope.
bool symbolTable_add(SymbolTable* symbols, Symbol sym) {
  for (int i = 0; i < symbols->count; i++) {
    Symbol s = symbols->symbols[i];
    // compare s and sym names

    if (s.nameLen != sym.nameLen) {
      break;
    }

    if (strncmp(s.name, sym.name, s.nameLen) == 0) {
      return false;
    }
  }

  assert(symbols->count < MAX_SYMBOLS);

  symbols->symbols[symbols->count] = sym;

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

  // TODO(harrison): search parent table

  return false;
}

bool getType(ASTNode* node, SymbolTable* symbols, Symbol** sym) {
  switch (node->type) {
    case AST_NODE_ADD:
      {
      } break;
    case AST_NODE_IDENTIFIER:
      {
        Token tok = node->identifier.token;

        if (!symbolTable_get(symbols, tok.start, tok.len, sym)) {
          return false;
        }

        return true;
      } break;
    default:
      {
        printf("can't get type of something\n");
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
      {
        Symbol* type = 0;
        {
          Token tok = node->declaration.type;
          if (!symbolTable_get(symbols, tok.start, tok.len, &type)) {
            printf("can't find type: %.*s\n", tok.len, tok.start);

            return false;
          }
        }

        Symbol identifier = symbol_makeDeclaration(node->declaration.identifier.start, node->declaration.identifier.len, type);
        symbolTable_add(symbols, identifier);

        printf("declared: '%.*s' of type %.*s\n", identifier.nameLen, identifier.name, type->nameLen, type->name);

        return true;
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

        /*
        if (!symbol_isSame(*lhs, *rhs)) {
          return false;
        }*/

        return true;
      } break;
    default:
      {
        printf("can't typecheck: %d\n", node->type);
      }
  }

  return false;
}

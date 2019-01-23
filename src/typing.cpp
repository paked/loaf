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

  // TODO(harrison): search parent table

  return false;
}

bool getType(ASTNode* node, SymbolTable* symbols, Symbol** sym) {
  switch (node->type) {
    case AST_NODE_ADD:
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
    case AST_NODE_NUMBER:
      {
        // TODO(harrison): lookup via Symbol_Atomic_Number field
        char* number = (char*) "number";
        if (!symbolTable_get(symbols, number, strlen(number), sym)) {
          return false;
        }

        return true;
      } break;
    case AST_NODE_IDENTIFIER:
      {
        Token tok = node->identifier.token;

        Symbol* decl = 0;
        if (!symbolTable_get(symbols, tok.start, tok.len, &decl)) {
          return false;
        }

        *sym = decl->info.declaration.typeSymbol;

        return true;
      } break;
    default:
      {
        printf("can't get type of %d\n", node->type);
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
        symbolTable_add(symbols, &identifier);

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
        symbolTable_add(symbols, &identifier);

        printf("declared: '%.*s' of type %.*s\n", identifier.nameLen, identifier.name, type->nameLen, type->name);
      } break;
    default:
      {
        printf("can't typecheck: %d\n", node->type);
      }
  }

  return false;
}

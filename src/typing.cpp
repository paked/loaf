enum {
  // x := 10
  SYMBOL_DECLARATION,

  // int, bool, string, float
  SYMBOL_ATOMIC,

  // function name, function parameters
  SYMBOL_FUNCTION,
};

struct Symbol;

struct Symbol_Declaration {
  Symbol* type;
};

struct Symbol_Atomic {};

struct Symbol_Function {
//  Symbol* returnType;

  array(Symbol_Declaration) parameters;
};

struct Symbol {
  char* name;
  int nameLen;
};

#define MAX_SYMBOLS (10)
struct SymbolTable {
  Symbol symbols[MAX_SYMBOLS];
  int count;

  SymbolScope* parent;
};

bool typeCheck(ASTNode* node) {

}

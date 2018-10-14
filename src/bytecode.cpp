typedef uint16 Instruction;

enum OPCode : Instruction {
  OP_RETURN,

  OP_SET_LOCAL,
  OP_GET_LOCAL,

  // Load constant onto stack
  OP_CONSTANT,

  // Unary negation
  OP_NEGATE,

  // Binary expressions: a # b (where # is an arithmathic operation)
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,

  OP_TEST_EQ,   // ==
  OP_TEST_GT,   // >
  OP_TEST_LT,   // <
  OP_TEST_OR,   // ||
  OP_TEST_AND,  // &&

  OP_JUMP_IF_FALSE,
};

enum ValueType {
  VALUE_NUMBER,
  VALUE_BOOL,

  // VALUE_OBJ
};

#define VALUE_IS_NUMBER(v) ((v).type == VALUE_NUMBER)
#define VALUE_IS_BOOL(v) ((v).type == VALUE_BOOL)

struct Value {
  ValueType type;

  union {
    float number;
    bool boolean;

    // Object* object;
  } as;
};

Value value_make(ValueType type) {
  Value v = {};
  v.type = type;

  switch (v.type) {
    case VALUE_NUMBER:
      { v.as.number = 0; } break;
    case VALUE_BOOL:
      { v.as.boolean = false; } break;
    default:
      {
        assert(!"Unknown value type");
      }
  }

  return v;
}

Value value_make(float t) {
  Value v = {};
  v.type = VALUE_NUMBER;

  v.as.number = t;

  return v;
}

Value value_make(bool t) {
  Value v = {};
  v.type = VALUE_BOOL;

  v.as.boolean = t;

  return v;
}

void value_print(Value v) {
  switch (v.type) {
    case VALUE_NUMBER:
      {
        printf("%f", v.as.number);
      } break;
    case VALUE_BOOL:
      {
        printf(v.as.boolean ? "true" : "false");

        break;
      }
    default:
      {
        printf("unknown: %d", v.type);
      }
  };
}

void value_println(Value v) {
  value_print(v);

  printf("\n");
}

bool value_equals(Value left, Value right) {
  if (left.type != right.type) {
    // TODO(harrison): this should not be possible. We need a type system.

    assert(!"Can't compare variables of different types");
  }

  switch (left.type) {
    case VALUE_BOOL:
      {
        return left.as.boolean == right.as.boolean;
      } break;
    case VALUE_NUMBER:
      {
        return us_equals(left.as.number, right.as.number);
      } break;
    default:
      {
        assert(!"Unknown type");
      }
  }

  return false;
}

// Constants keeps track of all the available constants in a program. Zero is initialisation.
// TODO(harrison): force constants count to fit within the maximum addressable space by opcodes (256)
#define CONSTANTS_CAPACITY_GROW(c) ( ((c) < 8) ? 8 : (c)*2 )
struct Constants {
  int count;
  int capacity;

  // NOTE(harrison): Don't store any references to this pointer. It can change
  // unexpectedly.
  Value* values;
};

// returns -1 on failure
int constants_add(Constants* constants, Value val) {
  if (constants->capacity < constants->count + 1) {
    constants->capacity = CONSTANTS_CAPACITY_GROW(constants->capacity);

    constants->values = REALLOC(Value, constants->values, constants->capacity);

    if (constants->values == 0) {
      return -1;
    }
  }

  *(constants->values + constants->count) = val;

  constants->count += 1;

  return constants->count - 1;
}

// Hunk represents a section of Loaf bytecode from a file/module/whatever
// distinction I decide to make. Zero is initialisation. There is no explicit
// free function as Hunk's are expected to last the duration of the program, so
// the operating system can dispose of their memory much better than us.
#define HUNK_CAPACITY_GROW(c) ( ((c) < 8) ? 8 : (c)*2 )
struct Hunk {
  int count;
  int capacity;

  // NOTE(harrison): Don't store any references to this pointer. It can change
  // unexpectedly.
  Instruction* code;
  uint32* lines;

  Constants constants;
  Constants locals;
};

bool hunk_write(Hunk* hunk, Instruction in, uint32 line) {
  if (hunk->capacity < hunk->count + 1) {
    hunk->capacity = HUNK_CAPACITY_GROW(hunk->capacity);

    hunk->code = REALLOC(Instruction, hunk->code, hunk->capacity);
    if (hunk->code == 0) {
      return false;
    }

    hunk->lines = REALLOC(uint32, hunk->lines, hunk->capacity);
    if (hunk->lines == 0) {
      return false;
    }
  }

  *(hunk->code + hunk->count) = in;
  *(hunk->lines + hunk->count) = line;

  hunk->count += 1;

  return true;
}

int hunk_addConstant(Hunk* hunk, Value val) {
  return constants_add(&hunk->constants, val);
}

int hunk_addLocal(Hunk* hunk, Value val) {
  return constants_add(&hunk->locals, val);
}

int hunk_disassembleInstruction(Hunk* hunk, int offset) {
  printf("%04d | %04d | ", hunk->lines[offset], offset);

  Instruction in = hunk->code[offset];
#define SIMPLE_INSTRUCTION(Code) \
  case Code: \
    { \
      printf("## Code ##\n"); \
      return offset + 1; \
    } break;

#define SIMPLE_INSTRUCTION2(Code) \
  case Code: \
      { \
        printf("## Code ## %d\n", hunk->code[offset + 1]); \
        return offset + 2; \
      } break;

  switch (in) {
    SIMPLE_INSTRUCTION(OP_RETURN);
    SIMPLE_INSTRUCTION(OP_NEGATE);

    SIMPLE_INSTRUCTION(OP_ADD);
    SIMPLE_INSTRUCTION(OP_SUBTRACT);
    SIMPLE_INSTRUCTION(OP_MULTIPLY);
    SIMPLE_INSTRUCTION(OP_DIVIDE);

    SIMPLE_INSTRUCTION(OP_TEST_EQ);

    SIMPLE_INSTRUCTION2(OP_SET_LOCAL);
    SIMPLE_INSTRUCTION2(OP_JUMP_IF_FALSE);

    case OP_CONSTANT:
      {
        int idx = hunk->code[offset + 1];
        printf("%s %d ", "OP_CONSTANT", idx);

        value_println(hunk->constants.values[idx]);

        return offset + 2;
      } break;
    case OP_GET_LOCAL:
      {
        int idx = hunk->code[offset + 1];
        printf("%s %d ", "OP_GET_LOCAL", idx);

        value_println(hunk->locals.values[idx]);

        return offset + 2;
      } break;
    default:
      {
        printf("Unknown opcode: %d\n", in);
      };
  }

  return offset + 1;

#undef SIMPLE_INSTRUCTION
#undef SIMPLE_INSTRUCTION2
}

void hunk_disassemble(Hunk* hunk, const char* name) {
  printf("=== %s ===\n", name);

  int i = 0;

  while (i < hunk->count) {
    i += hunk_disassembleInstruction(hunk, i);
  }
}

enum ProgramResult : uint32 {
  PROGRAM_RESULT_OK,
  PROGRAM_RESULT_COMPILE_ERROR,
  PROGRAM_RESULT_RUNTIME_ERROR
};

#define VM_STACK_MAX (256)
#define VM_LOCALS_MAX (256)
struct VM {
  Hunk* hunk;

  Value stack[VM_STACK_MAX];
  Value* stackTop;

  Instruction* ip;
};

void vm_load(VM* vm, Hunk* hunk) {
  vm->hunk = hunk;

  vm->ip = hunk->code;

  vm->stackTop = vm->stack;
}

void vm_stack_push(VM* vm, Value val) {
  *vm->stackTop = val;

  vm->stackTop += 1;
}

Value vm_stack_pop(VM* vm) {
  vm->stackTop -= 1;

  return *vm->stackTop;
}

ProgramResult vm_run(VM* vm) {
#define READ() (*vm->ip++)
  while (true) {
    int offset = (int) (vm->ip - vm->hunk->code);
    hunk_disassembleInstruction(vm->hunk, offset);

    Instruction in = READ();

    switch (in) {
      case OP_RETURN:
        {
          value_println(vm_stack_pop(vm));

          return PROGRAM_RESULT_OK;
        } break;
      case OP_CONSTANT:
        {
          Instruction id = READ();
          Value val = vm->hunk->constants.values[id];

          vm_stack_push(vm, val);
        } break;
      case OP_SET_LOCAL:
        {
          Instruction id = READ();

          // TODO(harrison): Store local variables in the VM, not in a Hunk
          vm->hunk->locals.values[id] = vm_stack_pop(vm);
        } break;
      case OP_GET_LOCAL:
        {
          Instruction id = READ();
          Value val = vm->hunk->locals.values[id];

          vm_stack_push(vm, val);
        } break;
      case OP_JUMP_IF_FALSE:
        {
          Value v = vm_stack_pop(vm);
          Instruction jumpOffset = READ();

          if (v.type == VALUE_BOOL && v.as.boolean == false) {
            vm->ip += jumpOffset;
          }
        } break;
      case OP_NEGATE:
        {
          Value v = vm_stack_pop(vm);

          if (!VALUE_IS_NUMBER(v)) {
            printf("RUNTIME ERROR: Value is not a number.");

            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          v.as.number *= -1;

          vm_stack_push(vm, v);
        } break;
      case OP_TEST_EQ:
        {
          Value b = vm_stack_pop(vm);
          Value a = vm_stack_pop(vm);

          Value c = {};
          c.type = VALUE_BOOL;

          c.as.boolean = value_equals(a, b);

          vm_stack_push(vm, c);
        } break;
#define BINARY_OP(op) \
  Value b = vm_stack_pop(vm); \
  Value a = vm_stack_pop(vm); \
  if (!VALUE_IS_NUMBER(a) || !VALUE_IS_NUMBER(b)) { \
    return PROGRAM_RESULT_RUNTIME_ERROR; \
  } \
  Value c = {}; \
  c.type = VALUE_NUMBER; \
  c.as.number = a.as.number op b.as.number; \
  vm_stack_push(vm, c);
      case OP_ADD:
        {
          BINARY_OP(+);
        } break;
      case OP_SUBTRACT:
        {
          BINARY_OP(-);
        } break;
      case OP_MULTIPLY:
        {
          BINARY_OP(*);
        } break;
      case OP_DIVIDE:
        {
          BINARY_OP(/);
        } break;
#undef BINARY_OP
      default:
        {
          // vm_setError("Unknown instruction");
          return PROGRAM_RESULT_RUNTIME_ERROR;
        } break;
    }
  }

  return PROGRAM_RESULT_OK;
#undef READ
}

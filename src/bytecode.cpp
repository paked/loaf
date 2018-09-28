typedef uint8 Instruction;

enum OPCode : Instruction {
  OP_RETURN,

  OP_SET_LOCAL,
  OP_GET_LOCAL,

  // Load constant onto stack
  OP_CONSTANT,

  // Unary negation
  OP_NEGATE,

  // a # b (where # is an arithmathic operation)
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE
};

typedef int Value;

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
#define SIMPLE_INSTRUCTION(code) \
  case code: \
    { \
      printf("%s\n", #code ); \
\
      return offset + 1; \
    } break;

  switch (in) {
    SIMPLE_INSTRUCTION(OP_RETURN);
    SIMPLE_INSTRUCTION(OP_NEGATE);

    SIMPLE_INSTRUCTION(OP_ADD);
    SIMPLE_INSTRUCTION(OP_SUBTRACT);
    SIMPLE_INSTRUCTION(OP_MULTIPLY);
    SIMPLE_INSTRUCTION(OP_DIVIDE);

    case OP_CONSTANT:
      {
        printf("%s %d\n", "OP_CONSTANT", hunk->constants.values[hunk->code[offset + 1]]);

        return offset + 2;
      } break;
    case OP_GET_LOCAL:
      {
        printf("%s %d\n", "OP_GET_LOCAL", hunk->locals.values[hunk->code[offset + 1]]);

        return offset + 2;
      } break;
    case OP_SET_LOCAL:
      {
        printf("%s %d\n", "OP_SET_LOCAL", hunk->code[offset + 1]);

        return offset + 2;
      } break;

    default:
      {
        printf("Unknown opcode: %d\n", in);
      };
  }

  return offset + 1;

#undef SIMPLE_INSTRUCTION
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
          printf("%d\n", vm_stack_pop(vm));

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
      case OP_NEGATE:
        {
          vm_stack_push(vm, -(vm_stack_pop(vm)));
        } break;

#define BINARY_OP(op) \
  Value b = vm_stack_pop(vm); \
  Value a = vm_stack_pop(vm); \
  vm_stack_push(vm, a op b);
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

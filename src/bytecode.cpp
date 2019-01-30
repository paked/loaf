// How to define a function
//
// Constants:
// 0. Value [String, "main"]
// 1. Value [Function, { x }
//
// OP_CONSTANT 0
// OP_CONSTANT 1
//
// OP_SET_GLOBAL
// -- defines function "main" as code "x". takes a string and function value
// off the stack

typedef uint16 Instruction;

enum OPCode : Instruction {
  OP_RETURN,

  OP_SET_LOCAL,
  OP_GET_LOCAL,

  OP_SET_GLOBAL,
  OP_GET_GLOBAL,

  OP_CALL,

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
  OP_TEST_GTE,   // >=
  OP_TEST_LTE,   // <=

  OP_TEST_OR,   // ||
  OP_TEST_AND,  // &&

  OP_JUMP_IF_FALSE,

  OP_LOG,
};

array_for(Instruction);
array_for(uint32);
array_for(Value);

struct Hunk {
  array(Instruction) code;
  array(uint32) lines;

  array(Value) constants;
};

void hunk_init(Hunk* hunk) {
  hunk->code = array_Instruction_init();
  hunk->lines = array_uint32_init();

  hunk->constants = array_Value_init();
}

int hunk_getCount(Hunk* hunk) {
  return (int) array_count(hunk->code);
}

bool hunk_write(Hunk* hunk, Instruction i, uint32 line) {
  array_Instruction_add(&hunk->code, i);
  array_uint32_add(&hunk->lines, line);

  return true;
}

int hunk_addConstant(Hunk* hunk, Value val) {
  array_Value_add(&hunk->constants, val);

  return ((int) array_count(hunk->constants)) - 1;
}

int hunk_disassembleInstruction(Hunk* hunk, int offset) {
  logf("%04d | %04d | ", hunk->lines[offset], offset);

  Instruction in = hunk->code[offset];

#define SIMPLE_INSTRUCTION(Code) \
  case Code: \
    { \
      logf("%s\n", #Code); \
      return offset + 1; \
    } break;

#define SIMPLE_INSTRUCTION2(Code) \
  case Code: \
      { \
        logf("%s %d\n", #Code, hunk->code[offset + 1]); \
        return offset + 2; \
      } break;

  switch (in) {
    SIMPLE_INSTRUCTION(OP_SET_GLOBAL);
    SIMPLE_INSTRUCTION(OP_GET_GLOBAL);
    SIMPLE_INSTRUCTION(OP_LOG);
    SIMPLE_INSTRUCTION(OP_NEGATE);

    SIMPLE_INSTRUCTION(OP_ADD);
    SIMPLE_INSTRUCTION(OP_SUBTRACT);
    SIMPLE_INSTRUCTION(OP_MULTIPLY);
    SIMPLE_INSTRUCTION(OP_DIVIDE);

    SIMPLE_INSTRUCTION(OP_TEST_EQ);
    SIMPLE_INSTRUCTION(OP_TEST_GT);
    SIMPLE_INSTRUCTION(OP_TEST_LT);
    SIMPLE_INSTRUCTION(OP_TEST_GTE);
    SIMPLE_INSTRUCTION(OP_TEST_LTE);

    SIMPLE_INSTRUCTION(OP_TEST_AND);
    SIMPLE_INSTRUCTION(OP_TEST_OR);

    SIMPLE_INSTRUCTION2(OP_SET_LOCAL);
    SIMPLE_INSTRUCTION2(OP_JUMP_IF_FALSE);
    SIMPLE_INSTRUCTION2(OP_CALL);
    SIMPLE_INSTRUCTION2(OP_RETURN);

    case OP_CONSTANT:
      {
        int idx = hunk->code[offset + 1];
        logf("%s %d ", "OP_CONSTANT", idx);

        value_logln(hunk->constants[idx]);

        return offset + 2;
      } break;
    case OP_GET_LOCAL:
      {
        int idx = hunk->code[offset + 1];
        logf("%s %d\n", "OP_GET_LOCAL", idx);

        return offset + 2;
      } break;
    default:
      {
        logf("Unknown opcode: %d\n", in);
      };
  }

  return offset + 1;

#undef SIMPLE_INSTRUCTION
#undef SIMPLE_INSTRUCTION2
}

void hunk_disassemble(Hunk* hunk, const char* name) {
  logf("+++ %s +++\n", name);

  int i = 0;

  while (i < hunk_getCount(hunk)) {
    i = hunk_disassembleInstruction(hunk, i);
  }

  logf("--- %s ---\n", name);
}

enum ProgramResult : uint32 {
  PROGRAM_RESULT_OK,
  PROGRAM_RESULT_COMPILE_ERROR,
  PROGRAM_RESULT_RUNTIME_ERROR
};

#define VM_STACK_MAX (256)
#define VM_FRAME_MAX (32)
#define VM_LOCALS_MAX (32)

struct Frame {
  Hunk* hunk;
  Instruction* ip;

  Value* originalStackPosition;

  Value slots[VM_LOCALS_MAX];
};

struct VM {
  Table globals;

  Value stack[VM_STACK_MAX];
  Value* stackTop;

  Frame frames[VM_FRAME_MAX];
  int frameCount;
};

void vm_load(VM* vm, Hunk* hunk) {
  table_init(&vm->globals);
  vm->stackTop = vm->stack;
  vm->frameCount = 0;

  Frame f = {};

  f.hunk = hunk;
  f.ip = f.hunk->code;
  f.originalStackPosition = vm->stackTop;

  vm->frames[vm->frameCount] = f;
  vm->frameCount += 1;
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
#define READ() (*frame->ip++)
  while (vm->frameCount > 0) {
    Frame* frame = &vm->frames[vm->frameCount -1];

#ifdef DEBUG
    for (int i = 0; i < VM_LOCALS_MAX; i++) {
      if (frame->slots[i].type == VALUE_NIL) {
        continue;
      }

      logf("slot %d: ", i);
      value_logln(frame->slots[i]);
    }

    for (Value* v = vm->stack; v != (vm->stackTop); v += 1) {
      logf("stack: ");
      value_logln(*v);
    }

    int offset = (int) (frame->ip - frame->hunk->code);
    hunk_disassembleInstruction(frame->hunk, offset);
#endif

    Instruction in = READ();

    switch (in) {
      case OP_RETURN:
        {
          int amount = (int) READ();

          Value ret = {};

          if (amount != 0) {
            ret = vm_stack_pop(vm);
          }

          vm->stackTop = frame->originalStackPosition;

          if (amount != 0) {
            vm_stack_push(vm, ret);
          }

          vm->frameCount -= 1;
        } break;
      case OP_CONSTANT:
        {
          Instruction id = READ();
          Value val = frame->hunk->constants[id];

          vm_stack_push(vm, val);
        } break;
      case OP_SET_LOCAL:
        {
          Instruction id = READ();

          frame->slots[id] = vm_stack_pop(vm);
        } break;
      case OP_GET_LOCAL:
        {
          Instruction id = READ();
          Value val = frame->slots[id];

          vm_stack_push(vm, val);
        } break;
      case OP_SET_GLOBAL:
        {
          Value func = vm_stack_pop(vm);
          Value name = vm_stack_pop(vm);

          if (name.type != VALUE_STRING) {
            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          table_set(&vm->globals, name.as.string, func);
        } break;
      case OP_GET_GLOBAL:
        {
          Value name = vm_stack_pop(vm);

          if (name.type != VALUE_STRING) {
            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          Value func;

          if (!table_get(&vm->globals, name.as.string, &func)) {
            logf("ERROR: unknown function\n");

            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          vm_stack_push(vm, func);

          value_logln(func);
        } break;
      case OP_CALL:
        {
          Value func = vm_stack_pop(vm);
          int arity = (int) READ();

          if (func.type != VALUE_FUNCTION) {
            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          if (vm->frameCount >= VM_FRAME_MAX - 1) {
            logf("ERROR: too many frames\n");

            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          Frame f = {};

          for (int i = arity - 1; i >= 0; i--) {
            Value param = vm_stack_pop(vm);

            f.slots[i] = param;
          }

          Hunk* newHunk = func.as.function.hunk;

          f.hunk = newHunk;
          f.ip = f.hunk->code;
          f.originalStackPosition = vm->stackTop;

          vm->frames[vm->frameCount] = f;
          vm->frameCount += 1;
        } break;
      case OP_JUMP_IF_FALSE:
        {
          Value v = vm_stack_pop(vm);
          Instruction jumpOffset = READ();

          if (v.type == VALUE_BOOL && v.as.boolean == false) {
            frame->ip += jumpOffset;
          }
        } break;
      case OP_NEGATE:
        {
          Value v = vm_stack_pop(vm);

          if (!VALUE_IS_NUMBER(v)) {
            logf("RUNTIME ERROR: Value is not a number.");

            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          v.as.number *= -1;

          vm_stack_push(vm, v);
        } break;
      case OP_LOG:
        {
          Value v = vm_stack_pop(vm);

          value_println(v);

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
#define COMPARE(Name, Op) \
      case Name: \
        { \
          Value b = vm_stack_pop(vm); \
          Value a = vm_stack_pop(vm); \
          if (!VALUE_IS_NUMBER(a) || !VALUE_IS_NUMBER(b)) { \
            return PROGRAM_RESULT_RUNTIME_ERROR; \
          } \
          Value c = {}; \
          c.type = VALUE_BOOL; \
          c.as.boolean = a.as.number Op b.as.number; \
          vm_stack_push(vm, c); \
        } break;
      COMPARE(OP_TEST_LT, <)
      COMPARE(OP_TEST_LTE, <=)
      COMPARE(OP_TEST_GT, >)
      COMPARE(OP_TEST_GTE, >=)
#undef COMPARE
      case OP_TEST_AND:
        {
          Value b = vm_stack_pop(vm);
          Value a = vm_stack_pop(vm);

          if (!VALUE_IS_BOOL(a) || !VALUE_IS_BOOL(b)) {
            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          Value c = {};
          c.type = VALUE_BOOL;

          c.as.boolean = a.as.boolean && b.as.boolean;

          vm_stack_push(vm, c);
        } break;
      case OP_TEST_OR:
        {
          Value b = vm_stack_pop(vm);
          Value a = vm_stack_pop(vm);

          if (!VALUE_IS_BOOL(a) || !VALUE_IS_BOOL(b)) {
            return PROGRAM_RESULT_RUNTIME_ERROR;
          }

          Value c = {};
          c.type = VALUE_BOOL;

          c.as.boolean = a.as.boolean || b.as.boolean;

          vm_stack_push(vm, c);
        } break;
#define BINARY_OP(op) \
  Value b = vm_stack_pop(vm); \
  Value a = vm_stack_pop(vm); \
  if (!VALUE_IS_NUMBER(a) || !VALUE_IS_NUMBER(b)) { \
    logf("INVALID BINARY OPERATION: '%s'\n", #op); \
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
          logf("Unknown instruction. Exiting...\n");

          return PROGRAM_RESULT_RUNTIME_ERROR;
        } break;
    }
  }

  return PROGRAM_RESULT_OK;
#undef READ
}

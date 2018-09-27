#include <stdlib.h>
#include <stdio.h>

#include <us.hpp>

#define REALLOC(Type, ptr, count) ((Type*) realloc(ptr, count * sizeof(Type)))

enum ProgramResult : uint32 {
  PROGRAM_RESULT_OK,
  PROGRAM_RESULT_COMPILE_ERROR,
  PROGRAM_RESULT_RUNTIME_ERROR
};

typedef uint8 Instruction;

enum OPCode : Instruction {
  OP_RETURN,
  OP_CONSTANT
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
  uint8* code;
  uint32* lines;

  Constants constants;
};

bool hunk_write(Hunk* hunk, Instruction in, uint32 line) {
  if (hunk->capacity < hunk->count + 1) {
    hunk->capacity = HUNK_CAPACITY_GROW(hunk->capacity);

    hunk->code = REALLOC(uint8, hunk->code, hunk->capacity);
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

int hunk_disassembleInstruction(Hunk* hunk, int offset) {
  printf("%04d | %04d | ", hunk->lines[offset], offset);

  uint8 in = hunk->code[offset];
#define SIMPLE_INSTRUCTION(code) \
  case code: \
    { \
      printf("%s\n", #code ); \
\
      return offset + 1; \
    } break; \

  switch (in) {
    SIMPLE_INSTRUCTION(OP_RETURN);

    case OP_CONSTANT:
      {
        printf("%s %d\n", "OP_CONSTANT", hunk->constants.values[hunk->code[offset + 1]]);

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

int main(int argc, char** argv) {
  Hunk hunk = {0};

  if (!hunk_write(&hunk, OP_CONSTANT, 0)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }

  int constant = hunk_addConstant(&hunk, 22);
  if (constant == -1) {
    printf("ERROR: could not add constant\n");

    return -1;
  }

  if (!hunk_write(&hunk, constant, 0)) {
    printf("ERROR: could not write chunk data\n");

    return -1;
  }

  if (!hunk_write(&hunk, OP_RETURN, 1)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }

  hunk_disassemble(&hunk, "main");

  return 0;
}

/*
int main(int argc, char** argv) {
  Program program = {0};
  Hunk hunk = {0};

  Instruction constant = program_addConstant(&program, 22);

  hunk_write(&hunk, OP_CONSTANT);
  hunk_write(&hunk, constant);

  ProgramResult res = program_execute(&program, &hunk);

  if (res != PROGRAM_RESULT_OK) {
    // printf("%s\n", program_getError(&program));

    return 1;
  }

  printf("Program finished executing...\n");

  return 0;
}*/

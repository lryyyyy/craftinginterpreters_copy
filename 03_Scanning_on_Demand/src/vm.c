#include "vm.h"
#include "common.h"
#include "debug.h"

VM vm;

static void ResetStack() { vm.stackTop = vm.stack; }

void InitVM() { ResetStack(); }

void FreeVM() {}

void Push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value Pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static InterpretResult Run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)                                                          \
  do {                                                                         \
    double b = Pop();                                                          \
    double a = Pop();                                                          \
    Push(a op b);                                                              \
  } while (false)

  while (true) {
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      Push(constant);
      break;
    }
    case OP_ADD: {
      BINARY_OP(+);
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(-);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(*);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(/);
      break;
    }
    case OP_NEGATE: {
      Push(-Pop());
      break;
    }
    case OP_RETURN: {
      Value result = Pop();
      printf("%f\n", result);
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult Interpret(const char *source) {
  Compile(source);
  return INTERPRET_OK;
}
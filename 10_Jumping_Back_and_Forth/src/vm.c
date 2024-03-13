#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "value.h"

VM vm;

static void ResetStack() { vm.stackTop = vm.stack; }

static void RuntimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  ResetStack();
}

void InitVM() {
  ResetStack();
  vm.objects = NULL;
  InitTable(&vm.strings);
  InitTable(&vm.globals);
}

void FreeVM() {
  FreeTable(&vm.strings);
  FreeTable(&vm.globals);
  FreeObjects();
}

void Push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value Pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static Value Peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool Not(Value value) {
  return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void Concatenate() {
  ObjString *str2 = AS_STRING(Pop());
  ObjString *str1 = AS_STRING(Pop());

  int length = str1->length + str2->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, str1->chars, str1->length);
  memcpy(chars + str1->length, str2->chars, str2->length);
  chars[length] = '\0';

  ObjString *result = GetString(chars, length);
  Push(OBJ_VAL(result));
}

static InterpretResult Run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(type, op)                                                    \
  do {                                                                         \
    if (!IS_NUMBER(Peek(0)) || !IS_NUMBER(Peek(1))) {                          \
      RuntimeError("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(Pop());                                               \
    double a = AS_NUMBER(Pop());                                               \
    Push(type(a op b));                                                        \
  } while (false)

  while (true) {
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      Push(constant);
      break;
    }
    case OP_NULL: {
      Push(NULL_VAL);
      break;
    }
    case OP_TRUE: {
      Push(BOOL_VAL(true));
      break;
    }
    case OP_FALSE: {
      Push(BOOL_VAL(false));
      break;
    }
    case OP_POP: {
      Pop();
      break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      TableSet(&vm.globals, name, Peek(0));
      Pop();
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!TableGet(&vm.globals, name, &value)) {
        RuntimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      Push(value);
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (TableSet(&vm.globals, name, Peek(0))) {
        TableDelete(&vm.globals, name);
        RuntimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      Push(vm.stack[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      vm.stack[slot] = Peek(0);
      break;
    }
    case OP_EQUAL: {
      Value b = Pop();
      Value a = Pop();
      Push(BOOL_VAL(ValueEqual(a, b)));
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_ADD: {
      if (IS_STRING(Peek(0)) && IS_STRING(Peek(1))) {
        Concatenate();
      } else if (IS_NUMBER(Peek(0)) && IS_NUMBER(Peek(1))) {
        double b = AS_NUMBER(Pop());
        double a = AS_NUMBER(Pop());
        Push(NUMBER_VAL(a + b));
      } else {
        RuntimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_NOT: {
      if (!IS_BOOL(Peek(0))) {
        RuntimeError("Operand must be a boolean.");
        return INTERPRET_RUNTIME_ERROR;
      }
      Push(BOOL_VAL(Not(Pop())));
      break;
    }
    case OP_NEGATE: {
      if (!IS_NUMBER(Peek(0))) {
        RuntimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      Push(NUMBER_VAL(-AS_NUMBER(Pop())));
      break;
    }
    case OP_PRINT: {
      Value result = Pop();
      PrintValue(result);
      printf("\n");
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (Not(Peek(0))) {
        vm.ip += offset;
      }
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      vm.ip += offset;
      break;
    }
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult Interpret(const char *source) {
  Chunk chunk;
  InitChunk(&chunk);

  if (!Compile(source, &chunk)) {
    FreeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = Run();

  FreeChunk(&chunk);
  return result;
}
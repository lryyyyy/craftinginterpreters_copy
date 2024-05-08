#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "value.h"

VM vm;

static void ResetStack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
  vm.open_upvalues = NULL;
}

static void RuntimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  for (int i = vm.frame_count - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
  ResetStack();
}

static Value ClockNative(int arg_count, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void DefineNative(const char* name, NativeFn function) {
  Push(OBJ_VAL(CopyString(name, (int)strlen(name))));
  Push(OBJ_VAL(NewNative(function)));
  TableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  Pop();
  Pop();
}

void InitVM() {
  ResetStack();
  vm.objects = NULL;
  InitTable(&vm.strings);
  InitTable(&vm.globals);
  DefineNative("clock", ClockNative);
}

void FreeVM() {
  FreeTable(&vm.strings);
  FreeTable(&vm.globals);
  FreeObjects();
}

void Push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value Pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

static Value Peek(int distance) { return vm.stack_top[-1 - distance]; }

static bool Call(ObjClosure* closure, int arg_count) {
  if (arg_count != closure->function->arity) {
    RuntimeError("Expected %d arguments but got %d.", closure->function->arity,
                 arg_count);
    return false;
  }
  if (vm.frame_count == FRAME_MAX) {
    RuntimeError("Stack overflow.");
    return false;
  }
  CallFrame *frame = &vm.frames[vm.frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack_top - arg_count - 1;
  return true;
}

static bool CallValue(Value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_CLOSURE:
      return Call(AS_CLOSURE(callee), arg_count);
    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(arg_count, vm.stack_top - arg_count);
      vm.stack_top -= arg_count + 1;
      Push(result);
      return true;
    }
    default:
      break; // Non-callable object type.
    }
  }
  RuntimeError("Can only call functions and classes.");
  return false;
}

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

static ObjUpvalue* CaptureUpvalue(Value* local) {
  ObjUpvalue *prev_upvalue = NULL;
  ObjUpvalue *upvalue = vm.open_upvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next;
  }
  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }
  ObjUpvalue* created_upvalue = NewUpvalue(local);
  created_upvalue->next = upvalue;
  if (prev_upvalue == NULL) {
    vm.open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }
  return created_upvalue;
}

static void CloseUpvalues(Value* last) {
  while (vm.open_upvalues != NULL &&
         vm.open_upvalues->location >= last) {
    ObjUpvalue* upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

static InterpretResult Run() {
  CallFrame *frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
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
      Push(frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = Peek(0);
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      Push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = Peek(0);
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
        frame->ip += offset;
      }
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }
    case OP_CALL: {
      int arg_count = READ_BYTE();
      if (!CallValue(Peek(arg_count), arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }
    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = NewClosure(function);
      Push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalue_count; i++) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (is_local) {
          closure->upvalues[i] =
              CaptureUpvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_CLOSE_UPVALUE: {
      CloseUpvalues(vm.stack_top - 1);
      Pop();
      break;
    }
    case OP_RETURN: {
      Value result = Pop();
      CloseUpvalues(frame->slots);
      vm.frame_count--;
      if (vm.frame_count == 0) {
        Pop();
        return INTERPRET_OK;
      }
      vm.stack_top = frame->slots;
      Push(result);
      frame = &vm.frames[vm.frame_count - 1];
      break;
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
  ObjFunction *function = Compile(source);
  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }
  Push(OBJ_VAL(function));
  ObjClosure* closure = NewClosure(function);
  Pop();
  Push(OBJ_VAL(closure));
  Call(closure, 0);
  return Run();
}

#include "object.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)AllocateObject(sizeof(type), objectType)

static Object *AllocateObject(size_t size, ObjType type) {
  Object *object = (Object *)reallocate(NULL, 0, size);
  object->type = type;
  object->next = vm.objects;
  vm.objects = object;
  return object;
}

ObjFunction *NewFunction() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->name = NULL;
  InitChunk(&function->chunk);
  return function;
}

ObjClosure *NewClosure(ObjFunction *function) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *,
                                   function->upvalue_count);
  for (int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

ObjNative *NewNative(NativeFn function) {
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjString *AllocateString(char *chars, int length, uint32_t hash) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  TableSet(&vm.strings, string, NULL_VAL);
  return string;
}

static uint32_t HashString(const char *key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *GetString(char *chars, int length) {
  uint32_t hash = HashString(chars, length);
  ObjString *interned = TableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  return AllocateString(chars, length, hash);
}

ObjString *CopyString(const char *chars, int length) {
  uint32_t hash = HashString(chars, length);
  ObjString *interned = TableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }
  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return AllocateString(heap_chars, length, hash);
}

ObjUpvalue *NewUpvalue(Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NULL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void PrintFunction(ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void PrintObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  case OBJ_FUNCTION:
    PrintFunction(AS_FUNCTION(value));
    break;
  case OBJ_CLOSURE:
    PrintFunction(AS_CLOSURE(value)->function);
    break;
  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  }
}

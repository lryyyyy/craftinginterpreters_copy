#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

void *reallocate(void *pointer, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }
  void *result = realloc(pointer, new_size);
  if (result == NULL) {
    exit(1);
  }
  return result;
}

static void FreeObject(Object *object) {
  switch (object->type) {
  case OBJ_STRING: {
    ObjString *string = (ObjString *)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjString, object);
    break;
  }
  }
}

void FreeObjects() {
  Object *object = vm.objects;
  while (object != NULL) {
    Object *next = object->next;
    FreeObject(object);
    object = next;
  }
}
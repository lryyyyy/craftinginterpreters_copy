#include "value.h"
#include "memory.h"

void InitValueArray(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}
void WriteValueArray(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values =
        GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}
void FreeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  InitValueArray(array);
}
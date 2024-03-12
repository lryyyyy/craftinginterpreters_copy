#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

#define REPL_MAX 1024

static void Repl() {
  char line[REPL_MAX];
  while (true) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    Interpret(line);
  }
}

static char *ReadFile(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

static void RunFile(const char *path) {
  char *source = ReadFile(path);
  InterpretResult result = Interpret(source);
  free(source);
  if (result == INTERPRET_COMPILE_ERROR)
    exit(65);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(70);
}

int main(int argc, const char *argv[]) {
  InitVM();
  if (argc == 1) {
    Repl();
  } else if (argc == 2) {
    RunFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }
  FreeVM();
  return 0;
}

#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

typedef struct {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

typedef struct {
  Token name;
  int depth;
  bool is_captured;
} Local;

typedef struct {
  uint8_t index;
  bool is_local;
} Upvalue;

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT,
} FunctionType;

typedef struct Compiler {
  struct Compiler *enclosing;
  ObjFunction *function;
  FunctionType type;
  Local locals[UINT8_COUNT];
  int local_count;
  Upvalue upvalues[UINT8_COUNT];
  int scope_depth;
} Compiler;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static void Grouping(bool can_assign);
static void Unary(bool can_assign);
static void Number(bool can_assign);
static void String(bool can_assign);
static void Literal(bool can_assign);
static void Binary(bool can_assign);
static void Call(bool can_assign);
static void Variable(bool can_assign);
static void And_(bool can_assign);
static void Or_(bool can_assign);
static void ParsePrecedence(Precedence precedence);
static void Declaration();
static void VarDeclaration();
static void Statement();
static void DefineVariable(uint8_t global);

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {Grouping, Call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {Unary, Binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, Binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, Binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, Binary, PREC_FACTOR},
    [TOKEN_BANG] = {Unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, Binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, Binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, Binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, Binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, Binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, Binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {Variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {String, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {Number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, And_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {Literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NULL] = {Literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, Or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {Literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *GetRule(TokenType type) { return &rules[type]; }

Parser parser;
Compiler *current = NULL;
Chunk *compiling_chunk;

static void InitCompiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  compiler->function = NewFunction();
  current = compiler;
  if (type != TYPE_SCRIPT) {
    current->function->name =
        CopyString(parser.previous.start, parser.previous.length);
  }
  Local *local = &current->locals[current->local_count++];
  local->depth = 0;
  local->is_captured = false;
  local->name.start = "";
  local->name.length = 0;
}

static Chunk *CurrentChunk() { return &current->function->chunk; }

static void ErrorAt(Token *token, const char *message) {
  if (parser.panic_mode)
    return;
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void Error(const char *message) { ErrorAt(&parser.previous, message); }

static void ErrorAtCurrent(const char *message) {
  ErrorAt(&parser.current, message);
}

static void Advance() {
  parser.previous = parser.current;

  while (true) {
    parser.current = ScanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    ErrorAtCurrent(parser.current.start);
  }
}

static void Consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    Advance();
    return;
  }

  ErrorAtCurrent(message);
}

static bool Check(TokenType type) { return parser.current.type == type; }

static bool Match(TokenType type) {
  if (!Check(type)) {
    return false;
  }
  Advance();
  return true;
}

static void EmitByte(uint8_t byte) {
  WriteChunk(CurrentChunk(), byte, parser.previous.line);
}

static void EmitBytes(uint8_t byte1, uint8_t byte2) {
  EmitByte(byte1);
  EmitByte(byte2);
}

static void EmitReturn() {
  EmitByte(OP_NULL);
  EmitByte(OP_RETURN);
}

static ObjFunction *EndCompiler() {
  EmitReturn();
  ObjFunction *function = current->function;
  if (!parser.had_error) {
    DisassembleChunk(CurrentChunk(), function->name != NULL
                                         ? function->name->chars
                                         : "<script>");
  }
  current = current->enclosing;
  return function;
}

static void Expression() { ParsePrecedence(PREC_ASSIGNMENT); }

static void ExpressionStatement() {
  Expression();
  Consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  EmitByte(OP_POP);
}

static void PrintStatement() {
  Expression();
  Consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  EmitByte(OP_PRINT);
}

static void Synchronize() {
  parser.panic_mode = false;
  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:;
    }
    Advance();
  }
}

static void Block() {
  while (!Check(TOKEN_RIGHT_BRACE) && !Check(TOKEN_EOF)) {
    Declaration();
  }
  Consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void BeginScope() { current->scope_depth++; }

static void EndScope() {
  current->scope_depth--;
  while (current->local_count > 0 &&
         current->locals[current->local_count - 1].depth >
             current->scope_depth) {
    if (current->locals[current->local_count - 1].is_captured) {
      EmitByte(OP_CLOSE_UPVALUE);
    } else {
      EmitByte(OP_POP);
    }
    current->local_count--;
  }
}

static int EmitJump(uint8_t instruction) {
  EmitByte(instruction);
  EmitByte(0xff);
  EmitByte(0xff);
  return CurrentChunk()->count - 2;
}

static void PatchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = CurrentChunk()->count - offset - 2;
  if (jump > UINT16_MAX) {
    Error("Too much code to jump over.");
  }
  CurrentChunk()->code[offset] = (jump >> 8) & 0xff;
  CurrentChunk()->code[offset + 1] = jump & 0xff;
}

static void IfStatement() {
  Consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  Expression();
  Consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  int then_jump = EmitJump(OP_JUMP_IF_FALSE);
  EmitByte(OP_POP);
  Statement();
  int else_jump = EmitJump(OP_JUMP);
  PatchJump(then_jump);
  EmitByte(OP_POP);
  if (Match(TOKEN_ELSE)) {
    Statement();
  }
  PatchJump(else_jump);
}

static void EmitLoop(int loop_start) {
  EmitByte(OP_LOOP);
  int offset = CurrentChunk()->count - loop_start + 2;
  if (offset > UINT16_MAX) {
    Error("Loop body too large.");
  }
  EmitByte((offset >> 8) & 0xff);
  EmitByte(offset & 0xff);
}

static void WhileStatement() {
  int loop_start = CurrentChunk()->count;
  Consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  Expression();
  Consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
  int exit_jump = EmitJump(OP_JUMP_IF_FALSE);
  EmitByte(OP_POP);
  Statement();
  EmitLoop(loop_start);
  PatchJump(exit_jump);
  EmitByte(OP_POP);
}

static void ForStatement() {
  BeginScope();
  Consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (Match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (Match(TOKEN_VAR)) {
    VarDeclaration();
  } else {
    ExpressionStatement();
  }
  int loop_start = CurrentChunk()->count;
  int exit_jump = -1;
  if (!Match(TOKEN_SEMICOLON)) {
    Expression();
    Consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
    exit_jump = EmitJump(OP_JUMP_IF_FALSE);
    EmitByte(OP_POP);
  }
  if (!Match(TOKEN_RIGHT_PAREN)) {
    int body_jump = EmitJump(OP_JUMP);
    int increment_start = CurrentChunk()->count;
    Expression();
    EmitByte(OP_POP);
    Consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
    EmitLoop(loop_start);
    loop_start = increment_start;
    PatchJump(body_jump);
  }
  Statement();
  EmitLoop(loop_start);
  if (exit_jump != -1) {
    PatchJump(exit_jump);
    EmitByte(OP_POP);
  }
  EndScope();
}

static void ReturnStatement() {
  if (current->type == TYPE_SCRIPT) {
    Error("Can't return from top-level code.");
  }
  if (Match(TOKEN_SEMICOLON)) {
    EmitReturn();
  } else {
    Expression();
    Consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    EmitByte(OP_RETURN);
  }
}

static void Statement() {
  if (Match(TOKEN_PRINT)) {
    PrintStatement();
  } else if (Match(TOKEN_RETURN)) {
    ReturnStatement();
  } else if (Match(TOKEN_IF)) {
    IfStatement();
  } else if (Match(TOKEN_WHILE)) {
    WhileStatement();
  } else if (Match(TOKEN_FOR)) {
    ForStatement();
  } else if (Match(TOKEN_LEFT_BRACE)) {
    BeginScope();
    Block();
    EndScope();
  } else {
    ExpressionStatement();
  }
}

static uint8_t MakeConstant(Value value) {
  int constant = AddConstant(CurrentChunk(), value);
  if (constant > UINT8_MAX) {
    Error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

static uint8_t IdentifierConstant(Token *name) {
  return MakeConstant(OBJ_VAL(CopyString(name->start, name->length)));
}

static void AddLocal(Token name) {
  if (current->local_count == UINT8_COUNT) {
    Error("Too many local variables in function.");
    return;
  }
  Local *local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = -1;
  local->is_captured = false;
}

static bool IdentifiersEqual(Token *a, Token *b) {
  if (a->length != b->length) {
    return false;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

static void DeclareVariable() {
  if (current->scope_depth == 0) {
    return;
  }
  Token *name = &parser.previous;
  for (int i = current->local_count - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scope_depth) {
      break;
    }
    if (IdentifiersEqual(name, &local->name)) {
      Error("Already a variable with this name in this scope.");
    }
  }
  AddLocal(*name);
}

static uint8_t ParseVariable(const char *error_message) {
  Consume(TOKEN_IDENTIFIER, error_message);
  DeclareVariable();
  if (current->scope_depth > 0) {
    return 0;
  }
  return IdentifierConstant(&parser.previous);
}

static void MarkInitialized() {
  if (current->scope_depth == 0) {
    return;
  }
  current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void Function(FunctionType type) {
  Compiler compiler;
  InitCompiler(&compiler, type);
  BeginScope();
  Consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!Check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        ErrorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = ParseVariable("Expect parameter name.");
      DefineVariable(constant);
    } while (Match(TOKEN_COMMA));
  }
  Consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  Consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  Block();
  ObjFunction *function = EndCompiler();
  EmitBytes(OP_CLOSURE, MakeConstant(OBJ_VAL(function)));
  for (int i = 0; i < function->upvalue_count; i++) {
    EmitByte(compiler.upvalues[i].is_local ? 1 : 0);
    EmitByte(compiler.upvalues[i].index);
  }
  EmitBytes(OP_CONSTANT, MakeConstant(OBJ_VAL(function)));
}

static void DefineVariable(uint8_t global) {
  if (current->scope_depth > 0) {
    MarkInitialized();
    return;
  }
  EmitBytes(OP_DEFINE_GLOBAL, global);
}

static void VarDeclaration() {
  uint8_t global = ParseVariable("Expect variable name.");
  if (Match(TOKEN_EQUAL)) {
    Expression();
  } else {
    EmitByte(OP_NULL);
  }
  Consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  DefineVariable(global);
}

static void FunDeclaration() {
  uint8_t global = ParseVariable("Expect function name.");
  MarkInitialized();
  Function(TYPE_FUNCTION);
  DefineVariable(global);
}

static void Declaration() {
  if (Match(TOKEN_VAR)) {
    VarDeclaration();
  } else if (Match(TOKEN_FUN)) {
    FunDeclaration();
  } else {
    Statement();
  }
  if (parser.panic_mode)
    Synchronize();
}

static void Grouping(bool can_assign) {
  Expression();
  Consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void EmitConstant(Value value) {
  EmitBytes(OP_CONSTANT, MakeConstant(value));
}

static void Literal(bool can_assign) {
  switch (parser.previous.type) {
  case TOKEN_FALSE:
    EmitByte(OP_FALSE);
    break;
  case TOKEN_NULL:
    EmitByte(OP_NULL);
    break;
  case TOKEN_TRUE:
    EmitByte(OP_TRUE);
    break;
  default:
    return;
  }
}

static void Number(bool can_assign) {
  double value = strtod(parser.previous.start, NULL);
  EmitConstant(NUMBER_VAL(value));
}

static void String(bool can_assign) {
  EmitConstant(OBJ_VAL(
      CopyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static int ResolveLocal(Compiler *compiler, Token *name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (IdentifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        Error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

static int AddUpvalue(Compiler *compiler, uint8_t index,
                      bool is_local) {
  int upvalue_count = compiler->function->upvalue_count;
  for (int i = 0; i < upvalue_count; i++) {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  }
  if (upvalue_count == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }
  compiler->upvalues[upvalue_count].is_local = is_local;
  compiler->upvalues[upvalue_count].index = index;
  return compiler->function->upvalue_count++;
}

static int ResolveUpvalue(Compiler *compiler, Token *name) {
  if (compiler->enclosing == NULL)
    return -1;

  int local = ResolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].is_captured = true;
    return AddUpvalue(compiler, (uint8_t)local, true);
  }
  int upvalue = ResolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return AddUpvalue(compiler, (uint8_t)upvalue, false);
  }
  return -1;
}

static void NamedVariable(Token name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = ResolveLocal(current, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else if ((arg = ResolveUpvalue(current, &name)) != -1) {
    get_op = OP_GET_UPVALUE;
    set_op = OP_SET_UPVALUE;
  } else {
    arg = IdentifierConstant(&name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }
  if (can_assign && Match(TOKEN_EQUAL)) {
    Expression();
    EmitBytes(set_op, (uint8_t)arg);
  } else {
    EmitBytes(get_op, (uint8_t)arg);
  }
}

static void And_(bool can_assign) {
  int end_jump = EmitJump(OP_JUMP_IF_FALSE);
  EmitByte(OP_POP);
  ParsePrecedence(PREC_AND);
  PatchJump(end_jump);
}

static void Or_(bool can_assign) {
  int else_jump = EmitJump(OP_JUMP_IF_FALSE);
  int end_jump = EmitJump(OP_JUMP);
  PatchJump(else_jump);
  EmitByte(OP_POP);
  ParsePrecedence(PREC_OR);
  PatchJump(end_jump);
}

static void Variable(bool can_assign) {
  NamedVariable(parser.previous, can_assign);
}

static void Unary(bool can_assign) {
  TokenType operator_type = parser.previous.type;
  ParsePrecedence(PREC_UNARY);
  switch (operator_type) {
  case TOKEN_MINUS:
    EmitByte(OP_NEGATE);
    break;
  case TOKEN_BANG:
    EmitByte(OP_NOT);
    break;
  default:
    return;
  }
}

static void Binary(bool can_assign) {
  TokenType operator_type = parser.previous.type;
  ParseRule *rule = GetRule(operator_type);
  ParsePrecedence((Precedence)(rule->precedence + 1));

  switch (operator_type) {
  case TOKEN_BANG_EQUAL:
    EmitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    EmitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    EmitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    EmitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    EmitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    EmitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    EmitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    EmitByte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    EmitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    EmitByte(OP_DIVIDE);
    break;
  default:
    return;
  }
}

static uint8_t ArgumentList() {
  uint8_t arg_count = 0;
  if (!Check(TOKEN_RIGHT_PAREN)) {
    do {
      Expression();
      if (arg_count == 255) {
        Error("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while (Match(TOKEN_COMMA));
  }
  Consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return arg_count;
}

static void Call(bool canAssign) {
  uint8_t arg_count = ArgumentList();
  EmitBytes(OP_CALL, arg_count);
}

static void ParsePrecedence(Precedence precedence) {
  Advance();
  ParseFn prefix_rule = GetRule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    Error("Expect expression.");
    return;
  }
  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);
  while (precedence <= GetRule(parser.current.type)->precedence) {
    Advance();
    ParseFn infix_rule = GetRule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }
  if (can_assign && Match(TOKEN_EQUAL)) {
    Error("Invalid assignment target.");
  }
}

ObjFunction *Compile(const char *source) {
  parser.had_error = false;
  parser.panic_mode = false;
  InitScanner(source);
  Compiler compiler;
  InitCompiler(&compiler, TYPE_SCRIPT);
  Advance();
  while (!Match(TOKEN_EOF)) {
    Declaration();
  }
  ObjFunction *function = EndCompiler();
  return parser.had_error ? NULL : function;
}

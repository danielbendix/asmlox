#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

#include "ops.h"
#include "native/generate.h"

// Temporary solution to making code executable
#include <sys/mman.h>
// Temporary to handle unconverted code paths
#include <assert.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction *function;
    FunctionType type;

    Local arguments[UINT8_COUNT];
    int argumentCount;
    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler *enclosing;
    bool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler *current = NULL;
ClassCompiler *currentClass = NULL;

struct {
    int capacity;
    int count;
    ObjFunction **functions;
    size_t combinedSize;
} compiledFunctions = {0, 0, NULL, 0};

static void pushCompiledFunction(ObjFunction *function)
{
    if (compiledFunctions.capacity < compiledFunctions.count + 1) {
        int oldCapacity = compiledFunctions.capacity;
        compiledFunctions.capacity = GROW_CAPACITY(oldCapacity);
        compiledFunctions.functions = GROW_ARRAY(ObjFunction *, compiledFunctions.functions, oldCapacity, compiledFunctions.capacity);
    }

    compiledFunctions.functions[compiledFunctions.count] = function;
    compiledFunctions.count++;
    compiledFunctions.combinedSize += function->chunk.count * sizeof(typeof(*function->chunk.code));
}

static void freeCompiledFunctions()
{
    if (compiledFunctions.functions) {
        free(compiledFunctions.functions);
        compiledFunctions.count = 0;
        compiledFunctions.capacity = 0;
        compiledFunctions.functions = NULL;
    }
}

Chunk *compilingChunk;

static Chunk *currentChunk()
{
    return &current->function->chunk;
}

#define CURRENT currentChunk(), parser.previous.line

static void errorAt(Token *token, const char *message)
{
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message)
{
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

static void emitByte(uint8_t byte)
{
    assert(false && "Attempting to emit lox bytecode");
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    assert(false && "Attempting to emit lox bytecode");
}

static void emitLoop_(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitByte((offset >> 8) & 0xFF);
    emitByte(offset & 0xFF);
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xFF);
    emitByte(0xFF);
    return currentChunk()->count - 2;
}

static void emitReturn_()
{
    if (current->type == TYPE_INITIALIZER) {
        emitGetArgument(CURRENT, current->argumentCount, 0);
        //emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitNil(CURRENT);
        //emitByte(OP_NIL);
    }

    if (current->type == TYPE_SCRIPT) {
        emitReturnFromScript(CURRENT);
    } else if (current->type == TYPE_INITIALIZER) {
        emitReturn(CURRENT, LOX_OP_RETURN_INITIALIZER);
    } else {
        emitReturn(CURRENT, LOX_OP_RETURN);
    }
}

static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value)
{
    emitGetConstant(CURRENT, makeConstant(value));
}

static void patchJump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;
    
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xFF;
    currentChunk()->code[offset + 1] = jump & 0xFF;
}

static void initCompiler(Compiler *compiler, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->argumentCount = 0;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    // We may need to do something about this.
    Local *local = &current->arguments[current->argumentCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction *endCompiler()
{
    emitReturn_();
    ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    pushCompiledFunction(function);

    // Then set extent of JIT code.

    // Kludge start
    void *space = mmap(NULL, sizeof(uint32_t) * function->chunk.count, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    //printf("%s\n", function->name != NULL ? function->name->chars : "main");
    char path[100] = "dumps/";
    strcpy(path + 6, function->name != NULL ? function->name->chars : "main");
    FILE *f = fopen(path, "wb");
    fwrite(function->chunk.code, sizeof(uint32_t), function->chunk.count, f);
    fclose(f);

    memcpy(space, function->chunk.code, sizeof(uint32_t) * function->chunk.count);
    reallocate(function->chunk.code, sizeof(uint32_t) * function->chunk.capacity, 0);
    mprotect(space, sizeof(uint32_t) * function->chunk.count, PROT_EXEC);
    function->chunk.code = space;
    // Kludge end

    current = current->enclosing;
    return function;
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->scopeDepth--;

    // TODO: For a scope, we could figure out the upvalue indices, and inject instructions to close them by index,
    // and then add to the stack pointer in one operation.
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitPop(CURRENT, 1);
        }
        current->localCount--;
    }
}

static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

typedef struct {
    int index;
    bool isArgument;
} LocalIndex;

static inline LocalIndex resolveLocalFromArray(Token *name, Local locals[], int count, bool isArgument)
{   
    for (int i = count - 1; i >= 0; i--) {
        Local *local = &locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return (LocalIndex) {.index = i, .isArgument = isArgument};
        }
    }
    return (LocalIndex) {.index = -1 };
}

static LocalIndex resolveLocal(Compiler *compiler, Token *name)
{
    LocalIndex index;
    index = resolveLocalFromArray(name, compiler->locals, compiler->localCount, false);
    if (index.index != -1) return index;
    index = resolveLocalFromArray(name, compiler->arguments, compiler->argumentCount, true);
    return index;
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal, bool isArgument)
{
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name)
{
    if (compiler->enclosing == NULL) return -1;

    LocalIndex local = resolveLocal(compiler->enclosing, name);
    if (local.index != -1) {
        if (local.isArgument) {
            compiler->enclosing->arguments[local.index].isCaptured = true;
        } else {
            compiler->enclosing->locals[local.index].isCaptured = true;
        }
        return addUpvalue(compiler, (uint8_t)local.index, true, local.isArgument);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false, false);
    }

    return -1;
}

static void addArgument(Token name)
{
    if (current->argumentCount == UINT8_COUNT) {
        error("Too many parameters in function.");
        return;
    }
    Local *argument = &current->arguments[current->argumentCount++];
    argument->name = name;
    argument->depth = current->scopeDepth;
    argument->isCaptured = false;
}

static void addLocal(Token name)
{
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static void declareVariable()
{
    if (current->scopeDepth == 0) return;

    Token *name = &parser.previous;
    // TODO: Make sure this doesn't shadow an argument, if we want to adhere to the semantics of lox.
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }
        
        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static void parseArgument()
{
    consume(TOKEN_IDENTIFIER, "Expect parameter name");

    Token *name = &parser.previous;
    for (int i = current->argumentCount - 1; i >= 0; i--) {
        Local *argument = &current->arguments[i];

        if (identifiersEqual(name, &argument->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addArgument(*name);
}

static uint8_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);
    
    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized()
{
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitDefineGlobal(CURRENT, LOX_OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList()
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void or_(bool canAssign)
{
    // TODO: Add OP_JUMP_IF_TRUE opcode and clean this up.
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void binary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:      emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:     emitBinary(CURRENT, LOX_OP_EQUAL); break;
        case TOKEN_GREATER:         emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            emitBinary(CURRENT, LOX_OP_LESS); break;
        case TOKEN_LESS_EQUAL:      emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:            emitBinary(CURRENT, LOX_OP_ADD); break;
        case TOKEN_MINUS:           emitBinary(CURRENT, LOX_OP_SUBTRACT); break;
        case TOKEN_STAR:            emitBinary(CURRENT, LOX_OP_MULTIPLY); break;
        case TOKEN_SLASH:           emitBinary(CURRENT, LOX_OP_DIVIDE); break;
        default: break; // Unreachable.
    }
}

static void call(bool canAssign)
{
    uint8_t argCount = argumentList();
    emitCall(CURRENT, LOX_OP_CALL, argCount);
    //emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

static void literal(bool canAssign)
{
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitFalse(CURRENT); break;
        case TOKEN_NIL: emitNil(CURRENT); break;
        case TOKEN_TRUE: emitTrue(CURRENT); break;
        default: return; // Unreachable.
    }
}

static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void handleArgument(int index, bool set)
{
    if (set) {
        emitSetArgument(CURRENT, current->argumentCount, index);
        // This could be an error, now that we can distinguish.
    } else {
        emitGetArgument(CURRENT, current->argumentCount, index);
    }
}

static void handleLocal(int index, bool set)
{
    if (set) {
        emitSetLocal(CURRENT, current->localCount, index);
    } else {
        emitGetLocal(CURRENT, current->localCount, index);
    }
}

static void handleUpvalue(int index, bool set)
{

}

static void handleGlobal(int index, bool set)
{
    if (set) {
        emitSetGlobal(CURRENT, LOX_OP_SET_GLOBAL, index);
    } else {
        emitGetGlobal(CURRENT, LOX_OP_GET_GLOBAL, index);
    }
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    void (*handler)(int, bool);
    LocalIndex arg = resolveLocal(current, &name);
    if (arg.index != -1) {
        if (arg.isArgument) {
            handler = handleArgument;
        } else {
            handler = handleLocal;
        }
    } else if ((arg.index = resolveUpvalue(current, &name)) != -1) {
        handler = handleUpvalue;
    } else {
        arg.index = identifierConstant(&name);
        handler = handleGlobal;
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        handler(arg.index, true);
    } else {
        handler(arg.index, false);
    }
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static Token syntheticToken(const char *text)
{
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void super_(bool canAssign)
{
    if (currentClass == NULL) {
        error("Can't user 'super' outside of a class.");
    } else {
        error("Can't use 'super' in a class with no superClass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

static void this_(bool canAssign)
{
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
}

static void unary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG: emitUnary(CURRENT, LOX_OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // unreachable
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},
    [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
    
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            parseArgument();
            //uint8_t constant = parseArgument();
            //defineVariable(constant);
        } while (match(TOKEN_COMMA));

    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();

    emitClosure(CURRENT, LOX_OP_CLOSURE, makeConstant(OBJ_VAL(function)));
    //emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    // This should be implemented as an upvalue map on the function object.
    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void method()
{
    consume(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(type);
    emitBytes(OP_METHOD, constant);
}

static void classDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void funDeclaration()
{
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration()
{
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitPop(CURRENT, 1);
}

static void forStatement()
{
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitConditionalJump(CURRENT);
        emitPop(CURRENT, 1);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump_(CURRENT);
        int incrementStart = currentChunk()->count;
        expression();
        emitPop(CURRENT, 1);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(CURRENT, loopStart);
        loopStart = incrementStart;
        patchJump_(CURRENT, bodyJump, currentChunk()->count);
    }

    statement();
    emitLoop(CURRENT, loopStart);

    if (exitJump != -1) {
        patchConditionalJump(CURRENT, exitJump, currentChunk()->count);
        emitPop(CURRENT, 1);
    }

    endScope();
}

static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitConditionalJump(CURRENT);
    emitPop(CURRENT, 1);
    statement();

    int endJump = emitJump_(CURRENT);

    patchConditionalJump(CURRENT, thenJump, currentChunk()->count);
    emitPop(CURRENT, 1);

    if (match(TOKEN_ELSE)) statement();
    patchJump_(CURRENT, endJump, currentChunk()->count);
}

static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitPrint(CURRENT, LOX_OP_PRINT);
}

static void returnStatement()
{
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn_();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitReturn(CURRENT, LOX_OP_RETURN);
        //emitByte(OP_RETURN);
    }
}

static void whileStatement()
{
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitConditionalJump(CURRENT);
    // TODO: make POP part of condition read
    emitPop(CURRENT, 1);
    //int exitJump = emitJump(OP_JUMP_IF_FALSE);
    //emitByte(OP_POP);
    statement();
    emitLoop(CURRENT, loopStart);
    //emitLoop(loopStart);

    patchConditionalJump(CURRENT, exitJump, currentChunk()->count);
    //patchJump(exitJump);
    //emitByte(OP_POP);
    // TODO: make POP part of condition read
    emitPop(CURRENT, 1);
}

static void synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
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

            default:
                ; // Do nothing.
        }

        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) synchronize();
}

static void statement()
{
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ObjFunction *compile(const char *source, bool isREPL)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_EOF, "Expect end of expression.");
    ObjFunction *function = endCompiler();
    
    if (parser.hadError) {
        return NULL;
    }

    size_t requiredSize = (compiledFunctions.combinedSize + JIT_RED_ZONE * compiledFunctions.count) * sizeof(uint32_t);

    if (isREPL) {
        assert(false);
    } else {
        void *address = mapMemory(requiredSize);
        if (!address) {
            error("Unable to map memory for JIT code");
            return NULL;
        }

        uint32_t *current = address;

        for (int i = 0; i < compiledFunctions.count; i++) {
            ObjFunction *function = compiledFunctions.functions[i];
            memcpy(current, function->chunk.code, sizeof(uint32_t) * function->chunk.count);
            function->chunk.code = current;
            function->chunk.isExecutable = true;
            current = current + function->chunk.count;

            // Write red zone.
            memset(current, 0xFF, sizeof(uint32_t) * JIT_RED_ZONE);
            current = current + JIT_RED_ZONE;
        }

        if (remapAsExecutable(address, requiredSize)) {
            unmapMemory(address, requiredSize);
            error("Unable to map memory for JIT code");
            return NULL;
        }

        // We assume that no further code allocations are necessary,
        // as only a single file is supported right now.
        vm.code = address;
        vm.codeEnd = (uint8_t *) address + requiredSize;
        vm.nextCode = NULL;
    }

    return function;
}

void markCompilerRoots()
{
    Compiler *compiler = current;
    while (compiler != NULL) {
        markObject((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}

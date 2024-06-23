#ifndef clox_code_h
#define clox_code_h

#include "common.h"
#include "object.h"

/* Number of instruction slots to have between functions in JIT memory.
 * The red zone is guaranteed to consist of invalid instructions.
 */
#define JIT_RED_ZONE 1

typedef struct {
    void *start;
    void *end;
    void *free;
    void *writable; // Start of writable pages.
    size_t available;

    ObjFunction **functions; // Sorted by code start address.
    size_t functionCount;
    size_t functionCapacity;
} Code;

void initCode(Code *code);
void freeCode(Code *code);
void codePushFunctions(Code *code, ObjFunction **functions, size_t count, size_t combinedSize, bool isREPL);
void codeExtend(Code *code, size_t newSize);
void codeRemoveWhite(Code *code);

#endif

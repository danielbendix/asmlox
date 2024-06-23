#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"
#include "code.h"

#include <setjmp.h>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    /// When we encounter this address during GC, we will stop traversing the stack.
    void *mainReturnAddress;

    void *startFrame;
    Table globals;
    Table strings;
    ObjString *initString;
    ObjUpvalue *openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;
    Obj *objects;
    int grayCount;
    int grayCapacity;
    Obj **grayStack;

    Code code;

    // Using setjmp/longjmp guarantees complete restoration of state,
    // But time and memory usage could be better.
    jmp_buf jmp;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source, bool isREPL);

#endif

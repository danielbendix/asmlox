#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

/* Number of instruction slots to have between functions in JIT memory.
 * The red zone is guaranteed to consist of undefined instructions.
 */
#define JIT_RED_ZONE 1

ObjFunction *compile(const char *source, bool isREPL);
void markCompilerRoots();

#endif

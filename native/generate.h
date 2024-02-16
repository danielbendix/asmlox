#include "../common.h"
#include "../chunk.h"

typedef enum {
    GEN_OK,
    GEN_BRANCH_OVERFLOW,
    GEN_IMMEDIATE_OVERFLOW,
} GenResult;

GenResult emitGetConstant(Chunk *chunk, int line, uint32_t index);

GenResult emitGetArgument(Chunk *chunk, int line, uint32_t argCount, uint32_t index);
GenResult emitSetArgument(Chunk *chunk, int line, uint32_t argCount, uint32_t index);

GenResult emitGetLocal(Chunk *chunk, int line, uint32_t localCount, uint32_t index);
GenResult emitSetLocal(Chunk *chunk, int line, uint32_t localCount, uint32_t index);

// TODO: emit(Get|Set)Upvalue

GenResult emitDefineGlobal(Chunk *chunk, int line, uint32_t op, uint32_t nameIndex);
GenResult emitGetGlobal(Chunk *chunk, int line, uint32_t op, uint32_t nameIndex);
GenResult emitSetGlobal(Chunk *chunk, int line, uint32_t op, uint32_t nameIndex);



GenResult emitNil(Chunk *chunk, int line);
GenResult emitFalse(Chunk *chunk, int line);
GenResult emitTrue(Chunk *chunk, int line);

GenResult emitBinary(Chunk *chunk, int line, uint32_t op);
GenResult emitPrint(Chunk *chunk, int line, uint32_t op);



GenResult emitReturn(Chunk *chunk, int line, uint32_t op);
GenResult emitReturnFromScript(Chunk *chunk, int line);

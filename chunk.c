#include <stdlib.h>
#include <value.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"

void initChunk(Chunk *chunk)
{
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	chunk->lines = NULL;
	initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk)
{
    if (!chunk->isExecutable) {
        FREE_ARRAY(uint32_t, chunk->code, chunk->capacity);
    } else {
        // FIXME: Add to free list.
    }
	FREE_ARRAY(int, chunk->lines, chunk->capacity);
	freeValueArray(&chunk->constants);
	initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint32_t op, int line)
{
	if (chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(uint32_t, chunk->code, oldCapacity, chunk->capacity);
		chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
	}

	chunk->code[chunk->count] = op;
	chunk->lines[chunk->count] = line;
	chunk->count++;
}

int addConstant(Chunk *chunk, Value value)
{
    pushGCRoot(value);
	writeValueArray(&chunk->constants, value);
    popGCRoot();
	return chunk->constants.count - 1;
}

#include "code.h"
#include "memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h>
#include <errno.h>

#define ROUND_UP_PAGE(pointer, pageSize) ((void *) (((uintptr_t) pointer + pageSize - 1) & ~(pageSize - 1)))
#define ROUND_DOWN_PAGE(pointer, pageSize) ((void *) ((uintptr_t) pointer & ~(pageSize - 1)))

#define MINIMUM_SEGMENT_SIZE (128 << 10)
#define UNDEFINED_INSTRUCTION 0xFF

static inline
size_t roundToPageSize(size_t size, size_t pageSize)
{
    size_t mask = pageSize - 1;
    return ~mask & (size + mask);
}

static 
int changeMemoryProtection(void *address, size_t size, int protection)
{
    int result = mprotect(address, size, protection);
    if (result == -1) {
        return errno;
    }
    return 0;
}

static
void setWritable(Code *code, void *from)
{
    if (code->writable > from) {
        if (changeMemoryProtection(from, code->writable - from, PROT_WRITE)) {
            printf("Unable to move write border left. Exiting...");
            exit(1);
        }
        code->writable = from;
    } else if (code->writable < from) {
        if (changeMemoryProtection(code->writable, from - code->writable, PROT_EXEC)) {
            printf("Unable to move write border right. Exiting...");
            exit(1);
        }
        code->writable = from;
    }
}

static
int changeSegmentProtection(Code *code, int protection)
{
    return changeMemoryProtection(code->start, code->end - code->start, protection);
}

static
void makeSegmentWritable(Code *code)
{
    if (changeSegmentProtection(code, PROT_WRITE)) {
        printf("Unable to make code segment writable. Exiting...");
        exit(1);
    }
    code->writable = code->start;
}

static
void makeSegmentExecutable(Code *code)
{
    if (changeSegmentProtection(code, PROT_EXEC)) {
        printf("Unable to make code segment writable. Exiting...");
        exit(1);
    }
    code->writable = code->end;
}

static
void *createNewMapping(size_t size)
{
    void *space = mmap(NULL, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (space == (void *) -1) {
        printf("Unable to create mapping for JIT code. Exiting...");
        exit(1);
    }
    return space;
}

static
void createInitialSegment(Code *code, size_t size)
{
    size_t pageSize = getpagesize();
    assert((size & (pageSize - 1)) == 0 && "Size should be a multiple of page size.");

    void *space = createNewMapping(size);
    code->start = space;
    code->end = space + size;
    code->free = space;
    code->writable = space;
    code->available = size;
    size_t initialRedZone = JIT_RED_ZONE * sizeof(uint32_t);
    memset(code->free, UNDEFINED_INSTRUCTION, initialRedZone);
    code->free += initialRedZone;
    code->available -= initialRedZone;
}

static void *createFixedMapping(void *address, size_t size)
{
    size_t pageSize = getpagesize();
    assert(((uintptr_t) address & (pageSize - 1)) == 0 && "address should be a multiple of page-aligned.");
    assert((size & (pageSize - 1)) == 0 && "Size should be a multiple of page size.");

#ifdef MAP_FIXED_NOREPLACE 
    void *space = mmap(address, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
#else
    void *space = mmap(address, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if (space != (void *) -1 && space != address) {
        munmap(space, size);
        errno = EEXIST;
        return (void *) -1;
    } else if (space == (void *) -1) {
        return (void *) -1;
    }
    return space;
}

static
int expandSegment(Code *code, size_t size)
{
    void *space = createFixedMapping(code->end, size);
    if (space == (void *) -1) {
        return errno;
    }
    return 0;
}

static
void expandOrMoveSegment(Code *code, size_t additionalSize)
{
    assert(false);

    if (expandSegment(code, additionalSize) == 0) {
        code->available += additionalSize;
        code->end += additionalSize;
        return;
    }

    assert(false);

    // We need to move the code.
    // Create a new segment with the desired size.
    // Sort function list.
    // Write functions into new segment.
    // (If compiling while code is running) Modify return addresses.
    // unmap old segment and set on code.
}

static 
void *copyFunctionCode(ObjFunction **functions, size_t functionCount, void *destination)
{
    uint32_t *write = destination;
    write += JIT_RED_ZONE;

    ObjFunction **current = functions;
    ObjFunction **const end = functions + functionCount;

    ObjFunction *function;
    for (; (function = *current)->chunk.code == write; current++) {
        write += function->chunk.count + JIT_RED_ZONE;
    }

    for (; current != end; current++) {
        ObjFunction *function = *current;
        uint32_t *writeEnd = write + function->chunk.count;
        size_t count = function->chunk.count;
        size_t size = count * sizeof(uint32_t);
        if (function->chunk.code < writeEnd) {
            memmove(write, function->chunk.code, size);
        } else {
            memcpy(write, function->chunk.code, size);
        }
        function->chunk.code = write;
        write += count;
        memset(write, UNDEFINED_INSTRUCTION, JIT_RED_ZONE * sizeof(uint32_t));
        write += JIT_RED_ZONE;
        current++;
    }

    return write;
}

static
void compactExisting(Code *code)
{
    // At this point we could trigger a GC, if the mutator has done sufficient
    // work before compilation started, to not copy any unreachable functions.
    makeSegmentWritable(code);
    code->free = copyFunctionCode(code->functions, code->functionCount, code->start);
    code->available = code->end - code->free;
}

void *writeNewFunctions(ObjFunction **functions, size_t functionCount, void *destination)
{
    uint32_t *write = destination;

    ObjFunction **current = functions;
    ObjFunction **const end = functions + functionCount;

    for (; current != end; current++) {
        ObjFunction *function = *current;
        memcpy(write, function->chunk.code, function->chunk.count * sizeof(uint32_t));

        // TODO: Move to helper function.
        FREE_ARRAY(uint32_t, function->chunk.code, function->chunk.capacity);
        function->chunk.code = write;
        function->chunk.isExecutable = true;

        write += function->chunk.count;

        memset(write, UNDEFINED_INSTRUCTION, JIT_RED_ZONE * sizeof(uint32_t));
        write += JIT_RED_ZONE;
    }

    return write;
}

void initCode(Code *code)
{
    code->start = NULL;
    code->end = NULL;
    code->free = NULL;
    code->writable = NULL;
    code->available = 0;

    code->functions = NULL;
    code->functionCount = 0;
    code->functionCapacity = 0;
}

void freeCode(Code *code)
{
    if (code->start) {
        munmap(code->start, code->end - code->start);
    }
    code->start = NULL;
    code->end = NULL;
    code->free = NULL;
    code->writable = NULL;
    code->available = 0;

    free(code->functions);
    code->functions = NULL;
    code->functionCount = 0;
    code->functionCapacity = 0;
}

void codePushFunctions(Code *code, ObjFunction **functions, size_t count, size_t combinedSize, bool isREPL)
{
    size_t pageSize = getpagesize();
    size_t requiredSize = (count * JIT_RED_ZONE + combinedSize) * sizeof(uint32_t);

    if (code->start == NULL) {
        size_t size = requiredSize + JIT_RED_ZONE * sizeof(uint32_t);
        if (isREPL) {
            if (size < MINIMUM_SEGMENT_SIZE) {
                size = MINIMUM_SEGMENT_SIZE;
            } else { // Exceedingly unlikely for REPL.
                size = size * 2;
            }
        } else {
            // We currently don't expect file runs to grow the code segment.
        }
        
        size = roundToPageSize(size, pageSize);

        createInitialSegment(code, size);
    }

    if (code->end - code->free > requiredSize) {

        setWritable(code, ROUND_DOWN_PAGE(code->free, pageSize));

        void *newFree = writeNewFunctions(functions, count, code->free);
        code->available -= newFree - code->free;
        code->free = newFree;

        setWritable(code, ROUND_UP_PAGE(code->free, pageSize));

    
        // mprotect(exec) from first page to page with free in it.

        size_t requiredCapacity = code->functionCount + count;
        if (code->functionCapacity < requiredCapacity) {
            size_t newCapacity = code->functionCapacity;
            while (newCapacity < requiredCapacity) {
                newCapacity = GROW_CAPACITY(code->functionCapacity);
            }
            code->functions = realloc(code->functions, newCapacity * sizeof(ObjFunction *));
            code->functionCapacity = newCapacity;
            if (code->functions == NULL) {
                exit(1);
            }
        }

        memcpy(code->functions + code->functionCount, functions, count * sizeof(ObjFunction *));
        code->functionCount += count;
    } else if (code->available >= requiredSize) {
        compactExisting(code);

        void *newFree = writeNewFunctions(functions, count, code->free);
        code->available -= newFree - code->free;
        code->free = newFree;

        setWritable(code, ROUND_UP_PAGE(code->free, pageSize));
    } else {
        // TODO: Expand segment.
        assert(false);
    }
}

void codeRemoveWhite(Code *code)
{
    ObjFunction **write = code->functions;
    ObjFunction **read = code->functions;
    ObjFunction **const end = code->functions + code->functionCount;

    size_t freed = 0;

    while (read != end) {
        ObjFunction *function = *read;
        if (function->obj.isMarked) {
            *write++ = *read;
        } else {
            freed += (function->chunk.count + JIT_RED_ZONE) * sizeof(uint32_t);
        }
        read++;
    }

    code->functionCount -= read - write;
    code->available += freed;
}







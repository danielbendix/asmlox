#include <stdlib.h>
#include <stdio.h>

#include <assert.h>

#include "ops.h"

#include "memory.h"
#include "value.h"
#include "vm.h"

__attribute__((noreturn))
void runtimeError(const char *string)
{

    // Run through frame pointers until we get back to the start.
    // TOOD: Unwind stack to when vm was called.
    // Maybe use setjmp/longjmp
    assert(false);
    printf("%s\n", string);
    exit(1);
}

const void *OP_TABLE[LOX_OP_COUNT] = {
    [LOX_OP_ADD] = (void *) op_add,

    [LOX_OP_LESS] = (void *) op_less,

    [LOX_OP_PRINT] = (void *) op_print,

    [LOX_OP_DEFINE_GLOBAL] = (void *) op_define_global,
    [LOX_OP_GET_GLOBAL] = (void *) op_get_global,
    [LOX_OP_SET_GLOBAL] = (void *) op_set_global,

    [LOX_OP_RETURN] = (void *) op_return,
    [LOX_OP_RETURN_NIL] = (void *) op_return_nil,
    [LOX_OP_RETURN_INITIALIZER] = (void *) LOX_OP_RETURN_INITIALIZER,
};

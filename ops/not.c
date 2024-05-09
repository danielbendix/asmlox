#include "../ops.h"
#include "../value.h"

static inline
bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

Value op_not(Value v1)
{
    return BOOL_VAL(isFalsey(v1));
}

#include "../ops.h"
#include "../value.h"

#include "binary.h"

Value op_greater(Value v1, Value v2)
{
    BINARY_OP(v1, v2, BOOL_VAL, >);
}

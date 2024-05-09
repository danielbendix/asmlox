#include "../ops.h"
#include "../value.h"

#include "binary.h"

Value op_subtract(Value v1, Value v2)
{
    BINARY_OP(v1, v2, NUMBER_VAL, -);
}

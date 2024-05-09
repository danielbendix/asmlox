#include "../ops.h"

Value op_increment(Value value)
{
    if (IS_NUMBER(value)) {
        value.as.number += 1;
        return value;
    } else {
        runtimeError("Operands must be a number.");
    }
}

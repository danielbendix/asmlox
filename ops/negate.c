#include "../ops.h"
#include "../value.h"

Value op_negate(Value value)
{
    if (IS_NUMBER(value)) {
        return NUMBER_VAL(-AS_NUMBER(value));
    }
    runtimeError("Operand must be a number.");
}

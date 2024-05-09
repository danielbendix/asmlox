#include "../ops.h"
#include "../value.h"

static inline
bool valuesEqual_(Value a, Value b)
{
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:       return true;
        case VAL_NUMBER:    return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:       return AS_OBJ(a) == AS_OBJ(b);
    }
}

Value op_equal(Value v1, Value v2)
{
    return BOOL_VAL(valuesEqual_(v1, v2));
}

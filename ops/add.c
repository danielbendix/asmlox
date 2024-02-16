#include <string.h>

#include "../memory.h"
#include "../object.h"
#include "../ops.h"
#include "../value.h"

static ObjString *concatenate_strings(ObjString *a, ObjString *b)
{
    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    return result;
}

Value op_add(Value v1, Value v2)
{
    if (v1.type == v2.type) {
        if (v1.type == VAL_NUMBER) {
            double b = AS_NUMBER(v1);
            double a = AS_NUMBER(v2);
            return NUMBER_VAL(a + b);
        } else if (IS_STRING(v1) && IS_STRING(v2)) {
            return OBJ_VAL(concatenate_strings(AS_STRING(v1), AS_STRING(v2)));
        }
    }
    runtimeError("Operands must be two numbers or two strings.");
}

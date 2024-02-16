#include "value.h"

__attribute__((noreturn))
void runtimeError(const char *string);

enum LOX_OP {
    LOX_OP_ADD,

    LOX_OP_PRINT,

    LOX_OP_DEFINE_GLOBAL,
    LOX_OP_GET_GLOBAL,
    LOX_OP_SET_GLOBAL,

    LOX_OP_RETURN,
    LOX_OP_RETURN_NIL,
    LOX_OP_RETURN_INITIALIZER,

    LOX_OP_COUNT,
};

Value op_add(Value v1, Value v2);

void op_print(Value value);

void op_define_global(Value value, ObjString *name);
Value op_get_global(ObjString *name);
Value op_set_global(Value value, ObjString *name);

void op_return();
void op_return_nil();
void op_return_initializer();

extern const void *OP_TABLE[];

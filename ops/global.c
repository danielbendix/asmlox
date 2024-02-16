#include "../memory.h"
#include "../object.h"
#include "../ops.h"
#include "../value.h"
#include "../vm.h"

void op_define_global(Value value, ObjString *name)
{
    tableSet(&vm.globals, name, value);
}

Value op_get_global(ObjString *name)
{
    Value value;
    if (!tableGet(&vm.globals, name, &value)) {
        runtimeError("Undefined variable.");
        //runtimeError("Undefined variable '%s'.", name->chars);
    }
    return value;
}

Value op_set_global(Value value, ObjString *name)
{
    if (tableSet(&vm.globals, name, value)) {
        tableDelete(&vm.globals, name);
        runtimeError("Undefined variable.");
        //runtimeError("Undefined variable '%s'.", name->chars);
    }
    return value;
}


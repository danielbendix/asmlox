#include "../ops.h"

// This signature is a hack to produce more malleable assembly
// that will load all values needed even with -O3
typedef Value (*Function)(ObjClosure *closure, Value *constants);

typedef enum {
    NOT_CALLABLE = 0,
    ARITY_MISMATCH = 1,
    

} CallError;

Value op_call(Value callee, int argCount)
{
    CallError error = NOT_CALLABLE;
    if (IS_OBJ(callee)) {
        Obj *object = AS_OBJ(callee);
        switch (object->type) {
            case OBJ_CLOSURE: {
                ObjClosure *closure = (ObjClosure *) object;
                if (closure->function->arity != argCount) {
                    error = ARITY_MISMATCH;
                    break;
                }
                Function function = (Function) closure->function->chunk.code;
                return function(closure, closure->function->chunk.constants.values);
            }
            default: break;
        }

    }
    switch (error) {
        case NOT_CALLABLE: runtimeError("Value is not callable.");
        case ARITY_MISMATCH: runtimeError("Wrong arity for call.");
    }
}

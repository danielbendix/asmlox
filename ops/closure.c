#include "../ops.h"
#include "../memory.h"

Value op_closure(ObjFunction *function)
{
    ObjClosure *closure = newClosure(function);
    // TODO: Implement upvalues
    //for (int i = 0; i < closure->upvalueCount; i++) {
    //    uint8_t isLocal = READ_BYTE();
    //    uint8_t index = READ_BYTE();
    //    if (isLocal) {
    //        closure->upvalues[i] = captureUpvalue(frame->slots + index);
    //    } else {
    //        closure->upvalues[i] = frame->closure->upvalues[index];
    //    }
    //}
    return OBJ_VAL(closure);
}

	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_return
    .globl  _op_return_nil
    .globl  _start_script
	.p2align	2

closure .req x19
constants .req x20
optable .req x21 

.equ INTERPRET_OK, 0
.equ STACK_SLOT_SHIFT, 4

; There should also be a return_initializer, and this could be specialized to return_nil
_op_return:
    ; closure is stored in x19
    ; at this point, we want to get the value on top of the stack
    ldp x0, x1, [sp] ; load return value
    ldr x2, [x19, #16] ; function pointer
    ldr w2, [x2, #16] ; function arity
    lsl x2, x2, STACK_SLOT_SHIFT

    ldp x19, x20, [x29, #-16] ; restore previous function and arity
    ldp x29, x30, [x29] ; the frame pointer is now restored, including the frame that stores fp and lr
    sub sp, x29, x2
    stp x0, x1, [sp] ; store return value on top of stack
    ret

_op_return_nil:
    ; this is not much shorter than return, but means that pushing a nil before this can be elided.
    ldr x2, [x19, #16] ; function pointer
    ldr w2, [x2, #16] ; function arity
    lsl x2, x2, STACK_SLOT_SHIFT

    ldp x19, x20, [x29, #-16] ; restore previous function and arity
    ldp x29, x30, [x29] ; the frame pointer is now restored, including the frame that stores fp and lr
    sub sp, x29, x2
    stp xzr, xzr, [sp, #-16]! ; store return value on top of stack
    ret

; This should just be emitted at the end of the script.
_return_from_script:
    ldp closure, constants, [x29, #-32]
    ldp optable, x22, [x29, #-16]
    add sp, x29, #16
    ldp x29, x30, [x29] ; reload frame pointer
    mov x0, INTERPRET_OK ; return code 0
    ret

; InterpretResult start_script(ObjClosure *closure, Value *constants, void *optable[], void *code)
_start_script:
    stp closure, constants, [sp, #-48]!
    stp optable, x22, [sp, #16]
    stp x29, x30, [sp, #32]
    add x29, sp, #32

    mov closure, x0
    mov constants, x1
    mov optable, x2
    br x3

; This should be be an adaption of callValue in vm.c
_call_function: ; x0 contains argCount, 
    ; get remaining code for call.

    stp x29, x30, [sp, #16]!
    mov x29, sp

    br x0

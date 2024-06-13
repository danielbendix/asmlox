closure .req x19
constants .req x20
optable .req x21 

.equ INTERPRET_OK, 0
.equ STACK_SLOT_SHIFT, 4

    .section __TEXT,__text,regular,pure_instructions

// There should also be a return_initializer, and this could be specialized to return_nil
    .globl _op_return
    .p2align 2
_op_return:
    // closure is stored in x19
    // at this point, we want to get the value on top of the stack
    ldp x0, x1, [sp] // load return value
    ldr x2, [x19, #16] // function pointer
    ldr w2, [x2, #16] // function arity
    lsl x2, x2, STACK_SLOT_SHIFT

    ldp closure, constants, [x29, #-16] // restore previous closure and constants
    // sub sp, x29, x2
    add sp, x29, x2
    ldp x29, x30, [x29] // the frame pointer is now restored, including the frame that stores fp and lr
    stp x0, x1, [sp, #16]! // store return value on top of stack
    ret

    .globl _op_return_nil
    .p2align 2
_op_return_nil:
    // this is not much shorter than return, but means that pushing a nil before this can be elided.
    ldr x2, [x19, #16] // function pointer
    ldr w2, [x2, #16] // function arity
    lsl x2, x2, STACK_SLOT_SHIFT

    ldp x19, x20, [x29, #-16] // restore previous function and arity
    ldp x29, x30, [x29] // the frame pointer is now restored, including the frame that stores fp and lr
    sub sp, x29, x2
    stp xzr, xzr, [sp, #-16]! // store return value on top of stack
    ret

// This is emitted at the end of the script. Not a linked function
_return_from_script:
    ldp closure, constants, [x29, #-16]
    add sp, x29, #16
    ldp x29, x30, [x29] // reload frame pointer
    mov x0, INTERPRET_OK // return code 0
    ret

// This function is generic, returning whatever the function of the closure returns
// T call_closure_no_args(ObjClosure<T> *closure)
    .globl _call_closure_no_args
    .p2align 2
_call_closure_no_args:
    // Spill registers and frame
    mov x1, #3
    stp x1, x0, [sp, #-48]!
    stp closure, constants, [sp, #16]
	stp	x29, x30, [sp, #32]
	add	x29, sp, #32

	ldr	x8, [x0, #16] // load function pointer : OFFSETOF ObjClosure.function
	ldr	w9, [x8, #16] // load arity : OFFSETOF ObjFunction.arity

    mov closure, x0
	ldr	x2, [x8, #32] // load code : OFFSETOF ObjFunction.chunk.code
	ldr	constants, [x8, #56] // load constant table : OFFSETOF ObjFunction.chunk.constants.values
	br	x2

// InterpretResult start_script(ObjClosure *closure)
    .globl _start_script
    .p2align 2
_start_script:
	adrp	x9, _vm@GOTPAGE
	ldr	x9, [x9, _vm@GOTPAGEOFF]
	str	lr, [x9] // OFFSETOF VM.mainReturnAddress
    mov x9, #0

	adrp	x8, _OP_TABLE@GOTPAGE
	ldr	x8, [x8, _OP_TABLE@GOTPAGEOFF]

    stp optable, x22, [sp, #-32]!
    stp x29, x30, [sp, #16]
    add x29, sp, #16

    mov optable, x8

    bl _call_closure_no_args

    ldp optable, x22, [x29, #-16]
    add sp, x29, #16
    ldp x29, x30, [x29]

    ret

.subsections_via_symbols

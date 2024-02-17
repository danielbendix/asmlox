	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_call                        ; -- Begin function op_call
	.p2align	2

closure .req x19
constants .req x20
optable .req x21 

_op_call:                               ; @op_call
	.cfi_startproc
; %bb.0:
    lsl x3, x2, #4 ; get the offset for the stack pointer
    add x3, sp, x3
    ldp x0, x1, [x3]
    stp closure, constants, [sp, #-32]!
	stp	x29, x30, [sp, #16]           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 32
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	cmp	w0, #3
	b.ne	LBB0_4
; %bb.1:
	ldr	w8, [x1]
	cmp	w8, #2
	b.ne	LBB0_4
; %bb.2:
	ldr	x8, [x1, #16] ; load function pointer
	ldr	w9, [x8, #16] ; load arity
	cmp	w9, w2
	b.ne	LBB0_5
; %bb.3:
	ldr	x2, [x8, #32] ; load code
	ldr	constants, [x8, #56] ; load constant table
    mov closure, x1
	;ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	br	x2
LBB0_4:
Lloh0:
	adrp	x0, l_.str@PAGE
Lloh1:
	add	x0, x0, l_.str@PAGEOFF
	bl	_runtimeError
LBB0_5:
Lloh2:
	adrp	x0, l_.str.1@PAGE
Lloh3:
	add	x0, x0, l_.str.1@PAGEOFF
	bl	_runtimeError
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpAdd	Lloh2, Lloh3
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Value is not callable."

l_.str.1:                               ; @.str.1
	.asciz	"Wrong arity for call."

.subsections_via_symbols

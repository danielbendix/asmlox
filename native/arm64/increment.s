	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_increment                   ; -- Begin function op_increment
	.p2align	2
_op_increment:                          ; @op_increment
	.cfi_startproc
    ldp x0, x1, [sp]
; %bb.0:
	cmp	w0, #2
	b.ne	LBB0_2
; %bb.1:
	fmov	d0, x1
	fmov	d1, #1.00000000
	fadd	d0, d0, d1
	fmov	x1, d0
    stp x0, x1, [sp]
	ret
LBB0_2:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh0:
	adrp	x0, l_.str@PAGE
Lloh1:
	add	x0, x0, l_.str@PAGEOFF
	bl	_runtimeError
	.loh AdrpAdd	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Operands must be a number."

.subsections_via_symbols

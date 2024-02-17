	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_less                        ; -- Begin function op_less
	.p2align	2
_op_less:                               ; @op_less
	.cfi_startproc
; %bb.0:
    ldp x0, x1, [sp, #16]
    ldp x2, x3, [sp]
    cmp w0, #2
    ccmp w0, w2, #0x0, eq
    b.ne LBB0_2
; %bb.1:
	fmov	d0, x3
	fmov	d1, x1
	fcmp	d1, d0
	cset	w1, mi
	mov	w0, #1
    stp x0, x1, [sp, #16]!
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
	.asciz	"Operands must be numbers."

.subsections_via_symbols

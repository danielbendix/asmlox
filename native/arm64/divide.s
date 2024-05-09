	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_divide                      ; -- Begin function op_divide
	.p2align	2
_op_divide:                             ; @op_divide
	.cfi_startproc
; %bb.0:
	cmp	w0, w2
	cset	w8, ne
	cmp	w0, #2
	csetm	w9, ne
	cmp	w8, w9
	b.ne	LBB0_2
; %bb.1:
	fmov	d0, x3
	fmov	d1, x1
	fdiv	d0, d1, d0
	fmov	x1, d0
	mov	w0, #2
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

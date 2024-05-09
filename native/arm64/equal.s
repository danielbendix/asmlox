	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_equal                       ; -- Begin function op_equal
	.p2align	2
_op_equal:                              ; @op_equal
	.cfi_startproc
; %bb.0:
    ldp x0, x1, [sp, #16]
    ldp x2, x3, [sp]
	fmov	d0, x1
	fmov	d1, x3
	mov	w8, #1
	cmp	x1, x3
	cset	w9, eq
	fcmp	d0, d1
	cset	w10, eq
	cmp	w1, w3
	cset	w11, eq
	cmp	w0, #1
	csel	w8, w8, w11, ne
	cmp	w0, #2
	csel	w8, w10, w8, eq
	cmp	w0, #3
	csel	w8, w9, w8, eq
	cmp	w0, w2
	csel	w1, wzr, w8, ne
	mov	w0, #1
    stp x0, x1, [sp, #16]!
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols

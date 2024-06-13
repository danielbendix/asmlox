    .section __TEXT,__text,regular,pure_instructions
    .globl _op_not
    .p2align 2
_op_not:
	.cfi_startproc
// %bb.0:
    // TODO: Can we assume that the single value will be loaded into x0 and x1 here?
    ldp x0, x1, [sp, #0]

	cmp	w0, #1
	ccmp	w1, #0, #0, eq
	ccmp	w0, #0, #4, ne
	cset	w1, eq
	mov	w0, #1
    
    stp x0, x1, [sp, #0]

	ret
	.cfi_endproc
                                        // -- End function
.subsections_via_symbols

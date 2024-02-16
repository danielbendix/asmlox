	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_print                       ; -- Begin function op_print
	.p2align	2
_op_print:                              ; @op_print
	.cfi_startproc
    ldp x0, x1, [sp], #16
; %bb.0:
	b	_printValue
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols

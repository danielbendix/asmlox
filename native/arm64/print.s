	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_print                       ; -- Begin function op_print
	.p2align	2
_op_print:                              ; @op_print
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_printValue
	mov	w0, #10
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	b	_putchar
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols

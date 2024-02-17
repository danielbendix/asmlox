	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_closure                     ; -- Begin function op_closure
	.p2align	2
_op_closure:                            ; @op_closure
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-32]!           ; 16-byte Folded Spill
    mov w1, #3
    stp x1, x0, [sp, #16]
	.cfi_def_cfa_offset 32
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_newClosure
	mov	x1, x0
	mov	w0, #3
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
    stp x0, x1, [sp]
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols

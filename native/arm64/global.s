	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_define_global               ; -- Begin function op_define_global
	.p2align	2
_op_define_global:                      ; @op_define_global
	.cfi_startproc
; %bb.0:
    mov x1, x2
    ldp x2, x3, [sp], #16
Lloh0:
	adrp	x9, _vm@GOTPAGE
Lloh1:
	ldr	x9, [x9, _vm@GOTPAGEOFF]
	add	x0, x9, #8
	b	_tableSet
	.loh AdrpLdrGot	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function


	.globl	_op_get_global                  ; -- Begin function op_get_global
	.p2align	2
_op_get_global:                         ; @op_get_global
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	stp	x29, x30, [sp]             ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 0
	.cfi_offset w30, 8
	.cfi_offset w29, 0
	mov	x1, x0
Lloh2:
	adrp	x8, _vm@GOTPAGE
Lloh3:
	ldr	x8, [x8, _vm@GOTPAGEOFF]
	add	x0, x8, #8
	add	x2, sp, #16
	bl	_tableGet
	tbz	w0, #0, LBB1_2
; %bb.1:
	ldp	x0, x1, [sp, #16]
	ldp	x29, x30, [sp]             ; 16-byte Folded Reload
	add	sp, sp, #16
	ret
LBB1_2:
Lloh4:
	adrp	x0, l_.str@PAGE
Lloh5:
	add	x0, x0, l_.str@PAGEOFF
	bl	_runtimeError
	.loh AdrpLdrGot	Lloh2, Lloh3
	.loh AdrpAdd	Lloh4, Lloh5
	.cfi_endproc
                                        ; -- End function


	.globl	_op_set_global                  ; -- Begin function op_set_global
	.p2align	2
_op_set_global:                         ; @op_set_global
	.cfi_startproc
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 32
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	mov	x20, x2
    ldp x2, x3, [sp, #32] ; load value from stack
Lloh6:
	adrp	x8, _vm@GOTPAGE
Lloh7:
	ldr	x8, [x8, _vm@GOTPAGEOFF]
	add	x19, x8, #8
    mov x0, x19
	mov	x1, x20
	bl	_tableSet
	cbnz	w0, LBB2_2
; %bb.1:
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
    ; TODO: We could load the value from the stack here.
	ret
LBB2_2:
	mov	x0, x19
	mov	x1, x20
	bl	_tableDelete
Lloh8:
	adrp	x0, l_.str@PAGE
Lloh9:
	add	x0, x0, l_.str@PAGEOFF
	bl	_runtimeError
	.loh AdrpLdrGot	Lloh6, Lloh7
	.loh AdrpAdd	Lloh8, Lloh9
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Undefined variable."

.subsections_via_symbols

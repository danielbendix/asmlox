	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_add                         ; -- Begin function op_add
	.p2align	2
_op_add:                                ; @op_add
	.cfi_startproc
; %bb.0:
    ; TODO: Load from stack
    ; TODO: Make all value types different 1-bits, and use bitwise AND here.
	cmp	w0, w2
	b.ne	LBB0_6
; %bb.1:
	cmp	w0, #2
	b.ne	LBB0_3
; %bb.2:
	fmov	d0, x1
	fmov	d1, x3
	fadd	d0, d0, d1
	fmov	x1, d0
    ; TODO: Is the order of operands correct here?
    stp     x0, x1, [sp, #-16]!
	ret
LBB0_3:
	cmp	w0, #3
	b.ne	LBB0_6
LBB0_4:
	stp	x22, x21, [sp, #-48]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 48
	stp	x20, x19, [sp, #16]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #32]             ; 16-byte Folded Spill
	add	x29, sp, #32
	mov	x19, x3
	mov	x20, x1
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	ldr	w8, [x20]
	and	x9, x2, #0xffffffff
	cmp	w8, #6
	ccmp	x9, #3, #0, eq
	b.ne	LBB0_6
; %bb.5:
	ldr	w8, [x19]
	cmp	w8, #6
	b.ne	LBB0_6
; %bb.5:
	ldrsw	x8, [x20, #16]
	ldrsw	x9, [x19, #16]
	add	x21, x9, x8
	add	x2, x21, #1
	mov	x0, #0
	mov	x1, #0
	bl	_reallocate
	mov	x22, x0
	ldr	x1, [x20, #24]
	ldrsw	x2, [x20, #16]
	bl	_memcpy
	ldrsw	x8, [x20, #16]
	add	x0, x22, x8
	ldr	x1, [x19, #24]
	ldrsw	x2, [x19, #16]
	bl	_memcpy
	strb	wzr, [x22, x21]
	mov	x0, x22
	mov	x1, x21
	bl	_takeString
	mov	x1, x0
	mov	w0, #3
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	ret
LBB0_6:
	stp	x29, x30, [sp, #-16]!             ; 16-byte Folded Spill
    mov x29, sp
	.cfi_def_cfa_offset 16 ; TODO: Is this right?
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_runtimeError
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function runtimeError
_runtimeError:                          ; @runtimeError
	.cfi_startproc
; %bb.0:
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
	bl	_puts
	mov	w0, #1
	bl	_exit
	.loh AdrpAdd	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.globl	_op_print                       ; -- Begin function op_print
	.p2align	2
_op_print:                              ; @op_print
	.cfi_startproc
; %bb.0:
	b	_printValue
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Operands must be two numbers or two strings."

.subsections_via_symbols
